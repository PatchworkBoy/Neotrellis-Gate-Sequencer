/**
 * sequencer.h -- Sequencer for picostepseq extended for Feather M4 Express & Neotrellis 8x8
 * 04 Nov 2023 - @apatchworkboy / Marci
 * 28 Apr 2023 - @todbot / Tod Kurt
 * 15 Aug 2022 - @todbot / Tod Kurt
 * Part of https://github.com/todbot/picostepseq/
 */

const int numsteps = 32;
const int ticks_per_quarternote = 24; // 24 ppq per midi std
const int steps_per_beat_default = 4; // => ticks_per_step = 24/4 = 6

typedef struct {
  uint8_t note; // midi note
  uint8_t vel;  // midi velocity (always 127 currently)
  uint8_t gate; // how long note should be on in 0-15 arbitrary units
  bool on;      // does this note play or is or muted
} Step;

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

const int valid_step_sizes[] = {QUARTER_NOTE, EIGHTH_NOTE, SIXTEENTH_NOTE};
const int valid_step_sizes_cnt = 3;

typedef void (*TriggerFunc)(uint8_t note, uint8_t vel, uint8_t gate, bool on);
typedef void (*ClockFunc)(clock_type_t type); // , int pos);

// stubs for when Sequencer object is only partially initialized
void fake_note_callback(uint8_t note, uint8_t vel, uint8_t gate, bool on) {  }
void fake_clock_callback(clock_type_t type) { }

class StepSequencer {
public:
  uint32_t tick_micros; // "micros_per_tick", microsecs per clock (6 clocks / step; 4 steps / quarternote)
  uint32_t last_tick_micros; // only change in update()
  uint32_t held_gate_millis;
  int ticks_per_step;  // expected values: 6 = 1/16th, 12 = 1/8, 24 = 1/4,
  int ticki; // which midi clock we're on
  int stepi; // which sequencer step we're on
  int seqno;
  int length;
  int transpose;
  bool playing;
  bool send_clock;
  uint32_t extclk_micros; // 0 = internal clock, non-zero = external clock
  TriggerFunc on_func;
  TriggerFunc off_func;
  ClockFunc clk_func;
  Step held_note;
  Step steps[numsteps];

  StepSequencer( float atempo=120, uint8_t aseqno=0 ) {
    transpose = 0;
    last_tick_micros = 0;
    stepi = 0;
    ticki = 0;
    ticks_per_step = 6; // 6 = 1/16th, 12 = 1/8, 24 = 1/4,
    seqno = aseqno;
    length = 32;
    playing = false;
    extclk_micros = 0;
    send_clock = false;
    set_tempo(atempo);
    on_func = fake_note_callback;
    off_func = fake_note_callback;
    clk_func = fake_clock_callback;
  }

  // get tempo as floating point, computed dynamically from ticks_micros
  float tempo() {
    return 60 * 1000 * 1000 / tick_micros / ticks_per_quarternote; //steps_per_beat * ticks_per_step);
  }

  // set tempo as floating point, computes ticks_micros
  void set_tempo(float bpm) {
    tick_micros = 60 * 1000 * 1000 / bpm / ticks_per_quarternote;
  }

  void update() {
    uint32_t now_micros = micros();
    if( (now_micros - last_tick_micros) < tick_micros ) { return; }  // not yet
    last_tick_micros = now_micros;
    trellis.read();

    // // serious debug cruft here
    // Serial.printf("up:%2d %d  %6ld %6ld  dt:%6ld t:%6ld last:%6ld\n",
    //               ticki, playing,  held_gate_millis, millis(),
    //               delta_t, tick_micros, last_tick_micros );
    //Serial.printf("up: %ld \t h:%2x/%2x/%1x/%d\n", held_gate_millis,
    //              held_note.note, held_note.vel, held_note.gate, held_note.on);

    // if we have a held note and it's time to turn it off, turn it off
    if( held_gate_millis != 0 && millis() >= held_gate_millis ) {
      held_gate_millis = 0;
      off_func( track_notes[0], 0, 1, true);
      off_func( track_notes[1], 0, 1, true);
      off_func( track_notes[2], 0, 1, true);
      off_func( track_notes[3], 0, 1, true);
      off_func( track_notes[4], 0, 1, true);
      off_func( track_notes[5], 0, 1, true);
      off_func( track_notes[6], 0, 1, true);
      off_func( track_notes[7], 0, 1, true);
      off_func( ctrl_notes[0], 0, 1, true);
      off_func( ctrl_notes[1], 0, 1, true);
      off_func( ctrl_notes[2], 0, 1, true);
      off_func( ctrl_notes[3], 0, 1, true);
    }

    if( send_clock && playing && !extclk_micros ) {
      clk_func( CLOCK );
    }

    if( ticki == 0 ) {
      // do a sequence step (i.e. every "ticks_per_step" ticks)
      if( extclk_micros ) {
        // do nothing, let midi clock trigger notes, but fall back to
        // internal clock if not externally clocked for a while
        if( (now_micros - extclk_micros) > tick_micros * ticks_per_quarternote ) {
          extclk_micros = 0;
          Serial.println("Turning EXT CLOCK off");
        }
      }
      else {  // else internally clocked
        trigger(now_micros, 0); // delta_t);
      }
    }

    // increment our ticks-per-step counter: 0,1,2,3,4,5, 0,1,2,3,4,5, ...
    ticki = (ticki + 1) % ticks_per_step;
  }

  // Trigger step when externally clocked (turns on external clock flag)
  void trigger_ext(uint32_t now_micros) {
    extclk_micros = now_micros;
    trigger(now_micros,0);
  }

  // Trigger step in sequence, when internally clocked
  //void trigger(uint32_t now, uint16_t delta_t) {
  void trigger(uint32_t now_micros, uint16_t delta_t) {
    if( !playing ) { trellis.show(); return; }
    trellis.show();
    (void)delta_t; // silence unused variable
    stepi = (stepi + 1) % length; // go to next step
    currstep = stepi;
    laststep = stepi - 1 < 0 ? length - 1 : stepi - 1;

    int color = 0;
    int hit = 0;
    switch (editing) {
      case 0:
        break;
      case 1:
        hit = seq1[stepi] > 0 ? PURPLE : W100;
        color = seq1[laststep] > 0 ? RED : W10;
        break;
      case 2:
        hit = seq2[stepi] > 0 ? PURPLE : W100;
        color = seq2[laststep] > 0 ? ORANGE : W10;
        break;
      case 3:
        hit = seq3[stepi] > 0 ? PURPLE : W100;
        color = seq3[laststep] > 0 ? YELLOW : W10;
        break;
      case 4:
        hit = seq4[stepi] > 0 ? PURPLE : W100;
        color = seq4[laststep] > 0 ? GREEN : W10;
        break;
      case 5:
        hit = seq5[stepi] > 0 ? PURPLE : W100;
        color = seq5[laststep] > 0 ? CYAN : W10;
        break;
      case 6:
        hit = seq6[stepi] > 0 ? PURPLE : W100;
        color = seq6[laststep] > 0 ? BLUE : W10;
        break;
      case 7:
        hit = seq7[stepi] > 0 ? CYAN : W100;
        color = seq7[laststep] > 0 ? PURPLE : W10;
        break;
      case 8:
        hit = seq8[stepi] > 0 ? BLUE : W100;
        color = seq8[laststep] > 0 ? PINK : W10;
        break;
      default:
        break;
    }
    switch (velocity) {
      case 0:
        break;
      case 1:
        hit = seq1[stepi] > 0 ? PURPLE : W100;
        switch (vel1[laststep]){
          case 0:
            color = W10;
            break;
          case 40:
            color = R40;
            break;
          case 80:
            color = R80;
            break;
          case 127:
            color = R127;
            break;
        }
        break;
      case 2:
        hit = seq2[stepi] > 0 ? PURPLE : W100;
        switch (vel2[laststep]){
          case 0:
            color = W10;
            break;
          case 40:
            color = O40;
            break;
          case 80:
            color = O80;
            break;
          case 127:
            color = O127;
            break;
        }
        break;
      case 3:
        hit = seq3[stepi] > 0 ? PURPLE : W100;
        switch (vel3[laststep]){
          case 0:
            color = W10;
            break;
          case 40:
            color = Y40;
            break;
          case 80:
            color = Y80;
            break;
          case 127:
            color = Y127;
            break;
        }
        break;
      case 4:
        hit = seq4[stepi] > 0 ? PURPLE : W100;
        switch (vel4[laststep]){
          case 0:
            color = W10;
            break;
          case 40:
            color = G40;
            break;
          case 80:
            color = G80;
            break;
          case 127:
            color = G127;
            break;
        }
        break;
      case 5:
        hit = seq5[stepi] > 0 ? PURPLE : W100;
        switch (vel5[laststep]){
          case 0:
            color = W10;
            break;
          case 40:
            color = C40;
            break;
          case 80:
            color = C80;
            break;
          case 127:
            color = C127;
            break;
        }
        break;
      case 6:
        hit = seq6[stepi] > 0 ? PURPLE : W100;
        switch (vel6[laststep]){
          case 0:
            color = W10;
            break;
          case 40:
            color = B40;
            break;
          case 80:
            color = B80;
            break;
          case 127:
            color = B127;
            break;
        }
        break;
      case 7:
        hit = seq7[stepi] > 0 ? PURPLE : W100;
        switch (vel7[laststep]){
          case 0:
            color = W10;
            break;
          case 40:
            color = P40;
            break;
          case 80:
            color = P80;
            break;
          case 127:
            color = P127;
            break;
        }
        break;
      case 8:
        hit = seq8[stepi] > 0 ? PURPLE : W100;
        switch (vel8[laststep]){
          case 0:
            color = W10;
            break;
          case 40:
            color = PK40;
            break;
          case 80:
            color = PK80;
            break;
          case 127:
            color = PK127;
            break;
        }
        break;
      default:
        break;
    }
    trellis.setPixelColor(stepi,hit);
    trellis.setPixelColor(laststep, color);
    if (length < numsteps) trellis.setPixelColor(length-1,C127);
    
    on_func(track_notes[0], vel1[stepi], 5, seq1[stepi] == 1 ? true : false);
    on_func(track_notes[1], vel2[stepi], 5, seq2[stepi] == 1 ? true : false);
    on_func(track_notes[2], vel3[stepi], 5, seq3[stepi] == 1 ? true : false);
    on_func(track_notes[3], vel4[stepi], 5, seq4[stepi] == 1 ? true : false);
    on_func(track_notes[4], vel5[stepi], 5, seq5[stepi] == 1 ? true : false);
    on_func(track_notes[5], vel6[stepi], 5, seq6[stepi] == 1 ? true : false);
    on_func(track_notes[6], vel7[stepi], 5, seq7[stepi] == 1 ? true : false);
    on_func(track_notes[7], vel8[stepi], 5, seq8[stepi] == 1 ? true : false);

    uint32_t micros_per_step = ticks_per_step * tick_micros;
    uint32_t gate_micros = 5 * micros_per_step / 16;  // s.gate is arbitary 0-15 value
    held_gate_millis = (now_micros + gate_micros) / 1000;
  }

  void toggle_play_stop() {
    if( playing ) {
      clear_pos(stepi); 
      stop(); 
      on_func(ctrl_notes[1], 127, 5, true);
      on_func(ctrl_notes[3], 127, 5, true);
    }
    else { 
      play();
      on_func(ctrl_notes[3], 127, 5, true);
      on_func(ctrl_notes[0], 127, 5, true);
    }
  }

  // signal to sequencer/MIDI core we want to start playing
  void play() {
    stepi = -1; // hmm, but goes to 0 on first downbeat
    ticki = 0;
    playing = true;
    if(send_clock && !extclk_micros) {
      clk_func( START );
    }
    on_func(ctrl_notes[3], 127, 5, true);
    on_func(ctrl_notes[0], 127, 5, true);
  }

  // signal to sequencer/MIDI core we want to stop/pause playing
  // FIXME: unclear this works correctly
  void pause() {
    playing = false;
    on_func(ctrl_notes[1], 127, 5, true);
    on_func(ctrl_notes[2], 127, 5, true);
  }

  // signal to sequencer/MIDI core we want to stop playing
  void stop() {
    playing = false;
    clear_pos(stepi);
    stepi = -1;
    if(send_clock && !extclk_micros) {
      clk_func( STOP );
    }
    on_func(ctrl_notes[1], 127, 5, true);
    on_func(ctrl_notes[3], 127, 5, true);
  }

  void reset() {
    stepi = -1;
    ticki = 0;
    on_func(ctrl_notes[3], 127, 5, true);
  }

  void clear_pos(int stepi) {
    int color = 0;
    int hit = 0;
    switch (editing) {
      case 1:
        hit = seq1[stepi] > 0 ? RED : W10;
        break;
      case 2:
        hit = seq2[stepi] > 0 ? ORANGE : W10;
        break;
      case 3:
        hit = seq3[stepi] > 0 ? YELLOW : W10; 
        break;
      case 4:
        hit = seq4[stepi] > 0 ? GREEN : W10;
        break;
      case 5:
        hit = seq5[stepi] > 0 ? CYAN : W10;
        break;
      case 6:
        hit = seq6[stepi] > 0 ? BLUE : W10;
        break;
      case 7:
        hit = seq7[stepi] > 0 ? PURPLE : W10;
        break;
      case 8:
        hit = seq8[stepi] > 0 ? PINK : W10;
        break;
    }
    trellis.setPixelColor(stepi,hit);
    trellis.show();
  }
};
