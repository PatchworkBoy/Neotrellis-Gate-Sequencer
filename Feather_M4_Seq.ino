/**
 * Feather M4 Seq -- An 8 track 32 step MIDI (& Analog) Gate (& CV) Sequencer for Feather M4 Express & Neotrellis 8x8
 * 04 Nov 2023 - @apatchworkboy / Marci
 * 28 Apr 2023 - original via @todbot / Tod Kurt https://github.com/todbot/picostepseq/
 *
 * Libraries needed (all available via Library Manager):
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
#include <Adafruit_NeoPixel.h>
#include <Adafruit_NeoTrellis.h>
#include <MIDI.h>
#include <ArduinoJson.h>
#include "flash_config.h"

// Serial Console logging...
const bool midi_out_debug = false;
const bool midi_in_debug = false;
const bool marci_debug = false;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(2, 8, NEO_GRB + NEO_KHZ800);

Adafruit_SPIFlash flash(&flashTransport);
FatVolume fatfs;

Adafruit_USBD_MIDI usb_midi;                                  // USB MIDI object
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDIusb);  // USB MIDI

// HOW BIG IS YOUR TRELLIS?
#define Y_DIM 8  //number of rows of key
#define X_DIM 8  //number of columns of keys

Adafruit_NeoTrellis t_array[Y_DIM / 4][X_DIM / 4] = {
  { Adafruit_NeoTrellis(0x30), Adafruit_NeoTrellis(0x2E) },
  { Adafruit_NeoTrellis(0x2F), Adafruit_NeoTrellis(0x31) }
};

Adafruit_MultiTrellis trellis((Adafruit_NeoTrellis*)t_array, Y_DIM / 4, X_DIM / 4);

#define D_FLASH "/M4SEQ32"
#define t_size Y_DIM* X_DIM
#define t_length t_size - 1

// Color defs for Trellis...
#include "color_defs.h"

const uint8_t numseqs = X_DIM;
const uint8_t numsteps = t_size / 2;
const uint8_t numpresets = X_DIM * 2;
uint8_t length = t_size / 2;  // same as numsteps at boot!
int lengths[numseqs];         // same as numsteps at boot!
int offsets[numseqs];

uint8_t brightness = 50;

// typedefs
typedef enum {
  TRIGATE = 0,
  CC = 1,
  NOTE = 2,
  CHORD = 3,
  ARP = 4,
} track_type;

typedef enum {
  NONE,
  START,
  STOP,
  CLOCK,
} clock_type_t;

typedef enum {
  QUARTER_NOTE = 24,
  EIGHTH_NOTE = 12,
  SIXTEENTH_NOTE = 6,
} valid_ticks_per_step;

typedef struct {
  int step_size;
  bool midi_send_clock;
  bool midi_forward_usb;
} Config;

uint8_t track_notes[numseqs];  // C2 thru G2
uint8_t ctrl_notes[3];
uint8_t track_chan[numseqs];
uint8_t ctrl_chan = 16;
track_type modes[numseqs] = { TRIGATE };

// Empty patterns...
bool mutes[numseqs];
bool seqs[numpresets][numseqs][numsteps];
int outcomes[numseqs];
int step8i[numseqs];
uint8_t divcounts[numseqs];
uint8_t divs[numpresets][numseqs];
uint8_t gates[numpresets][numseqs][numsteps];
uint8_t notes[numpresets][numseqs][numsteps];
uint8_t presets[numpresets];
uint8_t probs[numpresets][numseqs][numsteps];
uint8_t vels[numpresets][numseqs][numsteps];

const uint16_t dacrange = 4095;  // A0 & A1 = 12bit DAC on Feather M4 Express
uint8_t currsteps[numseqs];
uint8_t laststeps[numseqs];
uint8_t currstep;
uint8_t lastsel;
uint8_t laststep;
uint8_t sel_track;
uint8_t selstep;
uint8_t shifted;  // (SHIFT)
uint8_t swing;
uint8_t transpose;
bool chanedit;
bool divedit;
bool gateedit;
bool lenedit;
bool m4init;
bool notesedit;
bool offedit;
bool patedit;
bool presetmode;
bool probedit;
bool pulse;
bool run;
bool sure;
bool swingedit;
bool veledit;
bool write;
bool hzv[2] = { 0, 0 };
const byte cvpins[2] = { 14, 15 };
const byte gatepins[numseqs] = { 4, 5, 6, 9, 10, 11, 12, 13 };

void update_display() {
  if (presetmode == 1 || chanedit == 1 || sure == 1 || divedit == 1) {
    return;
  }
  uint32_t color = 0;
  uint32_t hit = 0;
  uint8_t trk_arr = sel_track - 1;

  //active step ticker
  if (gateedit == 1) {
    hit = gates[presets[trk_arr]][trk_arr][step8i[trk_arr]] > 0 ? PURPLE : W100;
    color = gates[presets[trk_arr]][trk_arr][laststeps[trk_arr]] < 15 ? Wheel(gates[presets[trk_arr]][trk_arr][laststeps[trk_arr]] * 5) : seq_col(sel_track);
  } else if (probedit == 1) {
    hit = seqs[presets[trk_arr]][trk_arr][step8i[trk_arr]] > 0 ? PURPLE : W100;
    color = probs[presets[trk_arr]][trk_arr][laststeps[trk_arr]] < 10 ? Wheel(probs[presets[trk_arr]][trk_arr][laststeps[trk_arr]] * 10) : seq_col(sel_track);
  } else if (veledit == 1) {
    hit = step8i[trk_arr] != selstep ? seqs[presets[trk_arr]][trk_arr][step8i[trk_arr]] > 0 ? PURPLE : W100 : W100;
    if (modes[trk_arr] == CC || modes[trk_arr] == NOTE) {
      color = Wheel(vels[presets[trk_arr]][trk_arr][laststeps[trk_arr]]);
    } else {
      color = seq_dim(sel_track, vels[presets[trk_arr]][trk_arr][laststeps[trk_arr]]);
    }
  } else if (notesedit == 1) {
    hit = step8i[trk_arr] != selstep ? seqs[presets[trk_arr]][trk_arr][step8i[trk_arr]] > 0 ? PURPLE : W100 : W100;
    if (modes[trk_arr] == CC || modes[trk_arr] == NOTE) {
      color = Wheel(notes[presets[trk_arr]][trk_arr][laststeps[trk_arr]]);
    } else {
      color = seq_dim(sel_track, notes[presets[trk_arr]][trk_arr][laststeps[trk_arr]]);
    }
  } else {
    hit = seqs[presets[trk_arr]][trk_arr][step8i[trk_arr]] > 0 ? PURPLE : W100;
    color = seqs[presets[trk_arr]][trk_arr][laststeps[trk_arr]] > 0 ? seq_col(sel_track) : 0;
  }
  trellis.setPixelColor(step8i[trk_arr], hit);
  strip.setPixelColor(0, seqs[presets[trk_arr]][trk_arr][step8i[trk_arr]] > 0 ? seq_col(sel_track) : (pulse == 1 ? W40 : 0));
  if (modes[trk_arr] == CC && veledit == 1) {
    trellis.setPixelColor(laststeps[trk_arr], laststeps[trk_arr] != selstep ? color : W100);
  } else {
    trellis.setPixelColor(laststeps[trk_arr], color);
  }
  if (offsets[trk_arr] - 1 > 0) trellis.setPixelColor(offsets[trk_arr] - 1, trk_arr > 1 ? R127 : B127);
  if (lengths[trk_arr] < numsteps) trellis.setPixelColor(lengths[trk_arr] - 1, trk_arr != 4 ? C127 : G127);
  trellis.show();
  strip.show();
}

void reset_display() {
  uint32_t hit = 0;
  uint8_t trk_arr = sel_track - 1;
  if (patedit == 1) {
    hit = seqs[presets[trk_arr]][trk_arr][step8i[trk_arr]] > 0 ? seq_col(sel_track) : W10;
  } else if (gateedit == 1) {
    hit = gates[presets[trk_arr]][trk_arr][step8i[trk_arr]] > 0 ? seq_col(sel_track) : W10;
  } else if (veledit == 1) {
    hit = vels[presets[trk_arr]][trk_arr][step8i[trk_arr]] > 0 ? seq_col(sel_track) : W10;
  } else if (probedit == 1) {
    hit = probs[presets[trk_arr]][trk_arr][step8i[trk_arr]] > 0 ? seq_col(sel_track) : W10;
  } else {
    hit = W10;
  }
  trellis.setPixelColor(step8i[trk_arr], hit);
  strip.setPixelColor(0, seq_col(sel_track));
  strip.show();
  trellis.show();
  show_sequence(sel_track);
}

#include "Sequencer.h"
#include "save_locations.h"

Config cfg = {
  .step_size = SIXTEENTH_NOTE,
  .midi_send_clock = true,
  .midi_forward_usb = true
};

// end hardware definitions

float tempo = 120;
StepSequencer seqr;

uint8_t midiclk_cnt = 0;
uint32_t midiclk_last_micros = 0;

//
// -- MIDI sending & receiving functions
//
// callback used by Sequencer to trigger note on
void send_note_on(uint8_t note, uint8_t vel, uint8_t gate, bool on, uint8_t chan) {
  if (on) {
    MIDIusb.sendNoteOn(note, vel, chan);
  }
  if (midi_out_debug) { Serial.printf("noteOn:  %d %d %d %d\n", note, vel, gate, on); }
}

// callback used by Sequencer to trigger note off
void send_note_off(uint8_t note, uint8_t vel, uint8_t gate, bool on, uint8_t chan) {
  if (on) {
    MIDIusb.sendNoteOff(note, vel, chan);
  }
  if (midi_out_debug) { Serial.printf("noteOff: %d %d %d %d\n", note, vel, gate, on); }
}

// callback used by Sequencer to send controlchange messages
void send_cc(uint8_t cc, uint8_t val, bool on, uint8_t chan) {
  if (on) {
    MIDIusb.sendControlChange(cc, val, chan);
  }
  if (midi_out_debug) { Serial.printf("controlChange: %d %d %d %d\n", cc, val, on, chan); }
}

// callback used by Sequencer to send midi clock when internally triggered
void send_clock_start_stop(clock_type_t type) {
  if (type == START) {
    MIDIusb.sendStart();
  } else if (type == STOP) {
    MIDIusb.sendStop();
  } else if (type == CLOCK) {
    MIDIusb.sendClock();
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

void handle_midi_in_NoteOff(uint8_t channel, uint8_t note, uint8_t vel) {
  Serial.println("Note Stop");
  switch (modes[sel_track - 1]) {
    case TRIGATE:
      MIDIusb.sendNoteOff(track_notes[sel_track - 1], vel, track_chan[sel_track - 1]);
      break;
    case NOTE:
      MIDIusb.sendNoteOff(note, vel, track_chan[sel_track - 1]);
      break;
    default: break;
  }
}

void handle_midi_in_NoteOn(uint8_t channel, uint8_t note, uint8_t vel) {
  Serial.println("Note Start");
  if (modes[sel_track - 1] == CC && veledit == 1) {
    switch (shifted) {
      case 0:
        vels[presets[sel_track - 1]][sel_track - 1][selstep] = note;
        seqs[presets[sel_track - 1]][sel_track - 1][selstep] = 1;
        break;
      case 1:
        // LIVE ENTRY
        vels[presets[sel_track - 1]][sel_track - 1][currstep] = note;
        seqs[presets[sel_track - 1]][sel_track - 1][currstep] = 1;
        break;
      default: break;
    }
  } else if (modes[sel_track - 1] == NOTE && veledit == 1) {
    switch (shifted) {
      case 0:
        send_note_off(notes[presets[sel_track - 1]][sel_track - 1][selstep], 0, 0, 1, track_chan[sel_track - 1]);
        notes[presets[sel_track - 1]][sel_track - 1][selstep] = note;
        vels[presets[sel_track - 1]][sel_track - 1][selstep] = vel;
        seqs[presets[sel_track - 1]][sel_track - 1][selstep] = 1;
        break;
      case 1:
        // LIVE ENTRY
        send_note_off(notes[presets[sel_track - 1]][sel_track - 1][currstep], 0, 0, 1, track_chan[sel_track - 1]);
        notes[presets[sel_track - 1]][sel_track - 1][currstep] = note;
        vels[presets[sel_track - 1]][sel_track - 1][currstep] = vel;
        seqs[presets[sel_track - 1]][sel_track - 1][currstep] = 1;
        MIDIusb.sendNoteOn(note, vel, track_chan[sel_track - 1]);
        break;
      default: break;
    }
  } else if (modes[sel_track - 1] == TRIGATE) {
    switch (shifted) {
      case 0:
        vels[presets[sel_track - 1]][sel_track - 1][selstep] = note;
        seqs[presets[sel_track - 1]][sel_track - 1][selstep] = 1;
        break;
      case 1:
        // LIVE ENTRY
        vels[presets[sel_track - 1]][sel_track - 1][currstep] = note;
        seqs[presets[sel_track - 1]][sel_track - 1][currstep] = 1;
        MIDIusb.sendNoteOn(track_notes[sel_track - 1], vel, track_chan[sel_track - 1]);
        trellis.setPixelColor(currstep, W100);
        break;
      default: break;
    }
  }
}

void handle_midi_in_CC(uint8_t channel, uint8_t cc, uint8_t val) {
  Serial.println("CC Incoming");
  switch (modes[sel_track - 1]) {
    case CC:
      vels[presets[sel_track - 1]][sel_track - 1][selstep] = val;
      // LIVE ENTRY
      //vels[presets[sel_track - 1]][sel_track - 1][currstep] = val;
      break;
    case NOTE:
      notes[presets[sel_track - 1]][sel_track - 1][selstep] = val;
      // LIVE ENTRY
      //notes[presets[sel_track - 1]][sel_track - 1][currstep] = val;
      break;
    default: break;
  }
}

//
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
      default: break;
    }
  }
}

void configure_sequencer() {
  Serial.println("configuring sequencer");
  seqr.set_tempo(tempo);
  seqr.ticks_per_step = cfg.step_size;
  seqr.on_func = send_note_on;
  seqr.off_func = send_note_off;
  seqr.clk_func = send_clock_start_stop;
  seqr.cc_func = send_cc;
  seqr.send_clock = cfg.midi_send_clock;
  seqr.length = length;
  seqr.transpose = transpose;
};

void toggle_selected(uint8_t keyId) {
  trellis.setPixelColor(lastsel + (numsteps - 1), seq_dim(lastsel, 40));
  trellis.setPixelColor(keyId, seq_col(sel_track));
  if (run == 0) {
    strip.setPixelColor(0, seq_col(sel_track));
    strip.show();
  }
}

void show_sequence(uint8_t seq) {
  patedit = 1;
  for (uint8_t i = 0; i < numsteps; ++i) {
    trellis.setPixelColor(i, seqs[presets[seq - 1]][seq - 1][i] > 0 ? seq_col(sel_track) : 0);
  }
  if (run == 0) { trellis.show(); }
}

void set_gate(uint8_t gid, uint8_t stp, int c) {
  uint32_t col = gates[presets[gid]][gid][stp] >= 15 ? c : Wheel(gates[presets[gid]][gid][stp] * 5);
  trellis.setPixelColor(stp, col);
  if (run == 0) { trellis.show(); }
}

void show_gates(uint8_t seq) {
  gateedit = 1;
  uint8_t gateId = seq - 1;
  uint32_t col = 0;
  for (uint8_t i = 0; i < numsteps; ++i) {
    set_gate(gateId, i, seq_col(seq));
  }
  if (run == 0) { trellis.show(); }
}

void show_probabilities(uint8_t seq) {
  probedit = 1;
  uint32_t col = 0;
  for (uint8_t i = 0; i < numsteps; ++i) {
    col = probs[presets[seq - 1]][seq - 1][i] == 10 ? seq_col(seq) : Wheel(probs[presets[seq - 1]][seq - 1][i] * 10);
    trellis.setPixelColor(i, col);
  }
  if (run == 0) { trellis.show(); }
}

void show_accents(uint8_t seq) { // velocities
  veledit = 1;
  uint32_t col = 0;
  if (modes[seq - 1] == CC) {
    for (uint8_t i = 0; i < numsteps; ++i) {
      trellis.setPixelColor(i, Wheel(vels[presets[sel_track - 1]][seq - 1][i]));
    }
  } else {
    for (uint8_t i = 0; i < numsteps; ++i) {
      trellis.setPixelColor(i, seq_dim(seq, vels[presets[sel_track - 1]][seq - 1][i]));
    }
  }
  if (run == 0) { trellis.show(); }
}

void show_notes(uint8_t seq) {
  notesedit = 1;
  uint32_t col = 0;
  if (modes[seq - 1] == NOTE) {
    for (uint8_t i = 0; i < numsteps; ++i) {
      trellis.setPixelColor(i, Wheel(notes[presets[sel_track - 1]][seq - 1][i]));
    }
  } else {
    for (uint8_t i = 0; i < numsteps; ++i) {
      trellis.setPixelColor(i, seq_dim(seq, notes[presets[sel_track - 1]][seq - 1][i]));
    }
  }
  if (run == 0) { trellis.show(); }
}

void show_presets() {
  for (uint8_t i = 0; i < numsteps; ++i) {
    trellis.setPixelColor(i, i < (numsteps / 2) ? 0 : W10);
  }
  trellis.setPixelColor(presets[sel_track - 1], W100);
  trellis.show();
}

void show_divisions() {
  for (uint8_t i = 0; i < numsteps; ++i) {
    trellis.setPixelColor(i, 0);
  }
  trellis.setPixelColor(divs[presets[sel_track - 1]][sel_track - 1], seq_col(sel_track));
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
  if (run == 0) { trellis.show(); }
}

// Update Channel Config MODE buttons
void mode_leds(uint8_t track) {
  for (uint8_t i = X_DIM * 3; i < numsteps; ++i) {
    trellis.setPixelColor(i, 0);
  }
  switch (modes[track - 1]) {
    case TRIGATE:
      trellis.setPixelColor(24, W100);
      break;
    case CC:
      trellis.setPixelColor(25, W100);
      if (track - 1 == 6 || track - 1 == 7) {
        if (hzv[(track - 1) - 6] == 1) {
          trellis.setPixelColor(31, seq_dim(track, 100));  //hzv
        } else {
          trellis.setPixelColor(31, W100);  //voct
        };
      }
      break;
    case NOTE:
      trellis.setPixelColor(26, W100);
      if (track - 1 == 6 || track - 1 == 7) {
        if (hzv[(track - 1) - 6] == 1) {
          trellis.setPixelColor(31, seq_dim(track, 100));  //hzv
        } else {
          trellis.setPixelColor(31, W100);  //voct
        };
      }
      break;
    case CHORD:
      trellis.setPixelColor(27, W100);
      break;
    case ARP:
      trellis.setPixelColor(28, W100);
      break;
    default: break;
  }
  if (run == 0) { trellis.show(); }
}

// Initialise Channel Config display...
void init_chan_conf(uint8_t track) {
  for (uint8_t i = 0; i < numsteps; ++i) {
    trellis.setPixelColor(i, 0);
  }
  trellis.setPixelColor(track_chan[track - 1] - 1, seq_col(track));
  switch (modes[track - 1]) {
    case TRIGATE:
      trellis.setPixelColor(24, W100);
      break;
    case CC:
      trellis.setPixelColor(25, W100);
      if (track - 1 == 6 || track - 1 == 7) {
        if (hzv[(track - 1) - 6] == 1) {
          trellis.setPixelColor(31, seq_dim(track, 100));  //hzv
        } else {
          trellis.setPixelColor(31, W100);  //voct
        };
      }
      break;
    case NOTE:
      trellis.setPixelColor(26, W100);
      if (track - 1 == 6 || track - 1 == 7) {
        if (hzv[(track - 1) - 6] == 1) {
          trellis.setPixelColor(31, seq_dim(track, 100));  //hzv
        } else {
          trellis.setPixelColor(31, W100);  //voct
        };
      }
      break;
    case CHORD:
      trellis.setPixelColor(27, W100);
      break;
    case ARP:
      trellis.setPixelColor(28, W100);
      break;
    default: break;
  }
  if (run == 0) { trellis.show(); }
}

// Show "Are you sure, Y/N" display...
void sure_pane() {
  for (uint8_t i = 0; i < t_size; ++i) {
    trellis.setPixelColor(i, i == 26 ? GREEN : i == 29 ? RED
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

  //Seq 1 > 8
  trellis.setPixelColor(32, sel_track == 1 ? seq_col(sel_track) : seq_dim(1, 40));
  trellis.setPixelColor(33, sel_track == 2 ? seq_col(sel_track) : seq_dim(2, 40));
  trellis.setPixelColor(34, sel_track == 3 ? seq_col(sel_track) : seq_dim(3, 40));
  trellis.setPixelColor(35, sel_track == 4 ? seq_col(sel_track) : seq_dim(4, 40));
  trellis.setPixelColor(36, sel_track == 5 ? seq_col(sel_track) : seq_dim(5, 40));
  trellis.setPixelColor(37, sel_track == 6 ? seq_col(sel_track) : seq_dim(6, 40));
  trellis.setPixelColor(38, sel_track == 7 ? seq_col(sel_track) : seq_dim(7, 40));
  trellis.setPixelColor(39, sel_track == 8 ? seq_col(sel_track) : seq_dim(8, 40));
  //Mutes
  trellis.setPixelColor(40, mutes[0] == 1 ? R80 : G40);
  trellis.setPixelColor(41, mutes[1] == 1 ? R80 : G40);
  trellis.setPixelColor(42, mutes[2] == 1 ? R80 : G40);
  trellis.setPixelColor(43, mutes[3] == 1 ? R80 : G40);
  trellis.setPixelColor(44, mutes[4] == 1 ? R80 : G40);
  trellis.setPixelColor(45, mutes[5] == 1 ? R80 : G40);
  trellis.setPixelColor(46, mutes[6] == 1 ? R80 : G40);
  trellis.setPixelColor(47, mutes[7] == 1 ? R80 : G40);
  //Panes
  trellis.setPixelColor(48, patedit == 1 ? R127 : R40);
  trellis.setPixelColor(49, veledit == 1 ? Y127 : Y40);
  trellis.setPixelColor(50, probedit == 1 ? P127 : P40);
  trellis.setPixelColor(51, gateedit == 1 ? B127 : B40);
  //Globals
  trellis.setPixelColor(52, shifted == 1 ? W100 : PK40);
  if (chanedit == 0) {
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
  } else {
    trellis.setPixelColor(53, R127);
  }
  trellis.setPixelColor(54, G40);
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
  trellis.setPixelColor(55, O127);
  //Control Row
  trellis.setPixelColor(56, GREEN);
  trellis.setPixelColor(57, RED);
  trellis.setPixelColor(58, ORANGE);
  trellis.setPixelColor(59, B80);
  trellis.setPixelColor(60, C40);
  trellis.setPixelColor(61, YELLOW);
  trellis.setPixelColor(62, RED);
  trellis.setPixelColor(63, GREEN);

  trellis.show();
}

#include "saveload.h"  /// FIXME:

TrellisCallback onKey(keyEvent evt) {
  auto const now = millis();
  auto const keyId = evt.bit.NUM;
  if (marci_debug) { Serial.println(keyId); }
  switch (evt.bit.EDGE) {
    case SEESAW_KEYPAD_EDGE_RISING:
      uint32_t col;
      if (sure == 1) { // SURE Y/N STEP EDIT
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
      } else if (divedit == 1) { // TRACK CLOCK DIVIDER STEP EDIT
        if (keyId < (numsteps)) {
          divs[presets[sel_track - 1]][sel_track - 1] = keyId;
          divcounts[sel_track - 1] = -1;
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
        } else if (keyId < 40 && keyId >= numsteps) {
          divedit = 1;
          lastsel = sel_track;
          sel_track = keyId - (numsteps -1);
          toggle_selected(keyId);
          show_divisions();
        } 
      } else if (presetmode == 1 && (keyId < numsteps || keyId > 39 && keyId < 56)) { // PRESET STEP EDIT
        if (keyId < (numpresets)) {
          for (uint8_t i = 0; i < numpresets; ++i) {
            trellis.setPixelColor(i, 0);
          }
          trellis.setPixelColor(presets[sel_track - 1], 0);
          trellis.setPixelColor(keyId, W100);
          presets[sel_track - 1] = keyId;
        } else if (keyId > ((numpresets - 1)) && keyId < (numpresets * 2)) {
          trellis.setPixelColor(presets[sel_track - 1], 0);
          trellis.setPixelColor(keyId - (X_DIM * 2), W100);
          for (uint8_t i = 0; i < numpresets; ++i) {
            presets[i] = (keyId - (X_DIM * 2));
          }
        } else if (keyId > 39 && keyId < 48) {
          if (mutes[keyId - 40] == 0) {
            mutes[keyId - 40] = 1;
            trellis.setPixelColor(keyId, R80);
          } else {
            mutes[keyId - 40] = 0;
            trellis.setPixelColor(keyId, G80);
          }
        }
        show_presets();
      } else if (chanedit == 1 && keyId < numsteps) { // CHANNEL STEP EDIT
        if (keyId < 16) {
          uint8_t chan = keyId + 1;
          track_chan[sel_track - 1] = chan;
          for (uint8_t i = 0; i < 16; ++i) {
            trellis.setPixelColor(i, 0);
          }
          trellis.setPixelColor(keyId, seq_col(sel_track));
        } else {
          switch (keyId) {
            case 24:
              modes[sel_track - 1] = TRIGATE;
              break;
            case 25:
              modes[sel_track - 1] = CC;
              break;
            case 26:
              modes[sel_track - 1] = NOTE;
              break;
            case 27:
              //modes[sel_track-1] = CHORD;
              break;
            case 28:
              //modes[sel_track-1] = ARP;
              break;
            case 31:
              if ((modes[sel_track - 1] == CC || modes[sel_track - 1] == NOTE) && (sel_track - 1 > 5)) {
                hzv[(sel_track - 1) - 6] = hzv[(sel_track - 1) - 6] == 0 ? 1 : 0;
              }
              break;
            default: break;
          }
          mode_leds(sel_track);
        }
      } else if (chanedit == 1 && ((keyId > (numsteps - 1) + X_DIM && keyId < (numsteps) + X_DIM + 12) || (keyId > (numsteps) + X_DIM + 13 && keyId < (t_size - 2)))) {
        // do nothin'
      } else if (lenedit == 1 && keyId < numsteps) { // LENGTH EDIT
        if (keyId + 1 >= offsets[sel_track - 1]) {
          lengths[sel_track - 1] = keyId + 1;
          //length = keyId + 1;
          lenedit = 0;
          trellis.setPixelColor(keyId, C127);
          trellis.setPixelColor(54, G40);
          configure_sequencer();
        }
      } else if (offedit == 1 && keyId < numsteps) { // OFFSET EDIT
        if (keyId + 1 <= lengths[sel_track - 1]) {
          offsets[sel_track - 1] = keyId + 1;
          //length = keyId + 1;
          offedit = 0;
          trellis.setPixelColor(keyId, B127);
          trellis.setPixelColor(54, B40);
          configure_sequencer();
        }
      } else if (gateedit == 1 && keyId < numsteps) { // GATE STEP EDIT
        uint8_t trk_arr = sel_track - 1;
        uint8_t gateId = trk_arr;
        if (gates[presets[trk_arr]][gateId][keyId] >= 15) {
          gates[presets[trk_arr]][gateId][keyId] = 0;
        }
        gates[presets[trk_arr]][gateId][keyId] += 3;
        set_gate(gateId, keyId, seq_col(sel_track));
      } else if (probedit == 1 && keyId < numsteps) { // PROBABILITY STEP EDIT
        uint8_t trk_arr = sel_track - 1;
        if (probs[presets[trk_arr]][trk_arr][keyId] == 10) {
          probs[presets[trk_arr]][trk_arr][keyId] = 0;
        }
        probs[presets[trk_arr]][trk_arr][keyId] += 1;
        col = probs[presets[trk_arr]][trk_arr][keyId] == 10 ? seq_col(sel_track) : Wheel(probs[presets[trk_arr]][trk_arr][keyId] * 10);
        trellis.setPixelColor(keyId, col);
        if (run == 0) { trellis.show(); }
      } else if (notesedit == 1 & keyId < numsteps) { // NOTES STEP EDIT
        uint8_t trk_arr = sel_track - 1;
        if (modes[trk_arr] == CC || modes[trk_arr] == NOTE) {
          uint8_t prev_selstep = selstep;
          selstep = keyId;
          trellis.setPixelColor(prev_selstep, Wheel(notes[presets[trk_arr]][trk_arr][prev_selstep]));
          trellis.setPixelColor(selstep, W100);
        }
      } else if (veledit == 1 & keyId < numsteps) { // VELOCITY STEP EDIT
        uint8_t trk_arr = sel_track - 1;
        if (modes[trk_arr] == CC || modes[trk_arr] == NOTE) {
          uint8_t prev_selstep = selstep;
          selstep = keyId;
          trellis.setPixelColor(prev_selstep, Wheel(vels[presets[trk_arr]][trk_arr][prev_selstep]));
          trellis.setPixelColor(selstep, W100);
        } else {
          switch (sel_track) {
            case 0:
              break;
            case 1:
              switch (vels[presets[trk_arr]][trk_arr][keyId]) {
                case 127:
                  trellis.setPixelColor(keyId, R40);
                  vels[presets[trk_arr]][trk_arr][keyId] = 40;
                  break;
                case 80:
                  trellis.setPixelColor(keyId, R127);
                  vels[presets[trk_arr]][trk_arr][keyId] = 127;
                  break;
                case 40:
                  trellis.setPixelColor(keyId, R80);
                  vels[presets[trk_arr]][trk_arr][keyId] = 80;
                  break;
                default:
                  break;
              }
              break;
            case 2:
              switch (vels[presets[trk_arr]][trk_arr][keyId]) {
                case 127:
                  trellis.setPixelColor(keyId, O40);
                  vels[presets[trk_arr]][trk_arr][keyId] = 40;
                  break;
                case 80:
                  trellis.setPixelColor(keyId, O127);
                  vels[presets[trk_arr]][trk_arr][keyId] = 127;
                  break;
                case 40:
                  trellis.setPixelColor(keyId, O80);
                  vels[presets[trk_arr]][trk_arr][keyId] = 80;
                  break;
                default:
                  break;
              }
              break;
            case 3:
              switch (vels[presets[trk_arr]][trk_arr][keyId]) {
                case 127:
                  trellis.setPixelColor(keyId, Y40);
                  vels[presets[trk_arr]][trk_arr][keyId] = 40;
                  break;
                case 80:
                  trellis.setPixelColor(keyId, Y127);
                  vels[presets[trk_arr]][trk_arr][keyId] = 127;
                  break;
                case 40:
                  trellis.setPixelColor(keyId, Y80);
                  vels[presets[trk_arr]][trk_arr][keyId] = 80;
                  break;
                default:
                  break;
              }
              break;
            case 4:
              switch (vels[presets[trk_arr]][trk_arr][keyId]) {
                case 127:
                  trellis.setPixelColor(keyId, G40);
                  vels[presets[trk_arr]][trk_arr][keyId] = 40;
                  break;
                case 80:
                  trellis.setPixelColor(keyId, G127);
                  vels[presets[trk_arr]][trk_arr][keyId] = 127;
                  break;
                case 40:
                  trellis.setPixelColor(keyId, G80);
                  vels[presets[trk_arr]][trk_arr][keyId] = 80;
                  break;
                default:
                  break;
              }
              break;
            case 5:
              switch (vels[presets[trk_arr]][trk_arr][keyId]) {
                case 127:
                  trellis.setPixelColor(keyId, C40);
                  vels[presets[trk_arr]][trk_arr][keyId] = 40;
                  break;
                case 80:
                  trellis.setPixelColor(keyId, C127);
                  vels[presets[trk_arr]][trk_arr][keyId] = 127;
                  break;
                case 40:
                  trellis.setPixelColor(keyId, C80);
                  vels[presets[trk_arr]][trk_arr][keyId] = 80;
                  break;
                default:
                  break;
              }
              break;
            case 6:
              switch (vels[presets[trk_arr]][trk_arr][keyId]) {
                case 127:
                  trellis.setPixelColor(keyId, B40);
                  vels[presets[trk_arr]][trk_arr][keyId] = 40;
                  break;
                case 80:
                  trellis.setPixelColor(keyId, B127);
                  vels[presets[trk_arr]][trk_arr][keyId] = 127;
                  break;
                case 40:
                  trellis.setPixelColor(keyId, B80);
                  vels[presets[trk_arr]][trk_arr][keyId] = 80;
                  break;
                default:
                  break;
              }
              break;
            case 7:
              switch (vels[presets[trk_arr]][trk_arr][keyId]) {
                case 127:
                  trellis.setPixelColor(keyId, P40);
                  vels[presets[trk_arr]][trk_arr][keyId] = 40;
                  break;
                case 80:
                  trellis.setPixelColor(keyId, P127);
                  vels[presets[trk_arr]][trk_arr][keyId] = 127;
                  break;
                case 40:
                  trellis.setPixelColor(keyId, P80);
                  vels[presets[trk_arr]][trk_arr][keyId] = 80;
                  break;
                default:
                  break;
              }
              break;
            case 8:
              switch (vels[presets[trk_arr]][trk_arr][keyId]) {
                case 127:
                  trellis.setPixelColor(keyId, PK40);
                  vels[presets[trk_arr]][trk_arr][keyId] = 40;
                  break;
                case 80:
                  trellis.setPixelColor(keyId, PK127);
                  vels[presets[trk_arr]][trk_arr][keyId] = 127;
                  break;
                case 40:
                  trellis.setPixelColor(keyId, PK80);
                  vels[presets[trk_arr]][trk_arr][keyId] = 80;
                  break;
                default:
                  break;
              }
              break;
            default:
              break;
          }
        }
      } else if (keyId < numsteps) { // STEP EDIT
        uint8_t trk_arr = sel_track - 1;
        col = W10;
        switch (seqs[presets[trk_arr]][trk_arr][keyId]) {
          case 1:
            seqs[presets[trk_arr]][trk_arr][keyId] = 0;
            break;
          case 0:
            col = seq_col(sel_track);
            seqs[presets[trk_arr]][trk_arr][keyId] = 1;
            break;
        }
        trellis.setPixelColor(keyId, col);
      } else if (keyId < 40) { // SELECT TRACK 1 - 8
        switch (keyId) {
          case 32:
            lastsel = sel_track;
            sel_track = 1;
            break;
          case 33:
            lastsel = sel_track;
            sel_track = 2;
            break;
          case 34:
            lastsel = sel_track;
            sel_track = 3;
            break;
          case 35:
            lastsel = sel_track;
            sel_track = 4;
            break;
          case 36:
            lastsel = sel_track;
            sel_track = 5;
            break;
          case 37:
            lastsel = sel_track;
            sel_track = 6;
            break;
          case 38:
            lastsel = sel_track;
            sel_track = 7;
            break;
          case 39:
            lastsel = sel_track;
            sel_track = 8;
            break;
        }
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
        uint8_t trk_arr = sel_track - 1;
        switch (keyId) {
          case 40: // MUTE 1
            if (mutes[0] == 0) {
              mutes[0] = 1;
              trellis.setPixelColor(40, R80);
            } else {
              mutes[0] = 0;
              trellis.setPixelColor(40, G40);
            }
            break;
          case 41: // MUTE 2
            if (mutes[1] == 0) {
              mutes[1] = 1;
              trellis.setPixelColor(41, R80);
            } else {
              mutes[1] = 0;
              trellis.setPixelColor(41, G40);
            }
            break;
          case 42: // MUTE 3
            if (mutes[2] == 0) {
              mutes[2] = 1;
              trellis.setPixelColor(42, R80);
            } else {
              mutes[2] = 0;
              trellis.setPixelColor(42, G40);
            }
            break;
          case 43: // MUTE 4
            if (mutes[3] == 0) {
              mutes[3] = 1;
              trellis.setPixelColor(43, R80);
            } else {
              mutes[3] = 0;
              trellis.setPixelColor(43, G40);
            }
            break;
          case 44: // MUTE 5
            if (mutes[4] == 0) {
              mutes[4] = 1;
              trellis.setPixelColor(44, R80);
            } else {
              mutes[4] = 0;
              trellis.setPixelColor(44, G40);
            }
            break;
          case 45: // MUTE 6
            if (mutes[5] == 0) {
              mutes[5] = 1;
              trellis.setPixelColor(45, R80);
            } else {
              mutes[5] = 0;
              trellis.setPixelColor(45, G40);
            }
            break;
          case 46: // MUTE 7
            if (mutes[6] == 0) {
              mutes[6] = 1;
              trellis.setPixelColor(46, R80);
            } else {
              mutes[6] = 0;
              trellis.setPixelColor(46, G40);
            }
            break;
          case 47: // MUTE 8
            if (mutes[7] == 0) {
              mutes[7] = 1;
              trellis.setPixelColor(47, R80);
            } else {
              mutes[7] = 0;
              trellis.setPixelColor(47, G40);
            }
            break;
          case 48: // SHOW PATTERN
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
          case 49: // SHOW VELOCITIES
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
          case 50: // SHOW PROBABILITY
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
          case 51: // SHOW GATES
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
          case 52: // SHIFT (^)
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
          case 53: // TRANSPOSE | ^TRACK DIVISION (RUNNING) | ^CHANNEL CONFIG (STOPPED)
            if (run == 0) {
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
          case 54: // LENGTH | ^OFFSET
            if (shifted == 0) {
              if (lenedit == 0) {
                offedit = 0;
                lenedit = 1;
                trellis.setPixelColor(54, G127);
              } else {
                lenedit = 0;
                trellis.setPixelColor(54, G40);
              }
            } else {
              if (offedit == 0) {
                lenedit = 0;
                offedit = 1;
                trellis.setPixelColor(54, B127);
              } else {
                offedit = 0;
                trellis.setPixelColor(54, B40);
              }
            }
            break;
          case 55: // BASE CLOCK DIVIDER | ^SWING
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
          case 56: // PLAY/STOP
            if (chanedit == 0) { seqr.toggle_play_stop(); }
            break;
          case 57: // STOP
            if (chanedit == 0) { seqr.stop(); }
            break;
          case 58: // RESET
            if (chanedit == 0) { seqr.reset(); }
            break;
          case 59: // WRITE
            if (chanedit == 0) {
              trellis.setPixelColor(59, R127);
              sequences_write();
            }
            break;
          case 60: // PRESETS | ^FACT RESET
            if (shifted == 1) {
              if (sure == 0) {
                trellis.setPixelColor(60, R127);
                sure_pane();
              }
            } else {
              if (presetmode == 0) {
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
          case 61: // CLOCK ON/OFF
            if (swingedit == 1) {
              swing = 0;
            } else if (veledit == 1) {
              for (uint8_t i = 0; i < numsteps; ++i) {
                vels[presets[sel_track - 1]][sel_track - 1][i] = 72;
              }
            } else if (notesedit == 1) {
              for (uint8_t i = 0; i < numsteps; ++i) {
                notes[presets[sel_track - 1]][sel_track - 1][i] = 0;
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
          case 62: // PARAM -
            if (chanedit == 1) {
              brightness = brightness > 15 ? brightness - 10 : 5;
              init_interface();
              init_chan_conf(sel_track);
            } else if (gateedit == 1 && swingedit == 0) {
              for (uint8_t i = 0; i < 32; ++i) {
                gates[presets[trk_arr]][trk_arr][i] = gates[presets[trk_arr]][trk_arr][i] - 1 > 1 ? gates[presets[trk_arr]][trk_arr][i] - 1 : 1;
                if (run == 0) set_gate(sel_track - 1, i, seq_col(sel_track));
              }
              if (run == 0) { trellis.show(); }
            } else if (shifted == 1 && swingedit == 0) {
              track_notes[sel_track - 1] = track_notes[sel_track - 1] > 0 ? track_notes[sel_track - 1] - 1 : 127;
            } else if (shifted == 1 && swingedit == 1) {
              swing = swing > 0 ? swing - 1 : 0;
            } else if (probedit == 1) {
              for (uint8_t i = 0; i < numsteps; ++i) {
                probs[presets[sel_track - 1]][sel_track - 1][i] = probs[presets[sel_track - 1]][sel_track - 1][i] > 1 ? probs[presets[sel_track - 1]][sel_track - 1][i] - 1 : 1;
              }
            } else if (veledit == 1) {
              if (modes[sel_track - 1] == CC || modes[sel_track - 1] == NOTE) {
                vels[presets[sel_track - 1]][sel_track - 1][selstep] = vels[presets[sel_track - 1]][sel_track - 1][selstep] > 5 ? vels[presets[sel_track - 1]][sel_track - 1][selstep] - 1 : 0;
              } else {
                for (uint8_t i = 0; i < numsteps; ++i) {
                  vels[presets[sel_track - 1]][sel_track - 1][i] = vels[presets[sel_track - 1]][sel_track - 1][i] > 5 ? vels[presets[sel_track - 1]][sel_track - 1][i] - 5 : 0;
                }
              }
            } else if (notesedit == 1) {
              if (modes[sel_track - 1] == CC || modes[sel_track - 1] == NOTE) {
                notes[presets[sel_track - 1]][sel_track - 1][selstep] = notes[presets[sel_track - 1]][sel_track - 1][selstep] > 0 ? notes[presets[sel_track - 1]][sel_track - 1][selstep] - 1 : 0;
              }
            } else {
              tempo = tempo - 1;
              configure_sequencer();
            }
            break;
          case 63: // PARAM +
            if (chanedit == 1) {
              brightness = brightness < 117 ? brightness + 10 : 127;
              init_interface();
              init_chan_conf(sel_track);
            } else if (gateedit == 1 && swingedit == 0) {
              for (uint8_t i = 0; i < 32; ++i) {
                gates[presets[trk_arr]][trk_arr][i] = gates[presets[trk_arr]][trk_arr][i] + 1 < 15 ? gates[presets[trk_arr]][trk_arr][i] + 1 : 15;
                if (run == 0) set_gate(sel_track - 1, i, seq_col(sel_track));
              }
              if (run == 0) { trellis.show(); }
            } else if (shifted == 1 && swingedit == 0) {
              track_notes[sel_track - 1] = track_notes[sel_track - 1] < 127 ? track_notes[sel_track - 1] + 1 : 1;
            } else if (shifted == 1 && swingedit == 1) {
              swing = swing < 30 ? swing + 1 : 30;
            } else if (probedit == 1) {
              for (uint8_t i = 0; i < numsteps; ++i) {
                probs[presets[sel_track - 1]][sel_track - 1][i] = probs[presets[sel_track - 1]][sel_track - 1][i] < 10 ? probs[presets[sel_track - 1]][sel_track - 1][i] + 1 : 10;
              }
            } else if (veledit == 1) {
              if (modes[sel_track - 1] == CC || modes[sel_track - 1] == NOTE) {
                vels[presets[sel_track - 1]][sel_track - 1][selstep] = vels[presets[sel_track - 1]][sel_track - 1][selstep] < 122 ? vels[presets[sel_track - 1]][sel_track - 1][selstep] + 1 : 127;
              } else {
                for (uint8_t i = 0; i < numsteps; ++i) {
                  vels[presets[sel_track - 1]][sel_track - 1][i] = vels[presets[sel_track - 1]][sel_track - 1][i] < 122 ? vels[presets[sel_track - 1]][sel_track - 1][i] + 5 : 127;
                }
              }
            } else if (notesedit == 1) {
              if (modes[sel_track - 1] == CC || modes[sel_track - 1] == NOTE) {
                notes[presets[sel_track - 1]][sel_track - 1][selstep] = notes[presets[sel_track - 1]][sel_track - 1][selstep] < 127 ? notes[presets[sel_track - 1]][sel_track - 1][selstep] + 1 : 127;
              }
            } else {
              tempo = tempo + 1;
              configure_sequencer();
            }
            break;
          default:
            break;
        }
      }
      break;
    case SEESAW_KEYPAD_EDGE_FALLING:
      break;
  }
  return nullptr;
}

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

  for (uint8_t i = 0; i < sizeof(gatepins); ++i) {
    pinMode(gatepins[i], OUTPUT);  //digtal (gate) out
  }
  //analogWriteResolution(12); //analog to 12bit (0-4095)

  Serial.begin(115200);
  if (marci_debug) { delay(2000); }
  init_flash();
  Serial.println(freeMemory());
  sequences_read();
  notes_read();
  velocities_read();
  probabilities_read();
  gates_read();
  settings_read();
  configure_sequencer();
  if (!trellis.begin()) {
    Serial.println(F("failed to begin trellis"));
    while (1) delay(1);
  } else {
    Serial.println(F("Trellis Init..."));
  }
  strip.begin();
  strip.setPixelColor(0, seq_col(1));
  strip.show();

  randomSeed(analogRead(0));  // for probability

  for (uint8_t l = 0; l < numseqs; l++) {
    lengths[l] = t_size / 2;
    
    divcounts[l] = 0;
    offsets[l] = 0;
    outcomes[l] = 1;
    for (uint8_t p = 0; p < numpresets; p++) {
      divs[p][l] = 0;
    }
  }

  patedit = patedit == 0 ? 1 : patedit;
  sel_track = sel_track == 0 ? 1 : sel_track;
  lastsel = lastsel == 0 ? 1 : sel_track;

  init_interface();
  Serial.println(freeMemory());
  Serial.println(F("GO!"));
  show_sequence(1);
}

//
// --- MIDI in/out - ADD NOTHING HERE!
//
void loop() {
  midi_read_and_forward();
  seqr.update();  // will call send_note_{on,off} callbacks
}