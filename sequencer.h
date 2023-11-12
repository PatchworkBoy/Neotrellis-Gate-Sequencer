/**
 * sequencer.h -- Sequencer for picostepseq extended for Feather M4 Express & Neotrellis 8x8
 * 04 Nov 2023 - @apatchworkboy / Marci
 * 28 Apr 2023 - @todbot / Tod Kurt
 * 15 Aug 2022 - @todbot / Tod Kurt
 * Part of https://github.com/todbot/picostepseq/
 */

const int ticks_per_quarternote = 24; // 24 ppq per midi std
const int steps_per_beat_default = 6; // => ticks_per_step = 24/4 = 6

uint32_t seq_col(int seq){
  switch (seq) {
    case 1:
      return RED;
    case 2:
      return ORANGE;
    case 3:
      return YELLOW;
    case 4:
      return GREEN;
    case 5:
      return CYAN;
    case 6:
      return BLUE;
    case 7:
      return PURPLE;
    case 8:
      return PINK;
    default:
      return 0;
  }
}
uint32_t seq_dim(uint8_t seq, uint8_t level){
  switch (seq) {
    case 1:
      return (level <= 20) ? W10 : (level <= 40) ? R40 : (level <= 80) ? R80 : (level <= 127) ? R127 : 0;
    case 2:
      return (level <= 20) ? W10 : (level <= 40) ? O40 : (level <= 80) ? O80 : (level <= 127) ? O127 : 0;
    case 3:
      return (level <= 20) ? W10 : (level <= 40) ? Y40 : (level <= 80) ? Y80 : (level <= 127) ? Y127 : 0;
    case 4:
      return (level <= 20) ? W10 : (level <= 40) ? G40 : (level <= 80) ? G80 : (level <= 127) ? G127 : 0;
    case 5:
      return (level <= 20) ? W10 : (level <= 40) ? C40 : (level <= 80) ? C80 : (level <= 127) ? C127 : 0;
    case 6:
      return (level <= 20) ? W10 : (level <= 40) ? B40 : (level <= 80) ? B80 : (level <= 127) ? B127 : 0;
    case 7:
      return (level <= 20) ? W10 : (level <= 40) ? P40 : (level <= 80) ? P80 : (level <= 127) ? P127 : 0;
    case 8:
      return (level <= 20) ? W10 : (level <= 40) ? PK40 : (level <= 80) ? PK80 : (level <= 127) ? PK127 : 0;
    default:
      return 0;
  }
}

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

typedef void (*TriggerFunc)(uint8_t note, uint8_t vel, uint8_t gate, bool on, uint8_t chan);
typedef void (*ClockFunc)(clock_type_t type); // , int pos);

// stubs for when Sequencer object is only partially initialized
void fake_note_callback(uint8_t note, uint8_t vel, uint8_t gate, bool on, uint8_t chan) {  }
void fake_clock_callback(clock_type_t type) { }

class StepSequencer {
public:
  uint32_t tick_micros; // "micros_per_tick", microsecs per clock (6 clocks / step; 4 steps / quarternote)
  uint8_t tick_pc;
  uint32_t last_tick_micros; // only change in update()
  uint32_t held_gate_millis[8];
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
    tick_pc = tick_micros / 100;
  }
 
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

  void update() {
    uint8_t sw = length > 1 ? swing : 0;
    uint32_t now_micros = micros();
    if((stepi % 2 ? (now_micros - last_tick_micros) < (tick_micros - (tick_pc * sw)):(now_micros - last_tick_micros) < (tick_micros + (tick_pc * sw)))) { return; }  // not yet, with Swing!
    last_tick_micros = now_micros;
    trellis.read();

    // if we have a held note and it's time to turn it off, turn it off
    for (uint8_t i = 0; i < 8; ++i){
      if( held_gate_millis[i] != 0 && millis() >= held_gate_millis[i] ) {
        held_gate_millis[i] = 0;
        off_func( track_notes[i]+transpose, 0, 1, true, track_chan[i]);
      }
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
        trigger(now_micros); // delta_t);
      }
    }

    // increment our ticks-per-step counter: 0,1,2,3,4,5, 0,1,2,3,4,5, ...
    ticki = (ticki + 1) % ticks_per_step;
  }

  // Trigger step when externally clocked (turns on external clock flag)
  void trigger_ext(uint32_t now_micros) {
    extclk_micros = now_micros;
    trigger(now_micros);
  }

  // Trigger step in sequence, when internally clocked
  void trigger(uint32_t now_micros) {
    
    if( !playing ) { trellis.show(); return; }

    stepi = (stepi + 1) % length; // go to next step
    currstep = stepi;
    laststep = stepi - 1 < 0 ? length - 1 : stepi - 1;

    uint32_t color = 0;
    uint32_t hit = 0;
    uint8_t trk_arr = sel_track - 1;
    if (presetmode == 0) {
      if (gateedit == 1){
        hit = gates[presets[trk_arr]][trk_arr][stepi] > 0 ? PURPLE : W100;
        color = gates[presets[trk_arr]][trk_arr][laststep] < 15 ? Wheel(gates[presets[trk_arr]][trk_arr][laststep]*5) : seq_col(sel_track);
      } else if (probedit == 1){
        hit = seqs[presets[trk_arr]][trk_arr][stepi] > 0 ? PURPLE : W100;
        color = probs[presets[trk_arr]][trk_arr][laststep] < 10 ? Wheel(probs[presets[trk_arr]][trk_arr][laststep]*10) : seq_col(sel_track);
      } else if (veledit == 1){
        hit = seqs[presets[trk_arr]][trk_arr][stepi] > 0 ? PURPLE : W100;
        color = seq_dim(sel_track,vels[presets[trk_arr]][trk_arr][laststep]);
      } else {
        hit = seqs[presets[trk_arr]][trk_arr][stepi] > 0 ? PURPLE : W100;
        color = seqs[presets[trk_arr]][trk_arr][laststep] > 0 ? seq_col(sel_track) : W10;
      }
      
      trellis.setPixelColor(stepi,hit);
      trellis.setPixelColor(laststep, color);
      if (length < numsteps) trellis.setPixelColor(length-1,C127);
    }
    // Do Probability!
    int outcomes[] = {1,1,1,1,1,1,1,1};
    uint32_t micros_per_step = ticks_per_step * tick_micros;
    uint32_t gate_micros;
    for (uint8_t i = 0; i < numseqs; ++i){
      if (probs[presets[i]][i][stepi] < 10){
        outcomes[i] = random(10) <= (probs[presets[i]][i][stepi]);
      }
      on_func(track_notes[i]+transpose, vels[presets[i]][i][stepi], gates[presets[trk_arr]][i][stepi], seqs[presets[i]][i][stepi] == 1 && mutes[i] == 0 ? outcomes[i] : false, track_chan[i]);
      gate_micros = gates[presets[i]][i][stepi] * micros_per_step / 16;
      held_gate_millis[i] = (now_micros + gate_micros) / 1000;
    }
    trellis.show();
  }

  void ctrl_stop(){
    off_func(ctrl_notes[0]+transpose, 127, 5, true, ctrl_chan);
    off_func(ctrl_notes[1]+transpose, 127, 5, true, ctrl_chan);
    off_func(ctrl_notes[2]+transpose, 127, 5, true, ctrl_chan);
  }

  void toggle_play_stop() {
    if( playing ) {
      clear_pos(stepi); 
      stop(); 
      on_func(ctrl_notes[1]+transpose, 127, 5, true, ctrl_chan);
      on_func(ctrl_notes[2]+transpose, 127, 5, true, ctrl_chan);
    }
    else { 
      play();
      on_func(ctrl_notes[2]+transpose, 127, 5, true, ctrl_chan);
      on_func(ctrl_notes[0]+transpose, 127, 5, true, ctrl_chan);
    }
    ctrl_stop();
  }

  // signal to sequencer/MIDI core we want to start playing
  void play() {
    stepi = -1; // hmm, but goes to 0 on first downbeat
    ticki = 0;
    playing = true;
    run = 1;
    if(send_clock && !extclk_micros) {
      clk_func( START );
    }
    on_func(ctrl_notes[2], 127, 5, true, ctrl_chan);
    on_func(ctrl_notes[0], 127, 5, true, ctrl_chan);
    ctrl_stop();
  }

  // signal to sequencer/MIDI core we want to stop playing
  void stop() {
    playing = false;
    run = 0;
    clear_pos(stepi);
    stepi = -1;
    if(send_clock && !extclk_micros) {
      clk_func( STOP );
    }
    for (uint8_t i = 0; i < numseqs; ++i){
      off_func( track_notes[i]+transpose, 0, 1, true, ctrl_chan);
    }
    on_func(ctrl_notes[1], 127, 5, true, ctrl_chan);
    on_func(ctrl_notes[2], 127, 5, true, ctrl_chan);
    ctrl_stop();
  }

  void reset() {
    stepi = -1;
    //ticki = 0;
    on_func(ctrl_notes[2], 127, 5, true, ctrl_chan);
    ctrl_stop();
  }

  void clear_pos(int stepi) {
    uint8_t trk_arr = sel_track - 1;
    int hit = 0;
    if (presetmode == 0){
      if (patedit == 1) {
        hit = seqs[presets[trk_arr]][trk_arr][stepi] > 0 ? seq_col(sel_track) : W10;
      } else if (gateedit == 1) {
        hit = gates[presets[trk_arr]][trk_arr][stepi] > 0 ? seq_col(sel_track) : W10;
      } else if (veledit == 1) {
        hit = vels[presets[trk_arr]][trk_arr][stepi] > 0 ? seq_col(sel_track) : W10;
      } else if (probedit == 1) {
        hit = probs[presets[trk_arr]][trk_arr][stepi] > 0 ? seq_col(sel_track) : W10;
      } else {
        hit = W10;
      }
      trellis.setPixelColor(stepi,hit);
    }
    trellis.show();
  }
};
