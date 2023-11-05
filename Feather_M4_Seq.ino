/**
 * Feather M4 Seq -- An 8 track 32 step MIDI Gate Sequencer for Feather M4 Express & Neotrellis 8x8
 * 04 Nov 2023 - @apatchworkboy / Marci
 * 28 Apr 2023 - original via @todbot / Tod Kurt https://github.com/todbot/picostepseq/
 */

#include <Adafruit_TinyUSB.h>
#include <Adafruit_NeoTrellis.h>
#include <MIDI.h>
#include <ArduinoJson.h>

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
const int track_notes[] = {36,37,38,39,40,41,42,43}; // C2 thru G2 
const int ctrl_notes[] = {48,49,50,51};
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
int seq2[] = {
  0, 0, 0, 0, 1, 0, 0, 0,
  0, 0, 0, 0, 1, 0, 0, 0,
  0, 0, 0, 0, 1, 0, 0, 0,
  0, 0, 0, 0, 1, 0, 0, 1
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
int currstep = 0;
int laststep = 0;
int editing = 1;
int velocity = 0;

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
  switch (seq) {
    case 1:
      for (int i = 0; i < 32; i++) {
        trellis.setPixelColor(i,seq1[i] > 0 ? RED : W10);
      }
      trellis.show();
      break;
    case 2:
      for (int i = 0; i < 32; i++) {
        trellis.setPixelColor(i,seq2[i] > 0 ? ORANGE : W10);
      }
      trellis.show();
      break;
    case 3:
      for (int i = 0; i < 32; i++) {
        trellis.setPixelColor(i,seq3[i] > 0 ? YELLOW : W10);
      }
      trellis.show();
      break;
    case 4:
      for (int i = 0; i < 32; i++) {
        trellis.setPixelColor(i,seq4[i] > 0 ? GREEN : W10);
      }
      trellis.show();
      break;
    case 5:
      for (int i = 0; i < 32; i++) {
        trellis.setPixelColor(i,seq5[i] > 0 ? CYAN : W10);
      }
      trellis.show();
      break;
    case 6:
      for (int i = 0; i < 32; i++) {
        trellis.setPixelColor(i,seq6[i] > 0 ? BLUE : W10);
      }
      trellis.show();
      break;
    case 7:
      for (int i = 0; i < 32; i++) {
        trellis.setPixelColor(i,seq7[i] > 0 ? PURPLE : W10);
      }
      trellis.show();
      break;
    case 8:
      for (int i = 0; i < 32; i++) {
        trellis.setPixelColor(i,seq8[i] > 0 ? PINK : W10);
      }
      trellis.show();
      break;
  } 
}

void show_accents(int seq) {
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
      trellis.show();
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
      trellis.show();
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
      trellis.show();
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
      trellis.show();
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
      trellis.show();
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
      trellis.show();
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
      trellis.show();
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
      trellis.show();
      break;
    default:
      break;
  } 
}

TrellisCallback onKey(keyEvent evt) {
  auto const now = millis();
  auto const keyId = evt.bit.NUM;
  if (marci_debug) {Serial.println(keyId);}
  switch (evt.bit.EDGE)
  {
    case SEESAW_KEYPAD_EDGE_RISING:
      //trellis.setPixelColor(evt.bit.NUM, Wheel(map(evt.bit.NUM, 0, X_DIM * Y_DIM, 0, 255))); //on rising
      if (keyId < 32) {
        switch (editing){
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
        switch (velocity){
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
      } else {
        switch (keyId){
          case 62:
            tempo = tempo - 1;
            configure_sequencer();
            break;
          case 63:
            tempo = tempo + 1;
            configure_sequencer();
            break;
          case 56:
            seqr.toggle_play_stop();
            break;
          case 57:
            seqr.stop();
            break;
          case 58:
            seqr.pause();
            break;
          case 59:
            seqr.reset();
            break;
          case 32:
            editing = 1;
            velocity = 0;
            show_sequence(editing);
            break;
          case 33:
            editing = 2;
            velocity = 0;
            show_sequence(editing);
            break;
          case 34:
            editing = 3;
            velocity = 0;
            show_sequence(editing);
            break;
          case 35:
            editing = 4;
            velocity = 0;
            show_sequence(editing);
            break;
          case 36:
            editing = 5;
            velocity = 0;
            show_sequence(editing);
            break;
          case 37:
            editing = 6;
            velocity = 0;
            show_sequence(editing);
            break;
          case 38:
            editing = 7;
            velocity = 0;
            show_sequence(editing);
            break;
          case 39:
            editing = 8;
            velocity = 0;
            show_sequence(editing);
            break;
          case 40:
            editing = 0;
            velocity = 1;
            show_accents(velocity);
            break;
          case 41:
            editing = 0;
            velocity = 2;
            show_accents(velocity);
            break;
          case 42:
            editing = 0;
            velocity = 3;
            show_accents(velocity);
            break;
          case 43:
            editing = 0;
            velocity = 4;
            show_accents(velocity);
            break;
          case 44:
            editing = 0;
            velocity = 5;
            show_accents(velocity);
            break;
          case 45:
            editing = 0;
            velocity = 6;
            show_accents(velocity);
            break;
          case 46:
            editing = 0;
            velocity = 7;
            show_accents(velocity);
            break;
          case 47:
            editing = 0;
            velocity = 8;
            show_accents(velocity);
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

void theaterChase(uint32_t c, uint8_t wait) {
  for (int j = 0; j < 10; j++) { //do 10 cycles of chasing
    for (int q = 0; q < 3; q++) {
      for (uint16_t i = 0; i < t_size; i = i + 3) {
        trellis.setPixelColor(i + q, c); //turn every third pixel on
      }
      trellis.show();
      delay(wait);
      for (uint16_t i = 0; i < t_size; i = i + 3) {
        trellis.setPixelColor(i + q, maincolor); //turn every third pixel off
      }
    }
  }
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
  trellis.setPixelColor(40,R80);
  trellis.setPixelColor(41,O80);
  trellis.setPixelColor(42,Y80);
  trellis.setPixelColor(43,G80);
  trellis.setPixelColor(44,C80);
  trellis.setPixelColor(45,B80);
  trellis.setPixelColor(46,P80);
  trellis.setPixelColor(47,PK80);
  //Control Row
  trellis.setPixelColor(56,GREEN);
  trellis.setPixelColor(57,RED);
  trellis.setPixelColor(58,ORANGE);
  trellis.setPixelColor(59,YELLOW);
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

  sequences_read();
  sequence_load(2);
  configure_sequencer();
  delay(100);
  if (!trellis.begin()) {
    Serial.println("failed to begin trellis");
    while (1) delay(1);
  } else {
    Serial.println("Init..."); 
  }
  
  randomSeed(analogRead(0));
  r = random(0, 256);
  g = random(0, 256);
  b = random(0, 256);
  theaterChase(seesaw_NeoPixel::Color(r, g, b), 50);
  init_interface();
  Serial.println("GO!"); 
  show_sequence(1);
}

//
// --- MIDI in/out on core0
//
void loop() {
  midi_read_and_forward();
  seqr.update();  // will call send_note_{on,off} callbacks
}