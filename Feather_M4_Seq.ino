/**
 * Feather M4 Seq -- An 8 track 32 step MIDI Gate Sequencer for Feather M4 Express & Neotrellis 8x8
 * 04 Nov 2023 - @apatchworkboy / Marci
 * 28 Apr 2023 - original via @todbot / Tod Kurt https://github.com/todbot/picostepseq/
 *
 * Libraries needed (all available via Library Manager):
 * - Adafruit_TinyUSB -- https://github.com/adafruit/Adafruit_TinyUSB_Arduino
 * - MIDI -- https://github.com/FortySevenEffects/arduino_midi_library
 * - Adafruit NeoTrellis -- part of Adafruit SeeSaw -- https://github.com/adafruit/Adafruit_Seesaw
 * - ArduinoJson -- https://arduinojson.org/
 *
 * To upload:
 * - Use Arduino IDE 1.8.19
 * - In "Tools", set "Tools / USB Stack: Adafruit TinyUSB"
 * - Program the sketch to the Feather M4 Express with "Upload"
 *
 **/
 

#include <Adafruit_TinyUSB.h>
#include <Adafruit_NeoTrellis.h>
#include <MIDI.h>
#include <ArduinoJson.h>

#define Y_DIM 8 //number of rows of key
#define X_DIM 8 //number of columns of keys
#define t_size Y_DIM*X_DIM
#define t_length t_size-1
#define maincolor seesaw_NeoPixel::Color(0,0,1)
#define RED seesaw_NeoPixel::Color(155,10,10)
#define ORANGE seesaw_NeoPixel::Color(155,80,10)
#define YELLOW seesaw_NeoPixel::Color(155,155,30)
#define GREEN seesaw_NeoPixel::Color(0,155,0)
#define CYAN seesaw_NeoPixel::Color(40,155,155)
#define BLUE seesaw_NeoPixel::Color(20,50,155)
#define PURPLE seesaw_NeoPixel::Color(100,20,155)
#define PINK seesaw_NeoPixel::Color(155,40,60)
#define OFF seesaw_NeoPixel::Color(0,0,0)
#define W100 seesaw_NeoPixel::Color(100,100,100)
#define W75 seesaw_NeoPixel::Color(75,75,75)
#define W10 seesaw_NeoPixel::Color(10,10,10)
#define PURPLE seesaw_NeoPixel::Color(100,20,155)

Adafruit_NeoTrellis t_array[Y_DIM / 4][X_DIM / 4] = {

  { Adafruit_NeoTrellis(0x30), Adafruit_NeoTrellis(0x2E) },
  { Adafruit_NeoTrellis(0x2F), Adafruit_NeoTrellis(0x31) }

};

Adafruit_MultiTrellis trellis((Adafruit_NeoTrellis *)t_array, Y_DIM / 4, X_DIM / 4);

int seq1[] = {
  1, 0, 0, 0, 0, 0, 0, 0,
  1, 0, 0, 0, 0, 0, 0, 0,
  1, 0, 0, 0, 0, 0, 0, 0,
  1, 0, 0, 0, 0, 0, 0, 1
};
int seq2[] = {
  0, 0, 0, 0, 1, 0, 0, 0,
  0, 0, 0, 0, 1, 0, 0, 0,
  0, 0, 0, 0, 1, 0, 0, 0,
  0, 0, 0, 0, 1, 0, 0, 1
};
int seq3[] = {
  0, 1, 1, 1, 0, 1, 1, 1, 
  0, 1, 1, 1, 0, 1, 1, 1, 
  0, 1, 1, 1, 0, 1, 1, 1, 
  0, 1, 1, 1, 0, 1, 1, 1
};
int seq4[] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0
};
int seq5[] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0
};
int seq6[] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0
};
int seq7[] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0
};
int seq8[] = {
  1, 1, 1, 1, 1, 1, 1, 1, 
  1, 1, 1, 1, 1, 1, 1, 1, 
  1, 1, 1, 1, 1, 1, 1, 1, 
  1, 1, 1, 1, 1, 1, 1, 1
};
int currstep = 0;
int editing = 1;

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
const bool midi_in_debug = true;

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
    MIDIusb.sendNoteOn(note, cfg.midi_velocity, cfg.midi_chan);
  }
  if (midi_out_debug) { Serial.printf("noteOn:  %d %d %d %d\n", note, vel, gate, on); }
}

// callback used by Sequencer to trigger note off
void send_note_off(uint8_t note, uint8_t vel, uint8_t gate, bool on) {
  if (on) {
    MIDIusb.sendNoteOff(note, cfg.midi_velocity, cfg.midi_chan);
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

TrellisCallback onKey(keyEvent evt) {
  auto const now = millis();
  auto const keyId = evt.bit.NUM;
  Serial.println(keyId);
  switch (evt.bit.EDGE)
  {
    case SEESAW_KEYPAD_EDGE_RISING:
      //trellis.setPixelColor(evt.bit.NUM, Wheel(map(evt.bit.NUM, 0, X_DIM * Y_DIM, 0, 255))); //on rising
      if (keyId < 32) {
        switch (editing){
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
            show_sequence(editing);
            break;
          case 33:
            editing = 2;
            show_sequence(editing);
            break;
          case 34:
            editing = 3;
            show_sequence(editing);
            break;
          case 35:
            editing = 4;
            show_sequence(editing);
            break;
          case 36:
            editing = 5;
            show_sequence(editing);
            break;
          case 37:
            editing = 6;
            show_sequence(editing);
            break;
          case 38:
            editing = 7;
            show_sequence(editing);
            break;
          case 39:
            editing = 8;
            show_sequence(editing);
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

////////////////////////////

//
// ---  MIDI in/out setup on core0
//
void setup() {
  TinyUSBDevice.setManufacturerDescriptor("todbot");
  TinyUSBDevice.setProductDescriptor("PicoStepSeq");

  MIDIusb.begin();
  MIDIusb.turnThruOff();   // turn off echo

  Serial.begin(115200);

  sequences_read();
  sequence_load(2);
  configure_sequencer();

  if (!trellis.begin()) {
    Serial.println("failed to begin trellis");
    while (1) delay(1);
  } else {
    Serial.println("Init..."); 
  }
  //Seq 1 > 8
  trellis.setPixelColor(32,RED);
  trellis.setPixelColor(33,ORANGE);
  trellis.setPixelColor(34,YELLOW);
  trellis.setPixelColor(35,GREEN);
  trellis.setPixelColor(36,CYAN);
  trellis.setPixelColor(37,BLUE);
  trellis.setPixelColor(38,PURPLE);
  trellis.setPixelColor(39,PINK);
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
  show_sequence(1);
}

//
// --- MIDI in/out on core0
//
void loop() {
  midi_read_and_forward();
  seqr.update();  // will call send_note_{on,off} callbacks
}
