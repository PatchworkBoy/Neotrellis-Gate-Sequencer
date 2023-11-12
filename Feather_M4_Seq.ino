/**
 * Feather M4 Seq -- An 8 track 32 step MIDI Gate Sequencer for Feather M4 Express & Neotrellis 8x8
 * 04 Nov 2023 - @apatchworkboy / Marci
 * 28 Apr 2023 - original via @todbot / Tod Kurt https://github.com/todbot/picostepseq/
 *
 * Libraries needed (all available via Library Manager):
 * - Adafruit SPIFlash -- https://github.com/adafruit/Adafruit_SPIFlash
 * - Adafruit_TinyUSB -- https://github.com/adafruit/Adafruit_TinyUSB_Arduino
 * - Adafruit Seesaw -- https://github.com/adafruit/Adafruit_Seesaw
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
#include <Adafruit_NeoTrellis.h>
#include <MIDI.h>
#include <ArduinoJson.h>
#include "flash_config.h"
#include <array>
using namespace std;

Adafruit_SPIFlash flash(&flashTransport);
FatVolume fatfs;

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
#define maincolor seesaw_NeoPixel::Color(0, 0, 5)
#define RED seesaw_NeoPixel::Color(155, 10, 10)
#define R40 seesaw_NeoPixel::Color(20, 5, 5)
#define R80 seesaw_NeoPixel::Color(90, 6, 6)
#define R127 seesaw_NeoPixel::Color(200, 6, 6)
#define ORANGE seesaw_NeoPixel::Color(155, 80, 10)
#define O40 seesaw_NeoPixel::Color(20, 10, 2)
#define O80 seesaw_NeoPixel::Color(90, 80, 10)
#define O127 seesaw_NeoPixel::Color(200, 100, 10)
#define YELLOW seesaw_NeoPixel::Color(155, 155, 30)
#define Y40 seesaw_NeoPixel::Color(20, 20, 4)
#define Y80 seesaw_NeoPixel::Color(90, 90, 25)
#define Y127 seesaw_NeoPixel::Color(200, 200, 40)
#define GREEN seesaw_NeoPixel::Color(0, 155, 0)
#define G40 seesaw_NeoPixel::Color(0, 20, 0)
#define G80 seesaw_NeoPixel::Color(0, 90, 0)
#define G127 seesaw_NeoPixel::Color(0, 200, 0)
#define CYAN seesaw_NeoPixel::Color(40, 155, 155)
#define C40 seesaw_NeoPixel::Color(4, 20, 20)
#define C80 seesaw_NeoPixel::Color(25, 90, 90)
#define C127 seesaw_NeoPixel::Color(40, 200, 200)
#define BLUE seesaw_NeoPixel::Color(20, 50, 155)
#define B40 seesaw_NeoPixel::Color(2, 10, 20)
#define B80 seesaw_NeoPixel::Color(10, 80, 90)
#define B127 seesaw_NeoPixel::Color(10, 100, 200)
#define PURPLE seesaw_NeoPixel::Color(100, 20, 155)
#define P40 seesaw_NeoPixel::Color(13, 3, 20)
#define P80 seesaw_NeoPixel::Color(60, 12, 90)
#define P127 seesaw_NeoPixel::Color(130, 27, 200)
#define PINK seesaw_NeoPixel::Color(155, 40, 60)
#define PK40 seesaw_NeoPixel::Color(20, 5, 9)
#define PK80 seesaw_NeoPixel::Color(90, 24, 36)
#define PK127 seesaw_NeoPixel::Color(200, 53, 80)
#define OFF seesaw_NeoPixel::Color(0, 0, 0)
#define W100 seesaw_NeoPixel::Color(100, 100, 100)
#define W75 seesaw_NeoPixel::Color(75, 75, 75)
#define W10 seesaw_NeoPixel::Color(4, 4, 4)

const uint8_t numseqs = X_DIM;
const uint8_t numsteps = t_size / 2;
const uint8_t numpresets = X_DIM * 2;
uint8_t length = t_size / 2;  // same as numsteps at boot!

// Multitrack seq bits
uint8_t track_notes[numseqs];  // C2 thru G2
uint8_t ctrl_notes[3];
uint8_t track_chan[numseqs];
uint8_t ctrl_chan = 16;

// Empty patterns...
uint8_t seqs[numpresets][numseqs][numsteps];
uint8_t gates[numpresets][numseqs][numsteps];
uint8_t vels[numpresets][numseqs][numsteps];
uint8_t probs[numpresets][numseqs][numsteps];
uint8_t mutes[numseqs];
uint8_t presets[numpresets];

uint8_t currstep = 0;
uint8_t laststep = 0;
uint8_t transpose = 0;
uint8_t run = 0;
uint8_t m4init = 0;
uint8_t presetmode = 0;
uint8_t chanedit = 0;
uint8_t veledit = 0;
uint8_t lenedit = 0;
uint8_t probedit = 0;
uint8_t noteedit = 0;
uint8_t gateedit = 0;
uint8_t swingedit = 0;
uint8_t patedit = 1;
uint8_t sel_track = 1;
uint8_t lastsel = 1;
int swing = 0;

#include "Sequencer.h"
#include "save_locations.h"
#include "saved_patterns_json.h"
#include "saved_velocities_json.h"
#include "saved_probabilities_json.h"
#include "saved_gates_json.h"
#include "saved_settings_json.h"

typedef struct {
  int step_size;
  bool midi_send_clock;
  bool midi_forward_usb;
} Config;

Config cfg = {
  .step_size = SIXTEENTH_NOTE,
  .midi_send_clock = true,
  .midi_forward_usb = true
};

const bool midi_out_debug = false;
const bool midi_in_debug = false;
const bool marci_debug = false;

// end hardware definitions

Adafruit_USBD_MIDI usb_midi;                                  // USB MIDI object
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDIusb);  // USB MIDI

float tempo = 120;
StepSequencer seqr;

uint8_t midiclk_cnt = 0;
uint32_t midiclk_last_micros = 0;

#include "saveload.h"  /// FIXME:

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
    seqr.trigger_ext(now_micros);                // FIXME: figure out 2nd arg (was step_millis)
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
  seqr.send_clock = cfg.midi_send_clock;
  seqr.length = length;
  seqr.transpose = transpose;
};

uint32_t Wheel(byte WheelPos) {
  if (WheelPos < 85) {
    return seesaw_NeoPixel::Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    return seesaw_NeoPixel::Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
    WheelPos -= 170;
    return seesaw_NeoPixel::Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  return 0;
}

void toggle_selected(uint8_t keyId) {
  trellis.setPixelColor(keyId, seq_col(sel_track));
  trellis.setPixelColor(lastsel + (numsteps - 1), seq_dim(lastsel, 40));
}

void show_sequence(uint8_t seq) {
  patedit = 1;
  for (uint8_t i = 0; i < numsteps; ++i) {
    trellis.setPixelColor(i, seqs[presets[seq - 1]][seq - 1][i] > 0 ? seq_col(sel_track) : W10);
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

void show_accents(uint8_t seq) {
  veledit = 1;
  uint32_t col = 0;
  for (uint8_t i = 0; i < numsteps; ++i) {
    trellis.setPixelColor(i, seq_dim(seq, vels[presets[sel_track - 1]][seq - 1][i]));
  }
  if (run == 0) { trellis.show(); }
}

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

void init_chan_conf(uint8_t track) {
  for (uint8_t i = 0; i < numsteps; ++i) {
    trellis.setPixelColor(i, 0);
  }
  trellis.setPixelColor(track_chan[track - 1] - 1, seq_col(track));
  if (run == 0) { trellis.show(); }
}

void show_presets() {
  for (uint8_t i = 0; i < numsteps; ++i) {
    trellis.setPixelColor(i, i < (numsteps / 2) ? 0 : W10);
  }
  trellis.setPixelColor(presets[sel_track - 1], W100);
  if (run == 0) { trellis.show(); }
}

TrellisCallback onKey(keyEvent evt) {
  auto const now = millis();
  auto const keyId = evt.bit.NUM;
  if (marci_debug) { Serial.println(keyId); }
  switch (evt.bit.EDGE) {
    case SEESAW_KEYPAD_EDGE_RISING:
      uint32_t col;
      if (presetmode == 1 && (keyId < numsteps || keyId > 39 && keyId < 56)) {
        if (keyId < (numpresets)) {
          for (uint8_t i = 0; i < numpresets; ++i) {
            trellis.setPixelColor(i, 0);
          }
          trellis.setPixelColor(presets[sel_track - 1], 0);
          trellis.setPixelColor(keyId, W100);
          presets[sel_track - 1] = keyId;
        } else if (keyId > ((numpresets - 1)) && keyId < (numpresets*2)) {
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
      } else if (chanedit == 1 && keyId < numsteps) {
        if (keyId < 16) {
          uint8_t chan = keyId + 1;
          track_chan[sel_track - 1] = chan;
          for (uint8_t i = 0; i < 16; ++i) {
            trellis.setPixelColor(i, 0);
          }
          trellis.setPixelColor(keyId, seq_col(sel_track));
        }
      } else if (lenedit == 1 && keyId < numsteps) {
        length = keyId + 1;
        lenedit = 0;
        trellis.setPixelColor(keyId, C127);
        trellis.setPixelColor(54, G40);
        configure_sequencer();
      } else if (gateedit == 1 && keyId < numsteps) {
        uint8_t trk_arr = sel_track - 1;
        uint8_t gateId = trk_arr;
        if (gates[presets[trk_arr]][gateId][keyId] >= 15) {
          gates[presets[trk_arr]][gateId][keyId] = 0;
        }
        gates[presets[trk_arr]][gateId][keyId] += 3;
        set_gate(gateId, keyId, seq_col(sel_track));
      } else if (probedit == 1 && keyId < numsteps) {
        uint8_t trk_arr = sel_track - 1;
        if (probs[presets[trk_arr]][trk_arr][keyId] == 10) {
          probs[presets[trk_arr]][trk_arr][keyId] = 0;
        }
        probs[presets[trk_arr]][trk_arr][keyId] += 1;
        col = probs[presets[trk_arr]][trk_arr][keyId] == 10 ? seq_col(sel_track) : Wheel(probs[presets[trk_arr]][trk_arr][keyId] * 10);
        trellis.setPixelColor(keyId, col);
        if (run == 0) { trellis.show(); }
      } else if (veledit == 1 & keyId < numsteps) {
        uint8_t trk_arr = sel_track - 1;
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
      } else if (keyId < numsteps) {
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
      } else if (keyId < 40) {
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
          case 40:
            if (mutes[0] == 0) {
              mutes[0] = 1;
              trellis.setPixelColor(40, R80);
            } else {
              mutes[0] = 0;
              trellis.setPixelColor(40, G80);
            }
            break;
          case 41:
            if (mutes[1] == 0) {
              mutes[1] = 1;
              trellis.setPixelColor(41, R80);
            } else {
              mutes[1] = 0;
              trellis.setPixelColor(41, G80);
            }
            break;
          case 42:
            if (mutes[2] == 0) {
              mutes[2] = 1;
              trellis.setPixelColor(42, R80);
            } else {
              mutes[2] = 0;
              trellis.setPixelColor(42, G80);
            }
            break;
          case 43:
            if (mutes[3] == 0) {
              mutes[3] = 1;
              trellis.setPixelColor(43, R80);
            } else {
              mutes[3] = 0;
              trellis.setPixelColor(43, G80);
            }
            break;
          case 44:
            if (mutes[4] == 0) {
              mutes[4] = 1;
              trellis.setPixelColor(44, R80);
            } else {
              mutes[4] = 0;
              trellis.setPixelColor(44, G80);
            }
            break;
          case 45:
            if (mutes[5] == 0) {
              mutes[5] = 1;
              trellis.setPixelColor(45, R80);
            } else {
              mutes[5] = 0;
              trellis.setPixelColor(45, G80);
            }
            break;
          case 46:
            if (mutes[6] == 0) {
              mutes[6] = 1;
              trellis.setPixelColor(46, R80);
            } else {
              mutes[6] = 0;
              trellis.setPixelColor(46, G80);
            }
            break;
          case 47:
            if (mutes[7] == 0) {
              mutes[7] = 1;
              trellis.setPixelColor(47, R80);
            } else {
              mutes[7] = 0;
              trellis.setPixelColor(47, G80);
            }
            break;
          case 48:
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
          case 49:
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
          case 50:
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
          case 51:
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
          case 52:
            if (noteedit == 0) {
              noteedit = 1;
              trellis.setPixelColor(52, W100);
            } else {
              noteedit = 0;
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
          case 53:
            if (run == 0) {
              if (noteedit == 0) {
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
            }
            break;
          case 54:
            if (lenedit == 0) {
              lenedit = 1;
              trellis.setPixelColor(54, G127);
            } else {
              lenedit = 0;
              trellis.setPixelColor(54, G40);
            }
            break;
          case 55:
            if (noteedit == 0) {
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
            }
            break;
          case 56:
            seqr.toggle_play_stop();
            break;
          case 57:
            seqr.stop();
            break;
          case 58:
            seqr.reset();
            break;
          case 59:
            trellis.setPixelColor(59,R127);
            trellis.show();
            sequences_write();
            break;
          case 60:
            if (noteedit == 1) {
              pattern_reset();
              show_sequence(1);
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
          case 61:
            if (swingedit == 1) {
              swing = 0;
            } else if (veledit == 1) {
              for (uint8_t i = 0; i < numsteps; ++i) {
                vels[presets[sel_track - 1]][sel_track - 1][i] = 80;
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
          case 62:
            if (gateedit == 1 && swingedit == 0) {
              for (uint8_t i = 0; i < 32; ++i) {
                gates[presets[trk_arr]][trk_arr][i] = gates[presets[trk_arr]][trk_arr][i] - 1 > 1 ? gates[presets[trk_arr]][trk_arr][i] - 1 : 1;
                if (run == 0) set_gate(sel_track - 1, i, seq_col(sel_track));
              }
              if (run == 0) { trellis.show(); }
            } else if (noteedit == 1 && swingedit == 0) {
              track_notes[sel_track - 1] = track_notes[sel_track - 1] > 0 ? track_notes[sel_track - 1] - 1 : 127;
            } else if (noteedit == 1 && swingedit == 1) {
              swing = swing > -30 ? swing - 1 : -30;
            } else if (probedit == 1) {
              for (uint8_t i = 0; i < numsteps; ++i) {
                probs[presets[sel_track - 1]][sel_track - 1][i] = probs[presets[sel_track - 1]][sel_track - 1][i] > 1 ? probs[presets[sel_track - 1]][sel_track - 1][i] - 1 : 1;
              }
            } else if (veledit == 1) {
              for (uint8_t i = 0; i < numsteps; ++i) {
                vels[presets[sel_track - 1]][sel_track - 1][i] = vels[presets[sel_track - 1]][sel_track - 1][i] > 20 ? vels[presets[sel_track - 1]][sel_track - 1][i] - 10 : 10;
              }
            } else {
              tempo = tempo - 1;
              configure_sequencer();
            }
            break;
          case 63:
            if (gateedit == 1 && swingedit == 0) {
              for (uint8_t i = 0; i < 32; ++i) {
                gates[presets[trk_arr]][trk_arr][i] = gates[presets[trk_arr]][trk_arr][i] + 1 < 15 ? gates[presets[trk_arr]][trk_arr][i] + 1 : 15;
                if (run == 0) set_gate(sel_track - 1, i, seq_col(sel_track));
              }
              if (run == 0) { trellis.show(); }
            } else if (noteedit == 1 && swingedit == 0) {
              track_notes[sel_track - 1] = track_notes[sel_track - 1] < 127 ? track_notes[sel_track - 1] + 1 : 1;
            } else if (noteedit == 1 && swingedit == 1) {
              swing = swing < 30 ? swing + 1 : 30;
            } else if (probedit == 1) {
              for (uint8_t i = 0; i < numsteps; ++i) {
                probs[presets[sel_track - 1]][sel_track - 1][i] = probs[presets[sel_track - 1]][sel_track - 1][i] < 10 ? probs[presets[sel_track - 1]][sel_track - 1][i] + 1 : 10;
              }
            } else if (veledit == 1) {
              for (uint8_t i = 0; i < numsteps; ++i) {
                vels[presets[sel_track - 1]][sel_track - 1][i] = vels[presets[sel_track - 1]][sel_track - 1][i] < 117 ? vels[presets[sel_track - 1]][sel_track - 1][i] + 10 : 127;
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
  // Initialize flash library and check its chip ID.
  if (!flash.begin()) {
    Serial.println(F("Error, failed to initialize flash chip!"));
    while (1) {
      yield();
    }
  }
  Serial.print(F("Flash chip JEDEC ID: 0x"));
  Serial.println(flash.getJEDECID(), HEX);

  // First call begin to mount the filesystem.  Check that it returns true
  // to make sure the filesystem was mounted.
  if (!fatfs.begin(&flash)) {
    Serial.println(F("Error, failed to mount newly formatted filesystem!"));
    Serial.println(
      F("Was the flash chip formatted with the SdFat_format example?"));
    while (1) {
      yield();
    }
  }
  Serial.println(F("Mounted filesystem!"));
  flash_store();
}

void init_interface() {
  //Seq 1 > 8
  trellis.setPixelColor(32, RED);
  trellis.setPixelColor(33, O40);
  trellis.setPixelColor(34, Y40);
  trellis.setPixelColor(35, G40);
  trellis.setPixelColor(36, C40);
  trellis.setPixelColor(37, B40);
  trellis.setPixelColor(38, P40);
  trellis.setPixelColor(39, PK40);
  //Mutes
  trellis.setPixelColor(40, mutes[0] == 1 ? R80 : G40);
  trellis.setPixelColor(41, mutes[0] == 1 ? R80 : G40);
  trellis.setPixelColor(42, mutes[0] == 1 ? R80 : G40);
  trellis.setPixelColor(43, mutes[0] == 1 ? R80 : G40);
  trellis.setPixelColor(44, mutes[0] == 1 ? R80 : G40);
  trellis.setPixelColor(45, mutes[0] == 1 ? R80 : G40);
  trellis.setPixelColor(46, mutes[0] == 1 ? R80 : G40);
  trellis.setPixelColor(47, mutes[0] == 1 ? R80 : G40);
  //Panes
  trellis.setPixelColor(48, patedit == 1 ? R127 : R40);
  trellis.setPixelColor(49, veledit == 1 ? Y127 : Y40);
  trellis.setPixelColor(50, probedit == 1 ? P127 : P40);
  trellis.setPixelColor(51, gateedit == 1 ? B127 : B40);
  //Globals
  trellis.setPixelColor(52, PK40);
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

  if (m4init == 0) {
    for (int y = 0; y < Y_DIM; y++) {
      for (int x = 0; x < X_DIM; x++) {
        trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_RISING, true);
        //trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_FALLING, true);
        trellis.registerCallback(x, y, onKey);
      }
    }
    for (int x = 0; x < X_DIM; x++) {
      for (int y = 0; y < Y_DIM; y++) {
        t_array[y][x].pixels.setBrightness(127);
      }
    }
    m4init = 1;
  }
  trellis.show();
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

  MIDIusb.begin();
  MIDIusb.turnThruOff();  // turn off echo

  Serial.begin(115200);
  delay(2000);
  init_flash();
  Serial.println(freeMemory());
  sequences_read();
  velocities_read();
  probabilities_read();
  gates_read();
  settings_read();
  configure_sequencer();
  if (!trellis.begin()) {
    Serial.println(F("failed to begin trellis"));
    while (1) delay(1);
  } else {
    Serial.println(F("Init..."));
  }

  randomSeed(analogRead(0));  // for probability

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