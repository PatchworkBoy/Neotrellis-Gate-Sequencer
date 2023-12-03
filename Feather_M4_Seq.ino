/**
 * Feather M4 Seq -- An 8 track 32 step MIDI (& Analog) Gate (& CV) Sequencer for Feather M4 Express & Neotrellis 8x8, with probability, looping, swing, velocity, arpeggiator and mutes
 * 04 Nov 2023 - @apatchworkboy / Marci https://github.com/PatchworkBoy/Neotrellis-Gate-Sequencer
 * Based on https://github.com/todbot/picostepseq/
 * 28 Apr 2023 - @todbot / Tod Kurt
 * 15 Aug 2022 - @todbot / Tod Kurt
 *
 * Libraries needed (all available via Arduino Library Manager):
 * - Adafruit SPIFlash -- https://github.com/adafruit/Adafruit_SPIFlash
 * - Adafruit_TinyUSB -- https://github.com/adafruit/Adafruit_TinyUSB_Arduino
 * - Adafruit Seesaw -- https://github.com/adafruit/Adafruit_Seesaw
 * - Adafruit Neopixel -- https://github.com/adafruit/Adafruit_Neopixel
 * - MIDI -- https://github.com/FortySevenEffects/arduino_midi_library
 * - ArduinoJson -- https://arduinojson.org/
 *
 * To upload:
 * - Use Arduino IDE 1.8.19+
 * - In "Tools", set "Tools / USB Stack: Adafruit TinyUSB"
 * - Program the sketch to the Feather M4 Express with "Upload"
 *
 **/

#include <SPI.h>
#include <SdFat.h>
#include <Adafruit_SPIFlash.h>
#include <Adafruit_TinyUSB.h>
#include <Adafruit_NeoPixel_ZeroDMA.h>
#include <Adafruit_NeoTrellis.h>
#include <MIDI.h>
#include <ArduinoJson.h>
#include <algorithm>
#include <vector>

// Enable analog Gate / CV output options - have you wired sockets to pins referenced at ln 80 / 81? If so... set to true
const bool analog_feats = true;

// Enable Serial MIDI output - have you wired a TRS/DIN socket to TX pin? If so... set to true
const bool serial_midi = true;

// HOW BIG IS YOUR TRELLIS?
#define Y_DIM 8  //number of rows of key
#define X_DIM 8  //number of columns of keys

// Serial Console logging...
const bool midi_out_debug = false;
const bool midi_in_debug = false;
const bool marci_debug = false;

#define t_size Y_DIM* X_DIM
#define t_length t_size - 1

Adafruit_NeoPixel_ZeroDMA strip(2, 8, NEO_GRB + NEO_KHZ800);
Adafruit_NeoTrellis t_array[Y_DIM / 4][X_DIM / 4] = {
  { Adafruit_NeoTrellis(0x30), Adafruit_NeoTrellis(0x2E) },
  { Adafruit_NeoTrellis(0x2F), Adafruit_NeoTrellis(0x31) }
};
Adafruit_MultiTrellis trellis((Adafruit_NeoTrellis*)t_array, Y_DIM / 4, X_DIM / 4);

#include "color_defs.h"

Adafruit_USBD_MIDI usb_midi;  // USB MIDI object
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDIusb);  // USB MIDI
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, serialmidi);    // Serial MIDI

#define D_FLASH "/M4SEQ32"

const uint8_t numtracks = X_DIM;
const uint8_t num_steps = t_size / 2;
const uint8_t numpresets = X_DIM * 2;

// Control Rows
const byte row1 = num_steps;
const byte row2 = num_steps + X_DIM;
const byte row3 = num_steps + (X_DIM * 2);
const byte row4 = num_steps + (X_DIM * 3);

// none of these matter if analog_feats == false
const uint8_t numdacs = 2;
const uint16_t dacrange = 4095;  // A0 & A1 = 12bit DAC on Feather M4 Express
const byte cvpins[2] = { 14, 15 };
const byte gatepins[numtracks] = { 4, 5, 6, 9, 10, 11, 12, 13 };
bool hzv[numdacs] = { 0, 0 };
// end of hardware-bound defs

// soft flags...
int midichan[16];
uint8_t length = t_size / 2;  // same as num_steps at boot!

// UI --------
uint8_t brightness = 50;
uint8_t sel_track;
uint8_t lastsel;
uint8_t lastchan;
uint8_t selstep;
uint8_t shifted;  // (SHIFT)
uint8_t transpose;

// typedefs
typedef enum {
  _PATTERN = 0,
  _VELOCITY = 1,
  _PROBABILITY = 2,
  _GATE = 3,
  _CONFIG = 4,
  _DIVISION = 5,
  _NOTE = 6,
  _CHORD = 7,
  _LENGTH = 8,
  _OFFSET = 9,
  _SWING = 10,
  _PRESET = 11,
  _CONFIRM = 100
} UI_Mode;

UI_Mode uimode;
// key masks
bool chanedit; // CONFIG
bool divedit; // DIVISION
bool gateedit; // GATE
bool notesedit; // NOTE
bool patedit; // PATTERN
bool probedit; // PROBABILITY
bool veledit; // VELOCITY
bool lenedit; // LENGTH
bool offedit; // OFFSET
bool swingedit; // SWING

// panes
bool presetmode; // PRESET
bool sure; // CONFIRM

bool m4init;
bool write;

#include "multisequencer.h"

typedef struct {
  int step_size;
  bool midi_send_clock;
  bool midi_forward_usb;
} Config;

Config cfg = {
  .step_size = SIXTEENTH_NOTE,
  .midi_send_clock = true,
  .midi_forward_usb = true,
};

float tempo = 120;
MultiStepSequencer<numtracks, numpresets, num_steps, numdacs, numarps> seqr;

uint8_t midiclk_cnt = 0;
uint32_t midiclk_last_micros = 0;

//
// -- MIDI sending & receiving functions
//
// callback used by Sequencer to send song position in beats since start
void send_song_pos(int beat) {
  MIDIusb.sendSongPosition(beat);
  if (serial_midi) serialmidi.sendSongPosition(beat);
}

// callback used by Sequencer to trigger note on
void send_note_on(uint8_t note, uint8_t vel, uint8_t gate, bool on, uint8_t chan) {
  if (on) {
    MIDIusb.sendNoteOn(note, vel, chan);
    if (serial_midi) serialmidi.sendNoteOn(note, vel, chan);
  }
  if (midi_out_debug) { Serial.printf("noteOn:  %d %d %d %d\n", note, vel, gate, on); }
}

// callback used by Sequencer to trigger note off
void send_note_off(uint8_t note, uint8_t vel, uint8_t gate, bool on, uint8_t chan) {
  if (on) {
    MIDIusb.sendNoteOff(note, vel, chan);
    if (serial_midi) serialmidi.sendNoteOff(note, vel, chan);
  }
  if (midi_out_debug) { Serial.printf("noteOff: %d %d %d %d\n", note, vel, gate, on); }
}

// callback used by Sequencer to send controlchange messages
void send_cc(uint8_t cc, uint8_t val, bool on, uint8_t chan) {
  if (on) {
    MIDIusb.sendControlChange(cc, val, chan);
    if (serial_midi) serialmidi.sendControlChange(cc, val, chan);
  }
  if (midi_out_debug) { Serial.printf("controlChange: %d %d %d %d\n", cc, val, on, chan); }
}

// callback used by Sequencer to send midi clock when internally triggered
void send_clock_start_stop(clock_type_t type) {
  if (type == START) {
    MIDIusb.sendStart();
    if (serial_midi) serialmidi.sendStart();
  } else if (type == STOP) {
    MIDIusb.sendStop();
    if (serial_midi) serialmidi.sendStop();
  } else if (type == CLOCK) {
    MIDIusb.sendClock();
    if (serial_midi) serialmidi.sendClock();
  }
  if (midi_out_debug) { Serial.printf("\tclk:%d\n", type); }
}

void handle_midi_in_start() {
  seqr.play();
  midiclk_cnt = 0;
  if (midi_in_debug) { Serial.println("midi in start"); }
}

void handle_midi_in_stop() {
  seqr.stop();
  if (midi_in_debug) { Serial.println("midi in stop"); }
}

// FIXME: midi continue?
void handle_midi_in_clock() {
  uint32_t now_micros = micros();
  // once every ticks_per_step, play note (24 ticks per quarter note => 6 ticks per 16th note)
  if (midiclk_cnt % seqr.ticks_per_step == 0) {  // ticks_per_step = 6 for 16th note
    seqr.trigger_ext(now_micros);
  }
  midiclk_cnt++;
  // once every quarter note, calculate new BPM, but be a little cautious about it
  if (midiclk_cnt == ticks_per_quarternote) {
    uint32_t new_tick_micros = (now_micros - midiclk_last_micros) / ticks_per_quarternote;
    if (new_tick_micros > seqr.tick_micros / 2 && new_tick_micros < seqr.tick_micros * 2) {
      seqr.tick_micros = new_tick_micros;
    }
    midiclk_last_micros = now_micros;
    midiclk_cnt = 0;
  }
}

// ln 396: To handle incoming SONG POS, come up with a way to navigate to:
// int songstepi = std::floor((((BEAT - 1) * steps_per_beat_default) + 1) / num_steps);
// int patternstepi = (((BEAT - 1) * steps_per_beat_default) + 1) % num_steps);
// (after coming up with song mode)

void handle_midi_in_NoteOff(uint8_t channel, uint8_t note, uint8_t vel) {
  uint8_t trk_arr = sel_track - 1;
  if (marci_debug) Serial.println("Note Stop");
  if (seqr.mutes[trk_arr] == 1) {
    if (serial_midi) serialmidi.sendNoteOff(note, vel, seqr.track_chan[trk_arr]);
    MIDIusb.sendNoteOff(note, vel, seqr.track_chan[trk_arr]);
  } else {
    switch (seqr.modes[trk_arr]) {
      case TRIGATE:
        MIDIusb.sendNoteOff(seqr.track_notes[trk_arr], vel, seqr.track_chan[trk_arr]);
        if (serial_midi) serialmidi.sendNoteOff(seqr.track_notes[trk_arr], vel, seqr.track_chan[trk_arr]);
        break;
      case NOTE:
        MIDIusb.sendNoteOff(note, vel, seqr.track_chan[trk_arr]);
        if (serial_midi) serialmidi.sendNoteOff(note, vel, seqr.track_chan[trk_arr]);
        break;
      case ARP:
        if (marci_debug) Serial.println("Firing NoteOff to arp for");
        if (marci_debug) Serial.println(note);
        arps[trk_arr - numarps].NoteOff(note);
        break;
      default: break;
    }
  }
}

void handle_midi_in_NoteOn(uint8_t channel, uint8_t note, uint8_t vel) {
  uint8_t trk_arr = sel_track - 1;
  if (marci_debug) Serial.println("Note Start");
  if (seqr.mutes[trk_arr] == 1) {
    if (seqr.modes[trk_arr] == TRIGATE) {
      note = seqr.track_notes[trk_arr];
    }
    if (serial_midi) serialmidi.sendNoteOn(note, vel, seqr.track_chan[trk_arr]);
    MIDIusb.sendNoteOn(note, vel, seqr.track_chan[trk_arr]);
  } else {
    uint8_t _s = constrain(seqr.ticki > 3 ? seqr.multistepi[trk_arr] + 1 : seqr.multistepi[trk_arr], 0, num_steps - 1);
    switch (seqr.modes[trk_arr]) {
      case CC:
        if (marci_debug) Serial.println("CC");
        if (veledit == 1) {
          switch (shifted) {
            case 0:
              seqr.vels[seqr.presets[trk_arr]][trk_arr][selstep] = note;
              seqr.seqs[seqr.presets[trk_arr]][trk_arr][selstep] = 1;
              break;
            case 1:
              // LIVE ENTRY
              seqr.vels[seqr.presets[trk_arr]][trk_arr][_s] = note;
              seqr.seqs[seqr.presets[trk_arr]][trk_arr][_s] = 1;
              break;
            default: break;
          }
        }
        break;
      case NOTE:
        if (marci_debug) Serial.println("Note");
        if (veledit == 1 || patedit == 1) {
          switch (shifted) {
            case 0:
              send_note_off(seqr.notes[seqr.presets[trk_arr]][trk_arr][selstep], 0, 0, 1, seqr.track_chan[trk_arr]);
              seqr.notes[seqr.presets[trk_arr]][trk_arr][selstep] = note;
              seqr.vels[seqr.presets[trk_arr]][trk_arr][selstep] = vel;
              seqr.seqs[seqr.presets[trk_arr]][trk_arr][selstep] = 1;
              break;
            case 1:
              // LIVE ENTRY
              send_note_off(seqr.notes[seqr.presets[trk_arr]][trk_arr][seqr.multistepi[trk_arr]], 0, 0, 1, seqr.track_chan[trk_arr]);
              MIDIusb.sendNoteOn(note, vel, seqr.track_chan[trk_arr]);
              if (serial_midi) serialmidi.sendNoteOn(note, vel, seqr.track_chan[trk_arr]);

              seqr.notes[seqr.presets[trk_arr]][trk_arr][_s] = note;
              seqr.vels[seqr.presets[trk_arr]][trk_arr][_s] = vel;
              seqr.seqs[seqr.presets[trk_arr]][trk_arr][_s] = 1;
              break;
            default: break;
          }
        }
        break;
      case TRIGATE:
        if (marci_debug) Serial.println("Trigate");
        switch (shifted) {
          case 0:
            seqr.vels[seqr.presets[trk_arr]][trk_arr][selstep] = note;
            seqr.seqs[seqr.presets[trk_arr]][trk_arr][selstep] = 1;
            break;
          case 1:
            // LIVE ENTRY
            MIDIusb.sendNoteOn(seqr.track_notes[trk_arr], vel, seqr.track_chan[trk_arr]);
            if (serial_midi) serialmidi.sendNoteOn(seqr.track_notes[trk_arr], vel, seqr.track_chan[trk_arr]);

            seqr.vels[seqr.presets[trk_arr]][trk_arr][_s] = note;
            seqr.seqs[seqr.presets[trk_arr]][trk_arr][_s] = 1;
            trellis.setPixelColor(seqr.multistepi[trk_arr], W100);
            break;
          default: break;
        }
        break;
      case ARP:
          if (marci_debug) Serial.println("Arp");
          switch (shifted) {
            case 0:
              if (marci_debug) Serial.println("Unshifted - ignore");
              break;
            case 1:
              if (marci_debug) Serial.println("Sending to Arp");
              arps[trk_arr - numarps].NoteOn(note);
              break;
            default: break;
          }
          break;
      default: break;
    }
  }
}

void handle_midi_in_CC(uint8_t channel, uint8_t cc, uint8_t val) {
  uint8_t trk_arr = sel_track - 1;
  Serial.println("CC Incoming");
  if (seqr.mutes[trk_arr] == 1) {
    if (serial_midi) serialmidi.sendControlChange(cc, val, seqr.track_chan[trk_arr]);
    MIDIusb.sendControlChange(cc, val, seqr.track_chan[trk_arr]);
  } else {
    switch (seqr.modes[trk_arr]) {
      case CC:
        switch (shifted) {
          case 0:
            seqr.track_notes[trk_arr] = cc;
            seqr.vels[seqr.presets[trk_arr]][trk_arr][selstep] = val;
            break;
          case 1:
            seqr.track_notes[trk_arr] = cc;
            seqr.vels[seqr.presets[trk_arr]][trk_arr][seqr.multistepi[trk_arr]] = val;
            break;
          default: break;
        }
        break;
      case NOTE:
        switch (shifted) {
          case 0:
            seqr.vels[seqr.presets[trk_arr]][trk_arr][selstep] = val;
            break;
          case 1:
            seqr.vels[seqr.presets[trk_arr]][trk_arr][seqr.multistepi[trk_arr]] = val;
            break;
          default: break;
        }
        break;
      default: break;
    }
  }
}


void midi_read_and_forward() {
  if (MIDIusb.read()) {
    midi::MidiType t = MIDIusb.getType();
    switch (t) {
      case midi::Start:
        handle_midi_in_start();
        break;
      case midi::Stop:
        handle_midi_in_stop();
        break;
      case midi::Clock:
        handle_midi_in_clock();
        break;
      // case midi::SongPosition: see ln236
      // break;
      default: break;
    }
  }
}


// Analog IO
void analog_gate(uint8_t pin, uint8_t direction) {
  if (analog_feats) digitalWrite(pin, direction == 1 ? HIGH : LOW);
}

void analog_cv(uint8_t pin, uint16_t value) {
  if (analog_feats) analogWrite(pin, value);
}


void update_display() {
  if (presetmode == 1 || chanedit == 1 || sure == 1 || divedit == 1) {
    return;
  }
  uint32_t color = 0;
  uint32_t hit = 0;
  uint8_t trk_arr = sel_track - 1;

  //active step ticker
  if (gateedit == 1) {
    hit = seqr.gates[seqr.presets[trk_arr]][trk_arr][seqr.multistepi[trk_arr]] > 0 ? PURPLE : W100;
    color = seqr.gates[seqr.presets[trk_arr]][trk_arr][seqr.laststeps[trk_arr]] < 15 ? Wheel(seqr.gates[seqr.presets[trk_arr]][trk_arr][seqr.laststeps[trk_arr]] * 5) : seq_col(sel_track);
  } else if (probedit == 1) {
    hit = seqr.seqs[seqr.presets[trk_arr]][trk_arr][seqr.multistepi[trk_arr]] > 0 ? PURPLE : W100;
    color = seqr.probs[seqr.presets[trk_arr]][trk_arr][seqr.laststeps[trk_arr]] < 10 ? Wheel(seqr.probs[seqr.presets[trk_arr]][trk_arr][seqr.laststeps[trk_arr]] * 10) : seq_col(sel_track);
  } else if (veledit == 1) {
    hit = seqr.multistepi[trk_arr] != selstep ? seqr.seqs[seqr.presets[trk_arr]][trk_arr][seqr.multistepi[trk_arr]] > 0 ? PURPLE : W100 : W100;
    if (seqr.modes[trk_arr] == CC || seqr.modes[trk_arr] == NOTE) {
      color = Wheel(seqr.vels[seqr.presets[trk_arr]][trk_arr][seqr.laststeps[trk_arr]]);
    } else {
      color = seq_dim(sel_track, seqr.vels[seqr.presets[trk_arr]][trk_arr][seqr.laststeps[trk_arr]]);
    }
  } else if (notesedit == 1) {
    hit = seqr.multistepi[trk_arr] != selstep ? seqr.seqs[seqr.presets[trk_arr]][trk_arr][seqr.multistepi[trk_arr]] > 0 ? PURPLE : W100 : W100;
    if (seqr.modes[trk_arr] == CC || seqr.modes[trk_arr] == NOTE) {
      color = Wheel(seqr.notes[seqr.presets[trk_arr]][trk_arr][seqr.laststeps[trk_arr]]);
    } else {
      color = seq_dim(sel_track, seqr.notes[seqr.presets[trk_arr]][trk_arr][seqr.laststeps[trk_arr]]);
    }
  } else {
    hit = seqr.seqs[seqr.presets[trk_arr]][trk_arr][seqr.multistepi[trk_arr]] > 0 ? PURPLE : W100;
    color = seqr.seqs[seqr.presets[trk_arr]][trk_arr][seqr.laststeps[trk_arr]] > 0 ? seq_col(sel_track) : 0;
  }
  trellis.setPixelColor(seqr.multistepi[trk_arr], hit);
  strip.setPixelColor(0, seqr.seqs[seqr.presets[trk_arr]][trk_arr][seqr.multistepi[trk_arr]] > 0 ? seq_col(sel_track) : (seqr.pulse == 1 ? W40 : 0));
  if (seqr.modes[trk_arr] == CC && veledit == 1) {
    trellis.setPixelColor(seqr.laststeps[trk_arr], seqr.laststeps[trk_arr] != selstep ? color : W100);
  } else {
    trellis.setPixelColor(seqr.laststeps[trk_arr], color);
  }
  if (seqr.offsets[trk_arr] - 1 > 0) trellis.setPixelColor(seqr.offsets[trk_arr] - 1, trk_arr > 1 ? R127 : B127);
  if (seqr.lengths[trk_arr] < num_steps) trellis.setPixelColor(seqr.lengths[trk_arr] - 1, trk_arr != 4 ? C127 : G127);
  
  if (seqr.resetflag == 1) {
    if (gateedit == 1) {
      show_gates(sel_track);
    } else if (veledit == 1) {
      show_accents(sel_track);
    } else if (probedit == 1) {
      show_probabilities(sel_track);
    } else {
      show_sequence(sel_track);
    }
    seqr.resetflag = 0;
  }
  
  trellis.show();
  strip.show();
}

void reset_display() {
  uint32_t hit = 0;
  uint8_t trk_arr = sel_track - 1;
  if (patedit == 1) {
    hit = seqr.seqs[seqr.presets[trk_arr]][trk_arr][seqr.multistepi[trk_arr]] > 0 ? seq_col(sel_track) : W10;
  } else if (gateedit == 1) {
    hit = seqr.gates[seqr.presets[trk_arr]][trk_arr][seqr.multistepi[trk_arr]] > 0 ? seq_col(sel_track) : W10;
  } else if (veledit == 1) {
    hit = seqr.vels[seqr.presets[trk_arr]][trk_arr][seqr.multistepi[trk_arr]] > 0 ? seq_col(sel_track) : W10;
  } else if (probedit == 1) {
    hit = seqr.probs[seqr.presets[trk_arr]][trk_arr][seqr.multistepi[trk_arr]] > 0 ? seq_col(sel_track) : W10;
  } else {
    hit = W10;
  }
  trellis.setPixelColor(seqr.multistepi[trk_arr], hit);
  strip.setPixelColor(0, seq_col(sel_track));
  strip.show();
}

void toggle_selected(uint8_t keyId) {
  trellis.setPixelColor(lastsel + (num_steps - 1), seq_dim(lastsel, 40));
  trellis.setPixelColor(keyId, seq_col(sel_track));
  if (!seqr.playing) {
    strip.setPixelColor(0, seq_col(sel_track));
    if (!seqr.playing) { strip.show(); }
  }
}

void show_sequence(uint8_t seq) {
  for (uint8_t i = 0; i < num_steps; ++i) {
    trellis.setPixelColor(i, seqr.seqs[seqr.presets[seq - 1]][seq - 1][i] > 0 ? 
    seq_col(sel_track) : 0);
  }
  if (!seqr.playing) { trellis.show(); }
}

void set_gate(uint8_t gid, uint8_t stp, int c) {
  uint32_t col = seqr.gates[seqr.presets[gid]][gid][stp] >= 15 ? c : Wheel(seqr.gates[seqr.presets[gid]][gid][stp] * 5);
  trellis.setPixelColor(stp, col);
  if (!seqr.playing) { trellis.show(); }
}

void show_gates(uint8_t seq) {
  gateedit = 1;
  uint8_t gateId = seq - 1;
  uint32_t col = 0;
  for (uint8_t i = 0; i < num_steps; ++i) {
    set_gate(gateId, i, seq_col(seq));
  }
  if (!seqr.playing) { trellis.show(); }
}

void show_probabilities(uint8_t seq) {
  probedit = 1;
  uint32_t col = 0;
  for (uint8_t i = 0; i < num_steps; ++i) {
    col = seqr.probs[seqr.presets[seq - 1]][seq - 1][i] == 10 ? seq_col(seq) : Wheel(seqr.probs[seqr.presets[seq - 1]][seq - 1][i] * 10);
    trellis.setPixelColor(i, col);
  }
  if (!seqr.playing) { trellis.show(); }
}

void show_accents(uint8_t seq) {  // velocities
  uint8_t trk_arr = sel_track - 1;
  veledit = 1;
  uint32_t col = 0;
  if (seqr.modes[seq - 1] == CC) {
    for (uint8_t i = 0; i < num_steps; ++i) {
      trellis.setPixelColor(i, Wheel(seqr.vels[seqr.presets[trk_arr]][seq - 1][i]));
    }
  } else {
    for (uint8_t i = 0; i < num_steps; ++i) {
      trellis.setPixelColor(i, seq_dim(seq, seqr.vels[seqr.presets[trk_arr]][seq - 1][i]));
    }
  }
  if (!seqr.playing) { trellis.show(); }
}

void show_notes(uint8_t seq) {
  uint8_t trk_arr = sel_track - 1;
  notesedit = 1;
  uint32_t col = 0;
  if (seqr.modes[seq - 1] == NOTE) {
    for (uint8_t i = 0; i < num_steps; ++i) {
      trellis.setPixelColor(i, Wheel(seqr.notes[seqr.presets[trk_arr]][seq - 1][i]));
    }
  } else {
    for (uint8_t i = 0; i < num_steps; ++i) {
      trellis.setPixelColor(i, seq_dim(seq, seqr.notes[seqr.presets[trk_arr]][seq - 1][i]));
    }
  }
  if (!seqr.playing) { trellis.show(); }
}

void show_presets() {
  uint8_t trk_arr = sel_track - 1;
  for (uint8_t i = 0; i < num_steps; ++i) {
    trellis.setPixelColor(i, i < (num_steps / 2) ? 0 : W10);
  }
  trellis.setPixelColor(seqr.presets[trk_arr], W100);
  trellis.show();
}

void show_divisions() {
  uint8_t trk_arr = sel_track - 1;
  for (uint8_t i = 0; i < num_steps; ++i) {
    trellis.setPixelColor(i, 0);
  }
  trellis.setPixelColor(seqr.divs[trk_arr], seq_col(sel_track));
  trellis.setPixelColor(53, O80);
  trellis.setPixelColor(52, W100);
  trellis.show();
}

// Update Transpose Key
void transpose_led() {
  switch (transpose) {
    case 0:
      trellis.setPixelColor(53, B40);
      break;
    case 12:
      trellis.setPixelColor(53, B80);
      break;
    case 24:
      trellis.setPixelColor(53, B127);
      break;
    default:
      break;
  }
  if (!seqr.playing) { trellis.show(); }
}

// Update Channel Config MODE buttons
void mode_leds(uint8_t track) {
  for (uint8_t i = X_DIM * 3; i < num_steps; ++i) {
    trellis.setPixelColor(i, 0);
  }
  switch (seqr.modes[track - 1]) {
    case TRIGATE:
      trellis.setPixelColor(24, W100);
      break;
    case CC:
      trellis.setPixelColor(25, W100);
      if (analog_feats && (track > numtracks - 2)) hzV_indicator(track);
      break;
    case NOTE:
      trellis.setPixelColor(26, W100);
      if (analog_feats && (track > numtracks - 2)) hzV_indicator(track);
      break;
    case ARP:
      if (track - 1 >= 4) trellis.setPixelColor(27, W100);
      break;
    case CHORD:
      trellis.setPixelColor(28, W100);
      break;
    default: break;
  }
  trellis.show();
}

void hzV_indicator(uint8_t track){
  if (hzv[(track - 1) - 6] == 1) {
    trellis.setPixelColor(31, seq_dim(track, 100));  //hzv
  } else {
    trellis.setPixelColor(31, W100);  //voct
  };
}

// Initialise Channel Config display...
void init_chan_conf(uint8_t track) {
  for (uint8_t i = 0; i < num_steps; ++i) {
    trellis.setPixelColor(i, 0);
  }
  lastchan = seqr.track_chan[track - 1];
  trellis.setPixelColor(seqr.track_chan[track - 1] - 1, seq_col(track));
  switch (seqr.modes[track - 1]) {
    case TRIGATE:
      trellis.setPixelColor(24, W100);
      break;
    case CC:
      trellis.setPixelColor(25, W100);
      if (analog_feats && (track > numtracks - 2)) hzV_indicator(track);
      break;
    case NOTE:
      trellis.setPixelColor(26, W100);
      if (analog_feats && (track > numtracks - 2)) hzV_indicator(track);
      break;
    case ARP:
      if (track - 1 >= 4) trellis.setPixelColor(27, W100);
      break;
    case CHORD:
      if (track - 1 >= 4) trellis.setPixelColor(28, W100);
      break;
    default: break;
  }
  trellis.show();
}

// Show "Are you sure, Y/N" display...
void sure_pane() {
  for (uint8_t i = 0; i < t_size; ++i) {
    trellis.setPixelColor(i, i == 26 ? GREEN 
                                     : i == 29 ? RED
                                                : 0);
  }
  sure = 1;
  trellis.setPixelColor(60, R40);
  trellis.show();
}

// Show via button color that we're writing to flash...
void toggle_write() {
  write = write == 0 ? 1 : 0;
  trellis.setPixelColor(59, write == 0 ? R40 : R127);
  trellis.show();
}

// Set Brightness of Neotrellis...
void set_brightness() {
  for (uint8_t x = 0; x < X_DIM; ++x) {
    for (uint8_t y = 0; y < Y_DIM; ++y) {
      t_array[y][x].pixels.setBrightness(brightness);
    }
  }
  strip.setBrightness(brightness * 2);
  trellis.show();
  strip.show();
}

// Initialise Neotrellis interactions and static control rows...
void init_interface() {
  if (m4init == 0) {
    for (int y = 0; y < Y_DIM; y++) {
      for (int x = 0; x < X_DIM; x++) {
        trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_RISING, true);
        //trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_FALLING, true);
        trellis.registerCallback(x, y, onKey);
      }
    }
    m4init = 1;
  }
  set_brightness();

  //Seq 1 > 8 & Mutes 1 > 8
  for (uint8_t b = 0; b < numtracks; ++b) {
    trellis.setPixelColor(row1 + b, sel_track == b + 1 ? seq_col(sel_track) : seq_dim(b + 1, 40));
    trellis.setPixelColor(row2 + b, seqr.mutes[b] == 1 ? R80 : G40);
  }

  //Panes
  trellis.setPixelColor(row3, patedit == 1 ? R127 : R40);
  trellis.setPixelColor(row3 + 1, veledit == 1 ? Y127 : Y40);
  trellis.setPixelColor(row3 + 2, probedit == 1 ? P127 : P40);
  trellis.setPixelColor(row3 + 3, gateedit == 1 ? B127 : B40);
  //Globals
  trellis.setPixelColor(row3 + 4, shifted == 1 ? W100 : PK40);
  if (chanedit == 0) {
    switch (transpose) {
      case 0:
        trellis.setPixelColor(row3 + 5, B40);
        break;
      case 12:
        trellis.setPixelColor(row3 + 5, B80);
        break;
      case 24:
        trellis.setPixelColor(row3 + 5, B127);
        break;
      default:
        break;
    }
  } else {
    trellis.setPixelColor(row3 + 5, R127);
  }
  trellis.setPixelColor(row3 + 6, G40);
  switch (cfg.step_size) {
    case SIXTEENTH_NOTE:
      trellis.setPixelColor(row3 + 7, O127);
      break;
    case QUARTER_NOTE:
      trellis.setPixelColor(row3 + 7, O40);
      break;
    case EIGHTH_NOTE:
      trellis.setPixelColor(row3 + 7, O80);
      break;
    default:
      break;
  }
  trellis.setPixelColor(row3 + 7, O127);
  //Control Row
  trellis.setPixelColor(row4, GREEN);
  trellis.setPixelColor(row4 + 1, RED);
  trellis.setPixelColor(row4 + 2, ORANGE);
  trellis.setPixelColor(row4 + 3, B80);
  trellis.setPixelColor(row4 + 4, C40);
  trellis.setPixelColor(row4 + 5, YELLOW);
  trellis.setPixelColor(row4 + 6, RED);
  trellis.setPixelColor(row4 + 7, GREEN);

  trellis.show();
}

#include "saveload.h"  /// FIXME:

// in vast need of improvements for efficiency:
TrellisCallback onKey(keyEvent evt) {
  auto const now = millis();
  auto const keyId = evt.bit.NUM;
  uint8_t trk_arr = sel_track - 1;
  if (marci_debug) { Serial.println(keyId); }
  switch (evt.bit.EDGE) {
    case SEESAW_KEYPAD_EDGE_RISING:
      uint32_t col;
      if (sure == 1) {  // SURE Y/N STEP EDIT
        if (keyId == 26) {
          sure = 1;
          trellis.setPixelColor(60, R127);
          trellis.setPixelColor(29, 0);
          pattern_reset();
        } else if (keyId == 29) {
          sure = 0;
          shifted = 0;
          init_interface();
          show_sequence(sel_track);
        } else {
          break;
        }
      } else if (divedit == 1) {  // TRACK CLOCK DIVIDER STEP EDIT
        if (keyId < (num_steps)) {
          seqr.divs[trk_arr] = keyId;
          seqr.divcounts[trk_arr] = -1;
          show_divisions();
        } else if (keyId == 52) {
          divedit = 0;
          shifted = 0;
          trellis.setPixelColor(52, PK40);
          trellis.setPixelColor(53, B40);
          show_sequence(sel_track);
        } else if (keyId == 53) {
          divedit = 0;
          trellis.setPixelColor(53, B40);
          show_sequence(sel_track);
        } else if (keyId == 58) {
          seqr.reset();
        } else if (keyId < 40 && keyId >= num_steps) {
          divedit = 1;
          lastsel = sel_track;
          sel_track = keyId - (num_steps - 1);
          toggle_selected(keyId);
          show_divisions();
        }
      } else if (presetmode == 1 && (keyId < num_steps || keyId > 39 && keyId < 56)) {  // PRESET STEP EDIT
        if (keyId < (numpresets)) {
          for (uint8_t i = 0; i < numpresets; ++i) {
            trellis.setPixelColor(i, 0);
          }
          trellis.setPixelColor(seqr.presets[trk_arr], 0);
          trellis.setPixelColor(keyId, W100);
          seqr.presets[trk_arr] = keyId;
          // seqr.reset();
        } else if (keyId > ((numpresets - 1)) && keyId < (numpresets * 2)) {
          trellis.setPixelColor(seqr.presets[trk_arr], 0);
          trellis.setPixelColor(keyId - (X_DIM * 2), W100);
          for (uint8_t i = 0; i < numpresets; ++i) {
            seqr.presets[i] = (keyId - (X_DIM * 2));
            // seqr.reset();
          }
        } else if (keyId > 39 && keyId < 48) {
          if (seqr.mutes[keyId - 40] == 0) {
            seqr.mutes[keyId - 40] = 1;
            trellis.setPixelColor(keyId, R80);
          } else {
            seqr.mutes[keyId - 40] = 0;
            trellis.setPixelColor(keyId, G80);
          }
        }
        show_presets();
      } else if (chanedit == 1 && keyId < num_steps) {  // CHANNEL STEP EDIT
        if (keyId < 16) {
          uint8_t chan = keyId + 1;
          seqr.track_chan[trk_arr] = chan;
          midichan[lastchan] = -1;
          midichan[chan] = trk_arr + 1;
          for (uint8_t i = 0; i < 16; ++i) {
            trellis.setPixelColor(i, 0);
          }
          trellis.setPixelColor(keyId, seq_col(sel_track));
          if (!seqr.playing) trellis.show();
        } else {
          switch (keyId) {
            case 24:
              seqr.modes[trk_arr] = TRIGATE;
              break;
            case 25:
              seqr.modes[trk_arr] = CC;
              break;
            case 26:
              seqr.modes[trk_arr] = NOTE;
              break;
            case 27:
              if (trk_arr >= 4) {
                seqr.modes[sel_track - 1] = ARP;
              }
              break;
            case 28:
              //seqr.modes[sel_track-1] = CHORD;
              break;
            case 31:
              if ((seqr.modes[trk_arr] == CC || seqr.modes[trk_arr] == NOTE) && (trk_arr > 5) && analog_feats) {
                hzv[(trk_arr)-6] = hzv[(trk_arr)-6] == 0 ? 1 : 0;
              }
              break;
            default: break;
          }
          mode_leds(sel_track);
        }
      } else if (chanedit == 1 && ((keyId > (num_steps - 1) + X_DIM && keyId < (num_steps) + X_DIM + 12) || (keyId > (num_steps) + X_DIM + 13 && keyId < (t_size - 2)))) {
        // do nothin'
      } else if (lenedit == 1 && keyId < num_steps) {  // LENGTH EDIT
        if (keyId + 1 >= seqr.offsets[trk_arr]) {
          seqr.lengths[trk_arr] = keyId + 1;
          //length = keyId + 1;
          lenedit = 0;
          trellis.setPixelColor(keyId, C127);
          trellis.setPixelColor(54, G40);
          configure_sequencer();
        }
      } else if (offedit == 1 && keyId < num_steps) {  // OFFSET EDIT
        if (keyId + 1 <= seqr.lengths[trk_arr]) {
          seqr.offsets[trk_arr] = keyId + 1;
          //length = keyId + 1;
          offedit = 0;
          trellis.setPixelColor(keyId, B127);
          trellis.setPixelColor(54, B40);
          configure_sequencer();
        }
      } else if (gateedit == 1 && keyId < num_steps) {  // GATE STEP EDIT
        uint8_t gateId = trk_arr;
        if (seqr.gates[seqr.presets[trk_arr]][gateId][keyId] >= 15) {
          seqr.gates[seqr.presets[trk_arr]][gateId][keyId] = 0;
        }
        seqr.gates[seqr.presets[trk_arr]][gateId][keyId] += 3;
        set_gate(gateId, keyId, seq_col(sel_track));
      } else if (probedit == 1 && keyId < num_steps) {  // PROBABILITY STEP EDIT
        if (seqr.probs[seqr.presets[trk_arr]][trk_arr][keyId] == 10) {
          seqr.probs[seqr.presets[trk_arr]][trk_arr][keyId] = 0;
        }
        seqr.probs[seqr.presets[trk_arr]][trk_arr][keyId] += 1;
        col = seqr.probs[seqr.presets[trk_arr]][trk_arr][keyId] == 10 ? seq_col(sel_track) : Wheel(seqr.probs[seqr.presets[trk_arr]][trk_arr][keyId] * 10);
        trellis.setPixelColor(keyId, col);
      } else if (notesedit == 1 & keyId < num_steps) {  // NOTES STEP EDIT
        if (seqr.modes[trk_arr] == CC || seqr.modes[trk_arr] == NOTE) {
          uint8_t prev_selstep = selstep;
          selstep = keyId;
          trellis.setPixelColor(prev_selstep, Wheel(seqr.notes[seqr.presets[trk_arr]][trk_arr][prev_selstep]));
          trellis.setPixelColor(selstep, W100);
        }
      } else if (veledit == 1 & keyId < num_steps) {  // VELOCITY STEP EDIT
        if (seqr.modes[trk_arr] == CC || seqr.modes[trk_arr] == NOTE) {
          uint8_t prev_selstep = selstep;
          selstep = keyId;
          trellis.setPixelColor(prev_selstep, Wheel(seqr.vels[seqr.presets[trk_arr]][trk_arr][prev_selstep]));
          trellis.setPixelColor(selstep, W100);
        } else {
          switch (sel_track) {
            case 0:
              break;
            case 1:
              switch (seqr.vels[seqr.presets[trk_arr]][trk_arr][keyId]) {
                case 127:
                  trellis.setPixelColor(keyId, R40);
                  seqr.vels[seqr.presets[trk_arr]][trk_arr][keyId] = 40;
                  break;
                case 80:
                  trellis.setPixelColor(keyId, R127);
                  seqr.vels[seqr.presets[trk_arr]][trk_arr][keyId] = 127;
                  break;
                case 40:
                  trellis.setPixelColor(keyId, R80);
                  seqr.vels[seqr.presets[trk_arr]][trk_arr][keyId] = 80;
                  break;
                default:
                  break;
              }
              break;
            case 2:
              switch (seqr.vels[seqr.presets[trk_arr]][trk_arr][keyId]) {
                case 127:
                  trellis.setPixelColor(keyId, O40);
                  seqr.vels[seqr.presets[trk_arr]][trk_arr][keyId] = 40;
                  break;
                case 80:
                  trellis.setPixelColor(keyId, O127);
                  seqr.vels[seqr.presets[trk_arr]][trk_arr][keyId] = 127;
                  break;
                case 40:
                  trellis.setPixelColor(keyId, O80);
                  seqr.vels[seqr.presets[trk_arr]][trk_arr][keyId] = 80;
                  break;
                default:
                  break;
              }
              break;
            case 3:
              switch (seqr.vels[seqr.presets[trk_arr]][trk_arr][keyId]) {
                case 127:
                  trellis.setPixelColor(keyId, Y40);
                  seqr.vels[seqr.presets[trk_arr]][trk_arr][keyId] = 40;
                  break;
                case 80:
                  trellis.setPixelColor(keyId, Y127);
                  seqr.vels[seqr.presets[trk_arr]][trk_arr][keyId] = 127;
                  break;
                case 40:
                  trellis.setPixelColor(keyId, Y80);
                  seqr.vels[seqr.presets[trk_arr]][trk_arr][keyId] = 80;
                  break;
                default:
                  break;
              }
              break;
            case 4:
              switch (seqr.vels[seqr.presets[trk_arr]][trk_arr][keyId]) {
                case 127:
                  trellis.setPixelColor(keyId, G40);
                  seqr.vels[seqr.presets[trk_arr]][trk_arr][keyId] = 40;
                  break;
                case 80:
                  trellis.setPixelColor(keyId, G127);
                  seqr.vels[seqr.presets[trk_arr]][trk_arr][keyId] = 127;
                  break;
                case 40:
                  trellis.setPixelColor(keyId, G80);
                  seqr.vels[seqr.presets[trk_arr]][trk_arr][keyId] = 80;
                  break;
                default:
                  break;
              }
              break;
            case 5:
              switch (seqr.vels[seqr.presets[trk_arr]][trk_arr][keyId]) {
                case 127:
                  trellis.setPixelColor(keyId, C40);
                  seqr.vels[seqr.presets[trk_arr]][trk_arr][keyId] = 40;
                  break;
                case 80:
                  trellis.setPixelColor(keyId, C127);
                  seqr.vels[seqr.presets[trk_arr]][trk_arr][keyId] = 127;
                  break;
                case 40:
                  trellis.setPixelColor(keyId, C80);
                  seqr.vels[seqr.presets[trk_arr]][trk_arr][keyId] = 80;
                  break;
                default:
                  break;
              }
              break;
            case 6:
              switch (seqr.vels[seqr.presets[trk_arr]][trk_arr][keyId]) {
                case 127:
                  trellis.setPixelColor(keyId, B40);
                  seqr.vels[seqr.presets[trk_arr]][trk_arr][keyId] = 40;
                  break;
                case 80:
                  trellis.setPixelColor(keyId, B127);
                  seqr.vels[seqr.presets[trk_arr]][trk_arr][keyId] = 127;
                  break;
                case 40:
                  trellis.setPixelColor(keyId, B80);
                  seqr.vels[seqr.presets[trk_arr]][trk_arr][keyId] = 80;
                  break;
                default:
                  break;
              }
              break;
            case 7:
              switch (seqr.vels[seqr.presets[trk_arr]][trk_arr][keyId]) {
                case 127:
                  trellis.setPixelColor(keyId, P40);
                  seqr.vels[seqr.presets[trk_arr]][trk_arr][keyId] = 40;
                  break;
                case 80:
                  trellis.setPixelColor(keyId, P127);
                  seqr.vels[seqr.presets[trk_arr]][trk_arr][keyId] = 127;
                  break;
                case 40:
                  trellis.setPixelColor(keyId, P80);
                  seqr.vels[seqr.presets[trk_arr]][trk_arr][keyId] = 80;
                  break;
                default:
                  break;
              }
              break;
            case 8:
              switch (seqr.vels[seqr.presets[trk_arr]][trk_arr][keyId]) {
                case 127:
                  trellis.setPixelColor(keyId, PK40);
                  seqr.vels[seqr.presets[trk_arr]][trk_arr][keyId] = 40;
                  break;
                case 80:
                  trellis.setPixelColor(keyId, PK127);
                  seqr.vels[seqr.presets[trk_arr]][trk_arr][keyId] = 127;
                  break;
                case 40:
                  trellis.setPixelColor(keyId, PK80);
                  seqr.vels[seqr.presets[trk_arr]][trk_arr][keyId] = 80;
                  break;
                default:
                  break;
              }
              break;
            default:
              break;
          }
        }
      } else if (keyId < num_steps) {  // STEP EDIT
        uint8_t trk_arr = sel_track - 1;
        col = W10;
        switch (seqr.seqs[seqr.presets[trk_arr]][trk_arr][keyId]) {
          case 1:
            seqr.seqs[seqr.presets[trk_arr]][trk_arr][keyId] = 0;
            break;
          case 0:
            col = seq_col(sel_track);
            seqr.seqs[seqr.presets[trk_arr]][trk_arr][keyId] = 1;
            break;
        }
        trellis.setPixelColor(keyId, col);
      } else if (keyId < 40) {  // SELECT TRACK 1 - 8
        lastsel = sel_track;
        sel_track = keyId - (num_steps - 1);
        toggle_selected(keyId);
        if (presetmode == 1) {
          show_presets();
        } else if (gateedit == 1) {
          patedit = 0;
          probedit = 0;
          veledit = 0;
          show_gates(sel_track);
        } else if (probedit == 1) {
          gateedit = 0;
          patedit = 0;
          veledit = 0;
          show_probabilities(sel_track);
        } else if (veledit == 1) {
          patedit = 0;
          probedit = 0;
          gateedit = 0;
          show_accents(sel_track);
        } else if (chanedit == 1) {
          patedit = 0;
          probedit = 0;
          veledit = 0;
          gateedit = 0;
          init_chan_conf(sel_track);
        } else {
          patedit = 1;
          probedit = 0;
          veledit = 0;
          gateedit = 0;
          show_sequence(sel_track);
        }
      } else {
        switch (keyId) {
          case 40:  // MUTE 1
            if (seqr.mutes[0] == 0) {
              seqr.mutes[0] = 1;
              trellis.setPixelColor(40, R80);
            } else {
              seqr.mutes[0] = 0;
              trellis.setPixelColor(40, G40);
            }
            break;
          case 41:  // MUTE 2
            if (seqr.mutes[1] == 0) {
              seqr.mutes[1] = 1;
              trellis.setPixelColor(41, R80);
            } else {
              seqr.mutes[1] = 0;
              trellis.setPixelColor(41, G40);
            }
            break;
          case 42:  // MUTE 3
            if (seqr.mutes[2] == 0) {
              seqr.mutes[2] = 1;
              trellis.setPixelColor(42, R80);
            } else {
              seqr.mutes[2] = 0;
              trellis.setPixelColor(42, G40);
            }
            break;
          case 43:  // MUTE 4
            if (seqr.mutes[3] == 0) {
              seqr.mutes[3] = 1;
              trellis.setPixelColor(43, R80);
            } else {
              seqr.mutes[3] = 0;
              trellis.setPixelColor(43, G40);
            }
            break;
          case 44:  // MUTE 5
            if (seqr.mutes[4] == 0) {
              seqr.mutes[4] = 1;
              trellis.setPixelColor(44, R80);
            } else {
              seqr.mutes[4] = 0;
              trellis.setPixelColor(44, G40);
            }
            break;
          case 45:  // MUTE 6
            if (seqr.mutes[5] == 0) {
              seqr.mutes[5] = 1;
              trellis.setPixelColor(45, R80);
            } else {
              seqr.mutes[5] = 0;
              trellis.setPixelColor(45, G40);
            }
            break;
          case 46:  // MUTE 7
            if (seqr.mutes[6] == 0) {
              seqr.mutes[6] = 1;
              trellis.setPixelColor(46, R80);
            } else {
              seqr.mutes[6] = 0;
              trellis.setPixelColor(46, G40);
            }
            break;
          case 47:  // MUTE 8
            if (seqr.mutes[7] == 0) {
              seqr.mutes[7] = 1;
              trellis.setPixelColor(47, R80);
            } else {
              seqr.mutes[7] = 0;
              trellis.setPixelColor(47, G40);
            }
            break;
          case 48:  // SHOW PATTERN
            uimode = _PATTERN;
            if (patedit == 0) {
              patedit = 1;
              veledit = 0;
              probedit = 0;
              gateedit = 0;
              trellis.setPixelColor(48, R127);
              trellis.setPixelColor(49, Y40);
              trellis.setPixelColor(50, P40);
              trellis.setPixelColor(51, B40);
              show_sequence(sel_track);
            }
            break;
          case 49:  // SHOW VELOCITIES
            uimode = _VELOCITY;
            if (veledit == 0) {
              veledit = 1;
              patedit = 0;
              probedit = 0;
              gateedit = 0;
              trellis.setPixelColor(48, R40);
              trellis.setPixelColor(49, Y127);
              trellis.setPixelColor(50, P40);
              trellis.setPixelColor(51, B40);
              show_accents(sel_track);
            }
            break;
          case 50:  // SHOW PROBABILITY
            uimode = _PROBABILITY;
            if (probedit == 0) {
              probedit = 1;
              patedit = 0;
              veledit = 0;
              gateedit = 0;
              trellis.setPixelColor(48, R40);
              trellis.setPixelColor(49, Y40);
              trellis.setPixelColor(50, P127);
              trellis.setPixelColor(51, B40);
              show_probabilities(sel_track);
            }
            break;
          case 51:  // SHOW GATES
            uimode = _GATE;
            if (gateedit == 0) {
              gateedit = 1;
              probedit = 0;
              patedit = 0;
              veledit = 0;
              trellis.setPixelColor(48, R40);
              trellis.setPixelColor(49, Y40);
              trellis.setPixelColor(50, P40);
              trellis.setPixelColor(51, B127);
              show_gates(sel_track);
            }
            break;
          case 52:  // SHIFT (^)
            if (shifted == 0) {
              shifted = 1;
              lenedit = 0;
              trellis.setPixelColor(54, G40);
              trellis.setPixelColor(52, W100);
            } else {
              shifted = 0;
              lenedit = 0;
              trellis.setPixelColor(54, G40);
              if (chanedit == 1) {
                chanedit = 0;
                patedit = 1;
                veledit = 0;
                probedit = 0;
                gateedit = 0;
                chanedit = 0;
                trellis.setPixelColor(48, R127);
                trellis.setPixelColor(49, Y40);
                trellis.setPixelColor(50, P40);
                trellis.setPixelColor(51, B40);
                trellis.setPixelColor(52, PK40);
                transpose_led();
                show_sequence(sel_track);
              }
              if (swingedit = 1) {
                swingedit = 0;
                switch (cfg.step_size) {
                  case SIXTEENTH_NOTE:
                    trellis.setPixelColor(55, O40);
                    break;
                  case QUARTER_NOTE:
                    trellis.setPixelColor(55, O80);
                    break;
                  case EIGHTH_NOTE:
                    trellis.setPixelColor(55, O127);
                    break;
                  default:
                    break;
                }
              }
              trellis.setPixelColor(52, PK40);
              transpose_led();
            }
            break;
          case 53:  // TRANSPOSE | ^TRACK DIVISION (RUNNING) | ^CHANNEL CONFIG (STOPPED)
            if (!seqr.playing) {
              if (shifted == 0) {
                switch (transpose) {
                  case 0:
                    transpose = 12;
                    trellis.setPixelColor(53, B80);
                    break;
                  case 12:
                    transpose = 24;
                    trellis.setPixelColor(53, B127);
                    break;
                  case 24:
                    transpose = 0;
                    trellis.setPixelColor(53, B40);
                    break;
                  default:
                    break;
                }
              } else {
                if (chanedit == 0) {
                  chanedit = 1;
                  trellis.setPixelColor(53, R127);
                  init_chan_conf(sel_track);
                } else {
                  transpose_led();
                  patedit = 1;
                  veledit = 0;
                  probedit = 0;
                  gateedit = 0;
                  chanedit = 0;
                  trellis.setPixelColor(48, R127);
                  trellis.setPixelColor(49, Y40);
                  trellis.setPixelColor(50, P40);
                  trellis.setPixelColor(51, B40);
                  show_sequence(sel_track);
                }
              }
              configure_sequencer();
            } else {
              if (shifted == 1) {
                divedit = divedit == 0 ? 1 : 0;
                if (divedit == 1) {
                  show_divisions();
                }
              }
            }
            break;
          case 54:  // LENGTH | ^OFFSET
            if (shifted == 0) {
              if (lenedit == 0) {
                uimode = _LENGTH;
                offedit = 0;
                lenedit = 1;
                trellis.setPixelColor(54, G127);
              } else {
                lenedit = 0;
                trellis.setPixelColor(54, G40);
              }
            } else {
              if (offedit == 0) {
                uimode = _OFFSET;
                lenedit = 0;
                offedit = 1;
                trellis.setPixelColor(54, B127);
              } else {
                offedit = 0;
                trellis.setPixelColor(54, B40);
              }
            }
            break;
          case 55:  // BASE CLOCK DIVIDER | ^SWING
            if (shifted == 0) {
              switch (cfg.step_size) {
                case SIXTEENTH_NOTE:
                  trellis.setPixelColor(55, O40);
                  cfg.step_size = QUARTER_NOTE;
                  break;
                case QUARTER_NOTE:
                  trellis.setPixelColor(55, O80);
                  cfg.step_size = EIGHTH_NOTE;
                  break;
                case EIGHTH_NOTE:
                  trellis.setPixelColor(55, O127);
                  cfg.step_size = SIXTEENTH_NOTE;
                  break;
                default:
                  break;
              }
              configure_sequencer();
            } else {
              if (swingedit == 0) {
                uimode = _SWING;
                trellis.setPixelColor(55, R127);
                swingedit = 1;
              } else {
                swingedit = 0;
                switch (cfg.step_size) {
                  case SIXTEENTH_NOTE:
                    trellis.setPixelColor(55, O127);
                    break;
                  case QUARTER_NOTE:
                    trellis.setPixelColor(55, O40);
                    break;
                  case EIGHTH_NOTE:
                    trellis.setPixelColor(55, O80);
                    break;
                  default:
                    break;
                }
              }
            }
            break;
          case 56:  // PLAY/STOP
            if (chanedit == 0) { seqr.toggle_play_stop(); }
            break;
          case 57:  // STOP
            if (chanedit == 0) { seqr.stop(); }
            break;
          case 58:  // RESET
            if (chanedit == 0) { 
              if (!seqr.playing) {
                seqr.reset();
              } else { 
                seqr.resetflag = 1; 
              }
            }
            break;
          case 59:  // WRITE
            if (chanedit == 0) {
              trellis.setPixelColor(59, R127);
              sequences_write();
            }
            break;
          case 60:  // PRESETS | ^FACT RESET
            if (shifted == 1) {
              if (sure == 0) {
                uimode = _CONFIRM;
                trellis.setPixelColor(60, R127);
                sure_pane();
              }
            } else {
              if (presetmode == 0) {
                uimode = _PRESET;
                presetmode = 1;
                trellis.setPixelColor(60, C127);
                show_presets();
              } else {
                presetmode = 0;
                trellis.setPixelColor(60, C40);
                show_sequence(sel_track);
              }
            }
            break;
          case 61:  // CLOCK ON/OFF
            if (swingedit == 1) {
              seqr.swing = 0;
            } else if (veledit == 1) {
              for (uint8_t i = 0; i < num_steps; ++i) {
                seqr.vels[seqr.presets[trk_arr]][trk_arr][i] = 72;
              }
            } else if (notesedit == 1) {
              for (uint8_t i = 0; i < num_steps; ++i) {
                seqr.notes[seqr.presets[trk_arr]][trk_arr][i] = 0;
              }
            } else {
              if (cfg.midi_send_clock == true) {
                cfg.midi_send_clock = false;
                trellis.setPixelColor(61, Y40);
              } else {
                cfg.midi_send_clock = true;
                trellis.setPixelColor(61, Y127);
              }
              configure_sequencer();
            }
            break;
          case 62:  // PARAM -
            if (seqr.modes[trk_arr] == ARP && patedit == 1) {
              uint8_t arp_id = trk_arr - numarps;
              if (shifted == 0) {
                arp_patterns[arp_id] = arp_patterns[arp_id] > 1 ? arp_patterns[arp_id] - 1 : 7;
              } else if (shifted == 1) {
                arp_octaves[arp_id] = arp_octaves[arp_id] > 1 ? arp_octaves[arp_id] - 1 : 4;
              }
            } else if (chanedit == 1) {
              brightness = brightness > 15 ? brightness - 10 : 5;
              init_interface();
              init_chan_conf(sel_track);
            } else if (gateedit == 1 && swingedit == 0) {
              for (uint8_t i = 0; i < num_steps; ++i) {
                seqr.gates[seqr.presets[trk_arr]][trk_arr][i] = seqr.gates[seqr.presets[trk_arr]][trk_arr][i] - 1 > 1 ? seqr.gates[seqr.presets[trk_arr]][trk_arr][i] - 1 : 1;
                if (!seqr.playing) set_gate(trk_arr, i, seq_col(sel_track));
              }
              if (!seqr.playing) { trellis.show(); }
            } else if (shifted == 1 && swingedit == 0) {
              seqr.track_notes[trk_arr] = seqr.track_notes[trk_arr] > 0 ? seqr.track_notes[trk_arr] - 1 : 127;
            } else if (shifted == 1 && swingedit == 1) {
              seqr.swing = seqr.swing > 0 ? seqr.swing - 1 : 0;
            } else if (probedit == 1) {
              for (uint8_t i = 0; i < num_steps; ++i) {
                seqr.probs[seqr.presets[trk_arr]][trk_arr][i] = seqr.probs[seqr.presets[trk_arr]][trk_arr][i] > 1 ? seqr.probs[seqr.presets[trk_arr]][trk_arr][i] - 1 : 1;
              }
            } else if (veledit == 1) {
              if (seqr.modes[trk_arr] == CC || seqr.modes[trk_arr] == NOTE) {
                if (shifted == 1) {
                  seqr.vels[seqr.presets[trk_arr]][trk_arr][selstep] = seqr.vels[seqr.presets[trk_arr]][trk_arr][selstep] > 5 ? seqr.vels[seqr.presets[trk_arr]][trk_arr][selstep] - 5 : 0;
                } else {
                  for (uint8_t i = 0; i < num_steps; ++i) {
                    seqr.vels[seqr.presets[trk_arr]][trk_arr][i] = seqr.vels[seqr.presets[trk_arr]][trk_arr][i] > 5 ? seqr.vels[seqr.presets[trk_arr]][trk_arr][i] - 5 : 0;
                  }
                }
              } else {
                for (uint8_t i = 0; i < num_steps; ++i) {
                  seqr.vels[seqr.presets[trk_arr]][trk_arr][i] = seqr.vels[seqr.presets[trk_arr]][trk_arr][i] > 5 ? seqr.vels[seqr.presets[trk_arr]][trk_arr][i] - 5 : 0;
                }
              }
            } else if (notesedit == 1) {
              if (seqr.modes[trk_arr] == CC || seqr.modes[trk_arr] == NOTE) {
                seqr.notes[seqr.presets[trk_arr]][trk_arr][selstep] = seqr.notes[seqr.presets[trk_arr]][trk_arr][selstep] > 0 ? seqr.notes[seqr.presets[trk_arr]][trk_arr][selstep] - 1 : 0;
              }
            } else {
              tempo = tempo - 1;
              configure_sequencer();
            }
            break;
          case 63:  // PARAM +
            if (seqr.modes[trk_arr] == ARP && patedit == 1) {
              uint8_t arp_id = trk_arr - numarps;
              if (shifted == 0) {
                arp_patterns[arp_id] = arp_patterns[arp_id] < 7 ? arp_patterns[arp_id] + 1 : 1;
              } else if (shifted == 1) {
                arp_octaves[arp_id] = arp_octaves[arp_id] < 4 ? arp_octaves[arp_id] + 1 : 1;
              }
            } else if (chanedit == 1) {
              brightness = brightness < 117 ? brightness + 10 : 127;
              init_interface();
              init_chan_conf(sel_track);
            } else if (gateedit == 1 && swingedit == 0) {
              for (uint8_t i = 0; i < num_steps; ++i) {
                seqr.gates[seqr.presets[trk_arr]][trk_arr][i] = seqr.gates[seqr.presets[trk_arr]][trk_arr][i] + 1 < 15 ? seqr.gates[seqr.presets[trk_arr]][trk_arr][i] + 1 : 15;
                if (!seqr.playing) set_gate(trk_arr, i, seq_col(sel_track));
              }
              if (!seqr.playing) { trellis.show(); }
            } else if (shifted == 1 && swingedit == 0) {
              seqr.track_notes[trk_arr] = seqr.track_notes[trk_arr] < 127 ? seqr.track_notes[trk_arr] + 1 : 1;
            } else if (shifted == 1 && swingedit == 1) {
              seqr.swing = seqr.swing < 30 ? seqr.swing + 1 : 30;
            } else if (probedit == 1) {
              for (uint8_t i = 0; i < num_steps; ++i) {
                seqr.probs[seqr.presets[trk_arr]][trk_arr][i] = seqr.probs[seqr.presets[trk_arr]][trk_arr][i] < 10 ? seqr.probs[seqr.presets[trk_arr]][trk_arr][i] + 1 : 10;
              }
            } else if (veledit == 1) {
              if (seqr.modes[trk_arr] == CC || seqr.modes[trk_arr] == NOTE) {
                if (shifted == 1) {
                  seqr.vels[seqr.presets[trk_arr]][trk_arr][selstep] = seqr.vels[seqr.presets[trk_arr]][trk_arr][selstep] < 122 ? seqr.vels[seqr.presets[trk_arr]][trk_arr][selstep] + 5 : 127;
                } else {
                  for (uint8_t i = 0; i < num_steps; ++i) {
                    seqr.vels[seqr.presets[trk_arr]][trk_arr][i] = seqr.vels[seqr.presets[trk_arr]][trk_arr][i] < 122 ? seqr.vels[seqr.presets[trk_arr]][trk_arr][i] + 5 : 127;
                  }
                }
              } else {
                for (uint8_t i = 0; i < num_steps; ++i) {
                  seqr.vels[seqr.presets[trk_arr]][trk_arr][i] = seqr.vels[seqr.presets[trk_arr]][trk_arr][i] < 122 ? seqr.vels[seqr.presets[trk_arr]][trk_arr][i] + 5 : 127;
                }
              }
            } else if (notesedit == 1) {
              if (seqr.modes[trk_arr] == CC || seqr.modes[trk_arr] == NOTE) {
                seqr.notes[seqr.presets[trk_arr]][trk_arr][selstep] = seqr.notes[seqr.presets[trk_arr]][trk_arr][selstep] < 127 ? seqr.notes[seqr.presets[trk_arr]][trk_arr][selstep] + 1 : 127;
              }
            } else {
              tempo = tempo + 1;
              configure_sequencer();
            }
            break;
          default:
            break;
        }
        if (!seqr.playing) trellis.show();
      }
      break;
    case SEESAW_KEYPAD_EDGE_FALLING:
      break;
  }
  return nullptr;
}

void configure_sequencer() {
  if (marci_debug) Serial.println(F("Configuring sequencer"));
  if (m4init == 0) {
    seqr.on_func = send_note_on;
    seqr.off_func = send_note_off;
    seqr.clk_func = send_clock_start_stop;
    seqr.pos_func = send_song_pos;
    seqr.cc_func = send_cc;
    seqr.disp_func = update_display;
    seqr.reset_func = reset_display;
    if (analog_feats) {
      seqr.gate_func = analog_gate;
      seqr.cv_func = analog_cv;
      seqr.analog_io = analog_feats;
    }
  }
  seqr.ticks_per_step = cfg.step_size;
  seqr.set_tempo(tempo);
  seqr.length = length;
  seqr.transpose = transpose;
  seqr.send_clock = cfg.midi_send_clock;
};

void init_flash() {
  if (!flash.begin()) {
    Serial.println(F("Error, failed to initialize flash chip!"));
    while (1) {
      yield();
    }
  }
  Serial.print(F("Flash chip JEDEC ID: 0x"));
  Serial.println(flash.getJEDECID(), HEX);

  if (!fatfs.begin(&flash)) {
    // I'm ASSUMING all Feather M4 Express come with CircuitPython preloaded, therefore flash should already be formatted, but someone correct me if I'm wrong!
    Serial.println(F("Error, failed to mount newly formatted filesystem!"));
    Serial.println(
      F("Was the flash chip formatted with the SdFat_format example?"));
    while (1) {
      yield();
    }
  }
  Serial.println(F("Mounted filesystem!"));

  // Make sure required folder exists... if it doesn't, create it
  flash_store();
}

#ifdef __arm__
extern "C" char* sbrk(int incr);
#else
extern char *__brkval;
#endif

int freeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif
}

////////////////////////////
//
// ---  DO ALL THE THINGS
//
void setup() {
  TinyUSBDevice.setManufacturerDescriptor("aPatchworkBoy");
  TinyUSBDevice.setProductDescriptor("M4StepSeq");

  MIDIusb.setHandleNoteOn(handle_midi_in_NoteOn);
  MIDIusb.setHandleNoteOff(handle_midi_in_NoteOff);
  MIDIusb.setHandleControlChange(handle_midi_in_CC);
  MIDIusb.begin(MIDI_CHANNEL_OMNI);
  MIDIusb.turnThruOff();  // turn off echo

  Serial.begin(115200);
  if (marci_debug) { delay(2000); }
  init_flash();
  if (marci_debug) { Serial.println(freeMemory()); }

  // For the sake of sanity and...
  for (uint8_t t = 0; t < numtracks; t++) {
    seqr.divs[t] = 0;
    seqr.lengths[t] = t_size / 2;
    seqr.divcounts[t] = -1;
    seqr.offsets[t] = 0;
    seqr.outcomes[t] = 1;
    seqr.multistepi[t] = -1;
  }
  patedit = patedit == 0 ? 1 : patedit;
  sel_track = sel_track == 0 ? 1 : sel_track;
  lastsel = lastsel == 0 ? 1 : sel_track;

  randomSeed(analogRead(0));  // for probability

  // Load in saved slots...
  sequences_read();
  notes_read();
  velocities_read();
  probabilities_read();
  gates_read();
  settings_read();
  configure_sequencer();

  if (analog_feats) {
    for (uint8_t i = 0; i < sizeof(gatepins); ++i) {
      pinMode(gatepins[i], OUTPUT);  //digtal (gate) out
    }
  }

  if (!trellis.begin()) {
    if (marci_debug) { Serial.println(F("Trellis failed to begin")); }
    while (1) delay(1);
  } else {
    if (marci_debug) { Serial.println(F("Trellis Init...")); }
  }

  if (marci_debug) { Serial.println(F("SerialMIDI Init...")); }
  if (serial_midi) serialmidi.begin();
  if (marci_debug) { Serial.println(F("...done!")); }

  strip.begin();
  strip.setPixelColor(0, seq_col(sel_track));
  strip.show();

  init_interface();

  if (marci_debug) {
    Serial.println(freeMemory());
    Serial.println(F("GO!"));
  }
  seqr.reset();
  show_sequence(sel_track);
}

//
// --- MIDI in/out - ADD NOTHING HERE!
//
void loop() {
  midi_read_and_forward();
  seqr.update();  // will call send_note_{on,off} callbacks
}