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

// for flashTransport definition
#include "flash_config.h"
Adafruit_SPIFlash flash(&flashTransport);

// file system object from SdFat
FatVolume fatfs;

#define D_FLASH "/M4SEQ32"
#define Y_DIM 8 //number of rows of key
#define X_DIM 8 //number of columns of keys
#define t_size Y_DIM*X_DIM
#define t_length t_size-1
#define maincolor seesaw_NeoPixel::Color(0,0,5)
#define RED seesaw_NeoPixel::Color(155,10,10)
#define R40 seesaw_NeoPixel::Color(20,5,5)
#define R80 seesaw_NeoPixel::Color(90,6,6)
#define R127 seesaw_NeoPixel::Color(200,6,6)
#define ORANGE seesaw_NeoPixel::Color(155,80,10)
#define O40 seesaw_NeoPixel::Color(20,10,2)
#define O80 seesaw_NeoPixel::Color(90,80,10)
#define O127 seesaw_NeoPixel::Color(200,100,10)
#define YELLOW seesaw_NeoPixel::Color(155,155,30)
#define Y40 seesaw_NeoPixel::Color(20,20,4)
#define Y80 seesaw_NeoPixel::Color(90,90,25)
#define Y127 seesaw_NeoPixel::Color(200,200,40)
#define GREEN seesaw_NeoPixel::Color(0,155,0)
#define G40 seesaw_NeoPixel::Color(0,20,0)
#define G80 seesaw_NeoPixel::Color(0,90,0)
#define G127 seesaw_NeoPixel::Color(0,200,0)
#define CYAN seesaw_NeoPixel::Color(40,155,155)
#define C40 seesaw_NeoPixel::Color(4,20,20)
#define C80 seesaw_NeoPixel::Color(25,90,90)
#define C127 seesaw_NeoPixel::Color(40,200,200)
#define BLUE seesaw_NeoPixel::Color(20,50,155)
#define B40 seesaw_NeoPixel::Color(2,10,20)
#define B80 seesaw_NeoPixel::Color(10,80,90)
#define B127 seesaw_NeoPixel::Color(10,100,200)
#define PURPLE seesaw_NeoPixel::Color(100,20,155)
#define P40 seesaw_NeoPixel::Color(13,3,20)
#define P80 seesaw_NeoPixel::Color(60,12,90)
#define P127 seesaw_NeoPixel::Color(130,27,200)
#define PINK seesaw_NeoPixel::Color(155,40,60)
#define PK40 seesaw_NeoPixel::Color(20,5,9)
#define PK80 seesaw_NeoPixel::Color(90,24,36)
#define PK127 seesaw_NeoPixel::Color(200,53,80)
#define OFF seesaw_NeoPixel::Color(0,0,0)
#define W100 seesaw_NeoPixel::Color(100,100,100)
#define W75 seesaw_NeoPixel::Color(75,75,75)
#define W10 seesaw_NeoPixel::Color(4,4,4)

int r, g, b = 0;

Adafruit_NeoTrellis t_array[Y_DIM / 4][X_DIM / 4] = {

  { Adafruit_NeoTrellis(0x30), Adafruit_NeoTrellis(0x2E) },
  { Adafruit_NeoTrellis(0x2F), Adafruit_NeoTrellis(0x31) }

};

Adafruit_MultiTrellis trellis((Adafruit_NeoTrellis *)t_array, Y_DIM / 4, X_DIM / 4);

const char* save_file = "/M4SEQ32/saved_sequences.json";
const char* save_file2 = "/M4SEQ32/saved_velocities.json";
const char* save_file3 = "/M4SEQ32/saved_settings.json";
const char* save_file4 = "/M4SEQ32/saved_probabilities.json";
const int track_notes[] = {36,37,38,39,40,41,42,43}; // C2 thru G2 
const int ctrl_notes[] = {48,49,50,51};

// factory reset patterns and maps...
int seq1[] = {
  1, 0, 0, 0, 0, 0, 0, 0,
  1, 0, 0, 0, 0, 0, 0, 0,
  1, 0, 0, 0, 0, 0, 0, 0,
  1, 0, 0, 0, 0, 0, 0, 1
};
int vel1[] = {
  127,40,80,40,80,40,80,40,
  127,40,80,40,80,40,80,40,
  127,40,80,40,80,40,80,40,
  127,40,80,40,80,40,80,40
};
int prob1[] = {
  10,7,5,4,10,7,5,2,
  10,10,10,10,10,10,10,10,
  10,10,10,10,10,10,10,10,
  10,10,10,10,10,10,10,10
};
int seq2[] = {
  0, 0, 0, 0, 1, 0, 0, 0,
  0, 0, 0, 0, 1, 0, 0, 0,
  0, 0, 0, 0, 1, 0, 0, 0,
  0, 0, 0, 0, 1, 0, 0, 1
};
int prob2[] = {
  10,10,10,10,10,10,10,10,
  10,10,10,10,10,10,10,10,
  10,10,10,10,10,10,10,10,
  10,10,10,10,10,10,10,10
};
int vel2[] = {
  80,40,80,40,127,40,80,40,
  80,40,80,40,127,40,80,40,
  80,40,80,40,127,40,80,40,
  80,40,80,40,127,40,80,40
};
int seq3[] = {
  0, 1, 1, 1, 0, 1, 1, 1, 
  0, 1, 1, 1, 0, 1, 1, 1, 
  0, 1, 1, 1, 0, 1, 1, 1, 
  0, 1, 1, 1, 0, 1, 1, 1
};
int prob3[] = {
  10,10,10,10,10,10,10,10,
  10,10,10,10,10,10,10,10,
  10,10,10,10,10,10,10,10,
  10,10,10,10,10,10,10,10
};
int vel3[] = {
  40,80,127,40,40,80,127,40,
  40,80,127,40,40,80,127,40,
  40,80,127,40,40,80,127,40,
  40,80,127,40,40,80,127,40
};
int seq4[] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0
};
int vel4[] = {
  127,40,80,40,80,40,80,40,
  127,40,80,40,80,40,80,40,
  127,40,80,40,80,40,80,40,
  127,40,80,40,80,40,80,40
};
int prob4[] = {
  10,10,10,10,10,10,10,10,
  10,10,10,10,10,10,10,10,
  10,10,10,10,10,10,10,10,
  10,10,10,10,10,10,10,10
};
int seq5[] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0
};
int vel5[] = {
  127,40,80,40,80,40,80,40,
  127,40,80,40,80,40,80,40,
  127,40,80,40,80,40,80,40,
  127,40,80,40,80,40,80,40
};
int prob5[] = {
  10,10,10,10,10,10,10,10,
  10,10,10,10,10,10,10,10,
  10,10,10,10,10,10,10,10,
  10,10,10,10,10,10,10,10
};
int seq6[] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0
};
int vel6[] = {
  127,40,80,40,80,40,80,40,
  127,40,80,40,80,40,80,40,
  127,40,80,40,80,40,80,40,
  127,40,80,40,80,40,80,40
};
int prob6[] = {
  10,10,10,10,10,10,10,10,
  10,10,10,10,10,10,10,10,
  10,10,10,10,10,10,10,10,
  10,10,10,10,10,10,10,10
};
int seq7[] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0
};
int vel7[] = {
  127,40,80,40,80,40,80,40,
  127,40,80,40,80,40,80,40,
  127,40,80,40,80,40,80,40,
  127,40,80,40,80,40,80,40
};
int prob7[] = {
  10,10,10,10,10,10,10,10,
  10,10,10,10,10,10,10,10,
  10,10,10,10,10,10,10,10,
  10,10,10,10,10,10,10,10
};
int seq8[] = {
  1, 1, 1, 1, 1, 1, 1, 1, 
  1, 1, 1, 1, 1, 1, 1, 1, 
  1, 1, 1, 1, 1, 1, 1, 1, 
  1, 1, 1, 1, 1, 1, 1, 1
};
int vel8[] = {
  127,40,80,40,80,40,80,40,
  127,40,80,40,80,40,80,40,
  127,40,80,40,80,40,80,40,
  127,40,80,40,80,40,80,40
};
int prob8[] = {
  10,10,10,10,10,10,10,10,
  10,10,10,10,10,10,10,10,
  10,10,10,10,10,10,10,10,
  10,10,10,10,10,10,10,10
};
int mutes[] = {0,0,0,0,0,0,0,0};
int currstep = 0;
int laststep = 0;
int patedit = 1;
int veledit = 0;
int lenedit = 0;
int probedit = 0;
int transpose = 0;
int run = 0;
int sel_track = 1;

#include "Sequencer.h"

typedef struct {
  int  step_size;  // aka "seqr.ticks_per_step"
  int  midi_chan;
  int  midi_velocity;
  bool midi_send_clock;
  bool midi_forward_usb;
} Config;

Config cfg  = {
  .step_size = SIXTEENTH_NOTE,
  //.step_size = EIGHTH_NOTE,
  .midi_chan = 1,
  .midi_velocity = 80,
  .midi_send_clock = true,
  .midi_forward_usb = true
};

const bool midi_out_debug = false;
const bool midi_in_debug = false;
const bool marci_debug = false;

const int numseqs = 8;
int length = 32;

// end hardware definitions

Adafruit_USBD_MIDI usb_midi;                                  // USB MIDI object
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDIusb);  // USB MIDI

float tempo = 120;
StepSequencer seqr;
Step sequences[numseqs][numsteps];

uint8_t midiclk_cnt = 0;
uint32_t midiclk_last_micros = 0;

#include "saveload.h" /// FIXME:

//
// -- MIDI sending & receiving functions
//


// callback used by Sequencer to trigger note on
void send_note_on(uint8_t note, uint8_t vel, uint8_t gate, bool on) {
  if (on) {
    MIDIusb.sendNoteOn(note, vel, cfg.midi_chan);
  }
  if (midi_out_debug) { Serial.printf("noteOn:  %d %d %d %d\n", note, vel, gate, on); }
}

// callback used by Sequencer to trigger note off
void send_note_off(uint8_t note, uint8_t vel, uint8_t gate, bool on) {
  if (on) {
    MIDIusb.sendNoteOff(note, vel, cfg.midi_chan);
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

// void handle_midi_in_songpos(unsigned int beats) {
//     Serial.printf("songpos:%d\n", beats);
//     if( beats == 0 ) {
//         seqr.stepi = 0;
//     }
// }

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
  //const int fscale = 230; // out of 255, 230/256 = .9 filter
  // once every ticks_per_step, play note (24 ticks per quarter note => 6 ticks per 16th note)
  if (midiclk_cnt % seqr.ticks_per_step == 0) {  // ticks_per_step = 6 for 16th note
    seqr.trigger_ext(now_micros);  // FIXME: figure out 2nd arg (was step_millis)
  }
  midiclk_cnt++;
  // once every quarter note, calculate new BPM, but be a little cautious about it
  if( midiclk_cnt == ticks_per_quarternote ) {
      uint32_t new_tick_micros = (now_micros - midiclk_last_micros) / ticks_per_quarternote;
      if( new_tick_micros > seqr.tick_micros/2 && new_tick_micros < seqr.tick_micros*2 ) {
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
        handle_midi_in_start();  break;
      case midi::Stop:
        handle_midi_in_stop();  break;
      case midi::Clock:
        handle_midi_in_clock();  break;
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

// Input a value 0 to 255 to get a color value.
// The colors are a transition r - g - b - back to r.
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

void show_sequence(int seq) {
  patedit = 1;
  switch (seq) {
    case 1:
      for (int i = 0; i < 32; i++) {
        trellis.setPixelColor(i,seq1[i] > 0 ? RED : W10);
      }
      break;
    case 2:
      for (int i = 0; i < 32; i++) {
        trellis.setPixelColor(i,seq2[i] > 0 ? ORANGE : W10);
      }
      break;
    case 3:
      for (int i = 0; i < 32; i++) {
        trellis.setPixelColor(i,seq3[i] > 0 ? YELLOW : W10);
      }
      break;
    case 4:
      for (int i = 0; i < 32; i++) {
        trellis.setPixelColor(i,seq4[i] > 0 ? GREEN : W10);
      }
      break;
    case 5:
      for (int i = 0; i < 32; i++) {
        trellis.setPixelColor(i,seq5[i] > 0 ? CYAN : W10);
      }
      break;
    case 6:
      for (int i = 0; i < 32; i++) {
        trellis.setPixelColor(i,seq6[i] > 0 ? BLUE : W10);
      }
      break;
    case 7:
      for (int i = 0; i < 32; i++) {
        trellis.setPixelColor(i,seq7[i] > 0 ? PURPLE : W10);
      }
      break;
    case 8:
      for (int i = 0; i < 32; i++) {
        trellis.setPixelColor(i,seq8[i] > 0 ? PINK : W10);
      }
      break;
  }
  if (run == 0) {trellis.show();} 
}

void show_probabilities(int seq) {
  probedit = 1;
  int col = 0;
  switch (seq){
    case 1:
      for (int i = 0; i < 32; i++) {
        col = prob1[i] == 10 ? RED : Wheel(prob1[i]*10);
        trellis.setPixelColor(i,col);
      }
      break;
    case 2:
      for (int i = 0; i < 32; i++) {
        col = prob2[i] == 10 ? ORANGE : Wheel(prob2[i]*10);
        trellis.setPixelColor(i,col);
      }
      break;
    case 3:
      for (int i = 0; i < 32; i++) {
        col = prob3[i] == 10 ? YELLOW : Wheel(prob3[i]*10);
        trellis.setPixelColor(i,col);
      }
      break;
    case 4:
      for (int i = 0; i < 32; i++) {
        col = prob4[i] == 10 ? GREEN : Wheel(prob4[i]*10);
        trellis.setPixelColor(i,col);
      }
      break;
    case 5:
      for (int i = 0; i < 32; i++) {
        col = prob5[i] == 10 ? CYAN : Wheel(prob5[i]*10);
        trellis.setPixelColor(i,col);
      }
      break;
    case 6:
      for (int i = 0; i < 32; i++) {
        col = prob6[i] == 10 ? BLUE : Wheel(prob6[i]*10);
        trellis.setPixelColor(i,col);
      }
      break;
    case 7:
      for (int i = 0; i < 32; i++) {
        col = prob7[i] == 10 ? PURPLE : Wheel(prob7[i]*10);
        trellis.setPixelColor(i,col);
      }
      break;
    case 8:
      for (int i = 0; i < 32; i++) {
        col = prob8[i] == 10 ? PINK : Wheel(prob8[i]*10);
        trellis.setPixelColor(i,col);
      }
      break;
  }
  if (run == 0) {trellis.show();}
}

void show_accents(int seq) {
  veledit = 1;
  int col = 0;
  switch (seq) {
    case 1:
      for (int i = 0; i < 32; i++) {
        switch (vel1[i]){
          case 0:
            col = W10;
            break;
          case 40:
            col = R40;
            break;
          case 80:
            col = R80;
            break;
          default:
            col = R127;
            break;
        }
        trellis.setPixelColor(i,col);
      }
      break;
    case 2:
      for (int i = 0; i < 32; i++) {
        switch (vel2[i]){
          case 0:
            col = W10;
            break;
          case 40:
            col = O40;
            break;
          case 80:
            col = O80;
            break;
          default:
            col = O127;
            break;
        }
        trellis.setPixelColor(i,col);
      }
      break;
    case 3:
      for (int i = 0; i < 32; i++) {
        switch (vel3[i]){
          case 0:
            col = W10;
            break;
          case 40:
            col = Y40;
            break;
          case 80:
            col = Y80;
            break;
          default:
            col = Y127;
            break;
        }
        trellis.setPixelColor(i,col);
      }
      break;
    case 4:
      for (int i = 0; i < 32; i++) {
        switch (vel4[i]){
          case 0:
            col = W10;
            break;
          case 40:
            col = G40;
            break;
          case 80:
            col = G80;
            break;
          default:
            col = G127;
            break;
        }
        trellis.setPixelColor(i,col);
      }
      break;
    case 5:
      for (int i = 0; i < 32; i++) {
        switch (vel5[i]){
          case 0:
            col = W10;
            break;
          case 40:
            col = C40;
            break;
          case 80:
            col = C80;
            break;
          default:
            col = C127;
            break;
        }
        trellis.setPixelColor(i,col);
      }
      break;
    case 6:
      for (int i = 0; i < 32; i++) {
        switch (vel6[i]){
          case 0:
            col = W10;
            break;
          case 40:
            col = B40;
            break;
          case 80:
            col = B80;
            break;
          default:
            col = B127;
            break;
        }
        trellis.setPixelColor(i,col);
      }
      break;
    case 7:
      for (int i = 0; i < 32; i++) {
        switch (vel7[i]){
          case 0:
            col = W10;
            break;
          case 40:
            col = P40;
            break;
          case 80:
            col = P80;
            break;
          default:
            col = P127;
            break;
        }
        trellis.setPixelColor(i,col);
      }
      break;
    case 8:
      for (int i = 0; i < 32; i++) {
        switch (vel8[i]){
          case 0:
            col = W10;
            break;
          case 40:
            col = PK40;
            break;
          case 80:
            col = PK80;
            break;
          default:
            col = PK127;
            break;
        }
        trellis.setPixelColor(i,col);
      }
      break;
    default:
      break;
  }
  if (run == 0) {trellis.show();}
}

TrellisCallback onKey(keyEvent evt) {
  auto const now = millis();
  auto const keyId = evt.bit.NUM;
  if (marci_debug) {Serial.println(keyId);}
  switch (evt.bit.EDGE)
  {
    case SEESAW_KEYPAD_EDGE_RISING:
      if (lenedit == 1 && keyId < 32) {
        length = keyId+1;
        trellis.setPixelColor(keyId,C127);
        configure_sequencer();
      } else if (probedit == 1 && keyId < 32) {
        switch (sel_track){
          case 1:
            if (prob1[keyId] == 10) {
              prob1[keyId] = 1;
            } else {
              prob1[keyId] += 1;
            }
            trellis.setPixelColor(keyId, prob1[keyId]*10 == 100 ? RED : Wheel(prob1[keyId]*10));
            break;
          case 2:
            if (prob2[keyId] == 10) {
              prob2[keyId] = 1;
            } else {
              prob2[keyId] += 1;
            }
            trellis.setPixelColor(keyId, prob2[keyId]*10 == 100 ? ORANGE : Wheel(prob2[keyId]*10));
            break;
          case 3:
            if (prob3[keyId] == 10) {
              prob3[keyId] = 1;
            } else {
              prob3[keyId] += 1;
            }
            trellis.setPixelColor(keyId, prob3[keyId]*10 == 100 ? YELLOW : Wheel(prob3[keyId]*10));
            break;
          case 4:
            if (prob4[keyId] == 10) {
              prob4[keyId] = 1;
            } else {
              prob4[keyId] += 1;
            }
            trellis.setPixelColor(keyId, prob4[keyId]*10 == 100 ? GREEN : Wheel(prob4[keyId]*10));
            break;
          case 5:
            if (prob5[keyId] == 10) {
              prob5[keyId] = 1;
            } else {
              prob5[keyId] += 1;
            }
            trellis.setPixelColor(keyId, prob5[keyId]*10 == 100 ? CYAN : Wheel(prob5[keyId]*10));
            break;
          case 6:
            if (prob6[keyId] == 10) {
              prob6[keyId] = 1;
            } else {
              prob6[keyId] += 1;
            }
            trellis.setPixelColor(keyId, prob6[keyId]*10 == 100 ? BLUE : Wheel(prob6[keyId]*10));
            break;
          case 7:
            if (prob7[keyId] == 10) {
              prob7[keyId] = 1;
            } else {
              prob7[keyId] += 1;
            }
            trellis.setPixelColor(keyId, prob7[keyId]*10 == 100 ? PURPLE : Wheel(prob7[keyId]*10));
            break;
          case 8:
            if (prob8[keyId] == 10) {
              prob8[keyId] = 1;
            } else {
              prob8[keyId] += 1;
            }
            trellis.setPixelColor(keyId, prob8[keyId]*10 == 100 ? PINK : Wheel(prob8[keyId]*10));
            break;
        }
        if (run == 0){trellis.show();}
      } else if (veledit == 1 & keyId < 32) {
        switch (sel_track){
          case 0:
            break;
          case 1:
            switch (vel1[keyId]){
              case 127:
                trellis.setPixelColor(keyId,R40);
                vel1[keyId] = 40;
                break;
              case 80:
                trellis.setPixelColor(keyId,R127);
                vel1[keyId] = 127;
                break;
              case 40:
                trellis.setPixelColor(keyId,R80);
                vel1[keyId] = 80;
                break;
              default:
                break;
            }
            break;
          case 2:
            switch (vel2[keyId]){
              case 127:
                trellis.setPixelColor(keyId,O40);
                vel2[keyId] = 40;
                break;
              case 80:
                trellis.setPixelColor(keyId,O127);
                vel2[keyId] = 127;
                break;
              case 40:
                trellis.setPixelColor(keyId,O80);
                vel2[keyId] = 80;
                break;
              default:
                break;
            }
            break;
          case 3:
            switch (vel3[keyId]){
              case 127:
                trellis.setPixelColor(keyId,Y40);
                vel3[keyId] = 40;
                break;
              case 80:
                trellis.setPixelColor(keyId,Y127);
                vel3[keyId] = 127;
                break;
              case 40:
                trellis.setPixelColor(keyId,Y80);
                vel3[keyId] = 80;
                break;
              default:
                break;
            }
            break;
          case 4:
            switch (vel4[keyId]){
              case 127:
                trellis.setPixelColor(keyId,G40);
                vel4[keyId] = 40;
                break;
              case 80:
                trellis.setPixelColor(keyId,G127);
                vel4[keyId] = 127;
                break;
              case 40:
                trellis.setPixelColor(keyId,G80);
                vel4[keyId] = 80;
                break;
              default:
                break;
            }
            break;
          case 5:
            switch (vel5[keyId]){
              case 127:
                trellis.setPixelColor(keyId,C40);
                vel5[keyId] = 40;
                break;
              case 80:
                trellis.setPixelColor(keyId,C127);
                vel5[keyId] = 127;
                break;
              case 40:
                trellis.setPixelColor(keyId,C80);
                vel5[keyId] = 80;
                break;
              default:
                break;
            }
            break;
          case 6:
            switch (vel6[keyId]){
              case 127:
                trellis.setPixelColor(keyId,B40);
                vel6[keyId] = 40;
                break;
              case 80:
                trellis.setPixelColor(keyId,B127);
                vel6[keyId] = 127;
                break;
              case 40:
                trellis.setPixelColor(keyId,B80);
                vel6[keyId] = 80;
                break;
              default:
                break;
            }
            break;
          case 7:
            switch (vel7[keyId]){
              case 127:
                trellis.setPixelColor(keyId,P40);
                vel7[keyId] = 40;
                break;
              case 80:
                trellis.setPixelColor(keyId,P127);
                vel7[keyId] = 127;
                break;
              case 40:
                trellis.setPixelColor(keyId,P80);
                vel7[keyId] = 80;
                break;
              default:
                break;
            }
            break;
          case 8:
            switch (vel8[keyId]){
              case 127:
                trellis.setPixelColor(keyId,PK40);
                vel8[keyId] = 40;
                break;
              case 80:
                trellis.setPixelColor(keyId,PK127);
                vel8[keyId] = 127;
                break;
              case 40:
                trellis.setPixelColor(keyId,PK80);
                vel8[keyId] = 80;
                break;
              default:
                break;
            }
            break;
          default:
            break;
        }
      } else if (keyId < 32) {
        switch (sel_track){
          case 0:
            break;
          case 1:
            switch (seq1[keyId]){
              case 1:
                trellis.setPixelColor(keyId,W10);
                seq1[keyId] = 0;
                break;
              case 0:
                trellis.setPixelColor(keyId,RED);
                seq1[keyId] = 1;
                break;
            }
            break;
          case 2:
            switch (seq2[keyId]){
              case 1:
                trellis.setPixelColor(keyId,W10);
                seq2[keyId] = 0;
                break;
              case 0:
                trellis.setPixelColor(keyId,ORANGE);
                seq2[keyId] = 1;
                break;
            }
            break;
          case 3:
            switch (seq3[keyId]){
              case 1:
                trellis.setPixelColor(keyId,W10);
                seq3[keyId] = 0;
                break;
              case 0:
                trellis.setPixelColor(keyId,YELLOW);
                seq3[keyId] = 1;
                break;
            }
            break;
          case 4:
            switch (seq4[keyId]){
              case 1:
                trellis.setPixelColor(keyId,W10);
                seq4[keyId] = 0;
                break;
              case 0:
                trellis.setPixelColor(keyId,GREEN);
                seq4[keyId] = 1;
                break;
            }
            break;
          case 5:
            switch (seq5[keyId]){
              case 1:
                trellis.setPixelColor(keyId,W10);
                seq5[keyId] = 0;
                break;
              case 0:
                trellis.setPixelColor(keyId,CYAN);
                seq5[keyId] = 1;
                break;
            }
            break;
          case 6:
            switch (seq6[keyId]){
              case 1:
                trellis.setPixelColor(keyId,W10);
                seq6[keyId] = 0;
                break;
              case 0:
                trellis.setPixelColor(keyId,BLUE);
                seq6[keyId] = 1;
                break;
            }
            break;
          case 7:
            switch (seq7[keyId]){
              case 1:
                trellis.setPixelColor(keyId,W10);
                seq7[keyId] = 0;
                break;
              case 0:
                trellis.setPixelColor(keyId,PURPLE);
                seq7[keyId] = 1;
                break;
            }
            break;
          case 8:
            switch (seq8[keyId]){
              case 1:
                trellis.setPixelColor(keyId,W10);
                seq8[keyId] = 0;
                break;
              case 0:
                trellis.setPixelColor(keyId,PINK);
                seq8[keyId] = 1;
                break;
            }
            break;
          default:
            break;
        }
      } else {
        switch (keyId){
          case 32:
            sel_track = 1;
            if (probedit == 1) {
              patedit = 0;
              veledit = 0;
              show_probabilities(sel_track);
            } else if (veledit == 1){
              patedit = 0;
              probedit = 0;
              show_accents(sel_track);
            } else {
              patedit = 1;
              probedit = 0;
              veledit = 0;
              show_sequence(sel_track);
            }
            break;
          case 33:
            sel_track = 2;
            if (probedit == 1) {
              show_probabilities(sel_track);
            } else if (veledit == 1){
              patedit = 0;
              probedit = 0;
              show_accents(sel_track);
            } else {
              patedit = 1;
              probedit = 0;
              veledit = 0;
              show_sequence(sel_track);
            }
            break;
          case 34:
            sel_track = 3;
            if (probedit == 1) {
              show_probabilities(sel_track);
            } else if (veledit == 1){
              patedit = 0;
              probedit = 0;
              show_accents(sel_track);
            } else {
              patedit = 1;
              probedit = 0;
              veledit = 0;
              show_sequence(sel_track);
            }
            break;
          case 35:
            sel_track = 4;
            if (probedit == 1) {
              show_probabilities(sel_track);
            } else if (veledit == 1){
              patedit = 0;
              probedit = 0;
              show_accents(sel_track);
            } else {
              patedit = 1;
              probedit = 0;
              veledit = 0;
              show_sequence(sel_track);
            }
            break;
          case 36:
            sel_track = 5;
            if (probedit == 1) {
              show_probabilities(sel_track);
            } else if (veledit == 1){
              patedit = 0;
              probedit = 0;
              show_accents(sel_track);
            } else {
              patedit = 1;
              probedit = 0;
              veledit = 0;
              show_sequence(sel_track);
            }
            break;
          case 37:
            sel_track = 6;
            if (probedit == 1) {
              show_probabilities(sel_track);
            } else if (veledit == 1){
              patedit = 0;
              probedit = 0;
              show_accents(sel_track);
            } else {
              patedit = 1;
              probedit = 0;
              veledit = 0;
              show_sequence(sel_track);
            }
            break;
          case 38:
            sel_track = 7;
            if (probedit == 1) {
              show_probabilities(sel_track);
            } else if (veledit == 1){
              patedit = 0;
              probedit = 0;
              show_accents(sel_track);
            } else {
              patedit = 1;
              probedit = 0;
              veledit = 0;
              show_sequence(sel_track);
            }
            break;
          case 39:
            sel_track = 8;
            if (probedit == 1) {
              show_probabilities(sel_track);
            } else if (veledit == 1){
              patedit = 0;
              probedit = 0;
              show_accents(sel_track);
            } else {
              patedit = 1;
              probedit = 0;
              veledit = 0;
              show_sequence(sel_track);
            }
            break;
          case 40:
            if (mutes[0] == 0){
              mutes[0] = 1;
              trellis.setPixelColor(40,R80);
            } else {
              mutes[0] = 0;
              trellis.setPixelColor(40,G80);
            }
            break;
          case 41:
            if (mutes[1] == 0){
              mutes[1] = 1;
              trellis.setPixelColor(41,R80);
            } else {
              mutes[1] = 0;
              trellis.setPixelColor(41,G80);
            }
            break;
          case 42:
            if (mutes[2] == 0){
              mutes[2] = 1;
              trellis.setPixelColor(42,R80);
            } else {
              mutes[2] = 0;
              trellis.setPixelColor(42,G80);
            }
            break;
          case 43:
            if (mutes[3] == 0){
              mutes[3] = 1;
              trellis.setPixelColor(43,R80);
            } else {
              mutes[3] = 0;
              trellis.setPixelColor(43,G80);
            }
            break;
          case 44:
            if (mutes[4] == 0){
              mutes[4] = 1;
              trellis.setPixelColor(44,R80);
            } else {
              mutes[4] = 0;
              trellis.setPixelColor(44,G80);
            }
            break;
          case 45:
            if (mutes[5] == 0){
              mutes[5] = 1;
              trellis.setPixelColor(45,R80);
            } else {
              mutes[5] = 0;
              trellis.setPixelColor(45,G80);
            }
            break;
          case 46:
            if (mutes[6] == 0){
              mutes[6] = 1;
              trellis.setPixelColor(46,R80);
            } else {
              mutes[6] = 0;
              trellis.setPixelColor(46,G80);
            }
            break;
          case 47:
            if (mutes[7] == 0){
              mutes[7] = 1;
              trellis.setPixelColor(47,R80);
            } else {
              mutes[7] = 0;
              trellis.setPixelColor(47,G80);
            }
            break;
          case 48:
            if (patedit == 0) {
              patedit = 1;
              veledit = 0;
              probedit = 0;
              trellis.setPixelColor(48,R127);
              trellis.setPixelColor(49,Y40);
              trellis.setPixelColor(50,P40);
              show_sequence(sel_track);
            }
            break;
          case 49:
            if (veledit == 0) {
              veledit = 1;
              patedit = 0;
              probedit = 0;
              trellis.setPixelColor(48,R40);
              trellis.setPixelColor(49,Y127);
              trellis.setPixelColor(50,P40);
              show_accents(sel_track);
            }
            break;
          case 50:
            if (probedit == 0) {
              probedit = 1;
              patedit = 0;
              veledit = 0;
              trellis.setPixelColor(48,R40);
              trellis.setPixelColor(49,Y40);
              trellis.setPixelColor(50,P127);
              show_probabilities(sel_track);
            }
            break;
          case 53:
            if (run == 0) {
              switch (transpose) {
                case 0:
                  transpose = 12;
                  trellis.setPixelColor(53,B80);
                  break;
                case 12:
                  transpose = 24;
                  trellis.setPixelColor(53,B127);
                  break;
                case 24:
                  transpose = 0;
                  trellis.setPixelColor(53,B40);
                  break;
                default:
                  break;
              }
              configure_sequencer();
            }
            break;
          case 54:
            if (lenedit == 0) {
              lenedit = 1;
              trellis.setPixelColor(54,G127);
            } else {
              lenedit = 0;
              trellis.setPixelColor(54,G40);
            }
            break;
          case 55:
            switch(cfg.step_size){
              case SIXTEENTH_NOTE:
                trellis.setPixelColor(55,O40);
                cfg.step_size = QUARTER_NOTE;
                break;
              case QUARTER_NOTE:
                trellis.setPixelColor(55,O80);
                cfg.step_size = EIGHTH_NOTE;
                break;
              case EIGHTH_NOTE:
                trellis.setPixelColor(55,O127);
                cfg.step_size = SIXTEENTH_NOTE;
                break;
              default:
                break;
            }
            configure_sequencer();
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
            sequences_write();
            break;
          case 60:
            pattern_reset();
            break;
          case 61:
            if (cfg.midi_send_clock == true){
              cfg.midi_send_clock = false;
              trellis.setPixelColor(61, Y40);
            } else {
              cfg.midi_send_clock = true;
              trellis.setPixelColor(61, Y127);
            }
            configure_sequencer();
            break;
          case 62:
            tempo = tempo - 1;
            configure_sequencer();
            break;
          case 63:
            tempo = tempo + 1;
            configure_sequencer();
            break;
          default:
            break;
        }
      }
      break;
    case SEESAW_KEYPAD_EDGE_FALLING:
      //trellis.setPixelColor(evt.bit.NUM, OFF); //off falling
      break;
  }
  return nullptr;
}

void init_flash(){
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

void init_interface(){
  //Seq 1 > 8
  trellis.setPixelColor(32,RED);
  trellis.setPixelColor(33,ORANGE);
  trellis.setPixelColor(34,YELLOW);
  trellis.setPixelColor(35,GREEN);
  trellis.setPixelColor(36,CYAN);
  trellis.setPixelColor(37,BLUE);
  trellis.setPixelColor(38,PURPLE);
  trellis.setPixelColor(39,PINK);
  //Vel 1 > 8
  trellis.setPixelColor(40,G40);
  trellis.setPixelColor(41,G40);
  trellis.setPixelColor(42,G40);
  trellis.setPixelColor(43,G40);
  trellis.setPixelColor(44,G40);
  trellis.setPixelColor(45,G40);
  trellis.setPixelColor(46,G40);
  trellis.setPixelColor(47,G40);
  //
  trellis.setPixelColor(48,R127);
  trellis.setPixelColor(49,Y40);
  trellis.setPixelColor(50,P40);
  switch (transpose) {
    case 0:
      trellis.setPixelColor(53,B40);
      break;
    case 12:
      trellis.setPixelColor(53,B80);
      break;
    case 24:
      trellis.setPixelColor(53,B127);
      break;
    default:
      break;
  }
  trellis.setPixelColor(54,G40);
  switch(cfg.step_size){
    case SIXTEENTH_NOTE:
      trellis.setPixelColor(55,O127);
      break;
    case QUARTER_NOTE:
      trellis.setPixelColor(55,O40);
      break;
    case EIGHTH_NOTE:
      trellis.setPixelColor(55,O80);
      break;
    default:
      break;
  }
  trellis.setPixelColor(55,O127);  
  //Control Row
  trellis.setPixelColor(56,GREEN);
  trellis.setPixelColor(57,RED);
  trellis.setPixelColor(58,ORANGE);
  trellis.setPixelColor(59,BLUE);
  trellis.setPixelColor(60,CYAN);
  trellis.setPixelColor(61,YELLOW);
  trellis.setPixelColor(62,RED);
  trellis.setPixelColor(63,GREEN);

  for (int y = 0; y < Y_DIM; y++) {
    for (int x = 0; x < X_DIM; x++) {
      //activate rising and falling edges on all keys
      trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_RISING, true);
      trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_FALLING, true);
      trellis.registerCallback(x, y, onKey);
    }
  }
  trellis.show();
}

////////////////////////////

//
// ---  MIDI in/out setup on core0
//
void setup() {
  TinyUSBDevice.setManufacturerDescriptor("aPatchworkBoy");
  TinyUSBDevice.setProductDescriptor("M4StepSeq");

  MIDIusb.begin();
  MIDIusb.turnThruOff();   // turn off echo

  Serial.begin(115200);

  init_flash();
  
  sequences_read();
  velocities_read();
  settings_read();
  sequence_load(2);
  configure_sequencer();
  delay(100);
  if (!trellis.begin()) {
    Serial.println(F("failed to begin trellis"));
    while (1) delay(1);
  } else {
    Serial.println(F("Init..."));
  }
  init_interface();
  Serial.println(F("GO!"));
  show_sequence(1);
}

//
// --- MIDI in/out on core0
//
void loop() {
  midi_read_and_forward();
  seqr.update();  // will call send_note_{on,off} callbacks
}