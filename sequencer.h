/**
 * sequencer.h -- Sequencer for picostepseq extended to Multitrack Sequencer (for Feather M4 Express)
 * 04 Nov 2023 - @apatchworkboy / Marci
 * 28 Apr 2023 - @todbot / Tod Kurt
 * 15 Aug 2022 - @todbot / Tod Kurt
 * Part of https://github.com/todbot/picostepseq/
 */

// typedefs
typedef enum {
  TRIGATE = 0,
  CC = 1,
  NOTE = 2,
  ARP = 3,
  CHORD = 4,
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

const int ticks_per_quarternote = 24;  
const int midi_clock_divider = 1; 
const int steps_per_beat_default = 6; 
const int valid_step_sizes[] = { QUARTER_NOTE, EIGHTH_NOTE, SIXTEENTH_NOTE };
const int valid_step_sizes_cnt = 3;

typedef void (*TriggerFunc)(uint8_t note, uint8_t vel, uint8_t gate, bool on, uint8_t chan);
typedef void (*CCFunc)(uint8_t cc, uint8_t val, bool on, uint8_t chan);
typedef void (*ClockFunc)(clock_type_t type);  // , int pos);
typedef void (*DispFunc)();
typedef void (*ResetFunc)();

// stubs for when Sequencer object is only partially initialized
void fake_updatedisplay_callback() {}
void fake_resetdisplay_callback() {}
void fake_clock_callback(clock_type_t type) {}
void fake_note_callback(uint8_t note, uint8_t vel, uint8_t gate, bool on, uint8_t chan) {}
void fake_cc_callback(uint8_t cc, uint8_t val, bool on, uint8_t chan) {}

class StepSequencer {
public:
  uint32_t tick_micros;  // "micros_per_tick", microsecs per clock (6 clocks / step; 4 steps / quarternote)
  uint32_t last_tick_micros;  // only change in update()
  uint32_t extclk_micros;  // 0 = internal clock, non-zero = external clock
  uint32_t held_gate_millis[numtracks];
  uint32_t held_gate_notes[numtracks];
  uint32_t held_gate_chans[numtracks];
  int multistepi[numtracks];
  int outcomes[numtracks];
  int lengths[numtracks];  // same as numsteps at boot!
  int offsets[numtracks];
  int ticks_per_step;  // expected values: 6 = 1/16th, 12 = 1/8, 24 = 1/4,
  int ticki;           // which midi clock we're on
  int stepi;           // which sequencer step we're on
  int seqno;
  int length;
  int transpose;
  uint8_t divcounts[numtracks];
  uint8_t divs[numtracks];
  uint8_t gates[numpresets][numtracks][numsteps];
  uint8_t notes[numpresets][numtracks][numsteps];
  uint8_t presets[numpresets];
  uint8_t probs[numpresets][numtracks][numsteps];
  uint8_t vels[numpresets][numtracks][numsteps];
  uint8_t laststeps[numtracks];
  uint8_t track_notes[numtracks];  // C2 thru G2
  uint8_t ctrl_notes[3];
  uint8_t track_chan[numtracks];
  uint8_t lastdac[2];
  uint8_t tick_pc;
  uint8_t ctrl_chan = 16;
  uint8_t laststep;
  uint8_t swing;
  uint8_t arpn[4];
  bool mutes[numtracks];
  bool seqs[numpresets][numtracks][numsteps];
  bool pulse;
  bool playing;
  bool send_clock;
  track_type modes[numtracks] = { TRIGATE };

  TriggerFunc on_func;
  TriggerFunc off_func;
  CCFunc cc_func;
  ClockFunc clk_func;
  DispFunc disp_func;
  ResetFunc reset_func;

  StepSequencer(float atempo = 120, uint8_t aseqno = 0) {
    transpose = 0;
    last_tick_micros = 0;
    stepi = 0;
    ticki = 0;
    ticks_per_step = 12;  // 12 = 1/16th, 24 = 1/8, 48 = 1/4,
    seqno = aseqno;
    length = 32;
    playing = false;
    extclk_micros = 0;
    send_clock = false;
    set_tempo(atempo);
    on_func = fake_note_callback;
    off_func = fake_note_callback;
    cc_func = fake_cc_callback;
    clk_func = fake_clock_callback;
    disp_func = fake_updatedisplay_callback;
    reset_func = fake_resetdisplay_callback;
  }

  // get tempo as floating point, computed dynamically from ticks_micros
  float tempo() {
    return 60 * 1000 * 1000 / tick_micros / ticks_per_quarternote;  //steps_per_beat * ticks_per_step);
  }

  // set tempo as floating point, computes ticks_micros
  void set_tempo(float bpm) {
    tick_micros = 60 * 1000 * 1000 / bpm / ticks_per_quarternote;
    tick_pc = tick_micros / 100;
  }

  void update() {
    uint8_t sw = length > 1 ? swing : 0;
    uint32_t now_micros = micros();

    if ((stepi % 2 ? (now_micros - last_tick_micros) < (tick_micros - (tick_pc * sw)) : (now_micros - last_tick_micros) < (tick_micros + (tick_pc * sw)))) {
      return;
    }  // not yet, with Swing!
    last_tick_micros = now_micros;

    // if we have a held note and it's time to turn it off, turn it off
    for (uint8_t i = 0; i < 8; ++i) {
      if (held_gate_millis[i] != 0 && millis() >= held_gate_millis[i]) {
        held_gate_millis[i] = 0;
        switch (modes[i]) {
          case ARP:
            switch (i){
              case 4:
                off_func(arpn[0], 0, 1, true, track_chan[i]);
                break;
              case 5:
                off_func(arpn[1], 0, 1, true, track_chan[i]);
                break;
              case 6:
                off_func(arpn[2], 0, 1, true, track_chan[i]);
                break;
              case 7:
                off_func(arpn[3], 0, 1, true, track_chan[i]);
                break;
              default: break;
            }
            break;
          case TRIGATE:
            off_func(track_notes[i] + transpose, 0, 1, true, track_chan[i]);
            digitalWrite(gatepins[i], LOW);
            break;
          case CC:
            digitalWrite(gatepins[i], LOW);
            break;
          case NOTE:
            off_func(held_gate_notes[i], 0, 1, true, held_gate_chans[i]);
            digitalWrite(gatepins[i], LOW);
            held_gate_notes[i] = 0;
            held_gate_chans[i] = 0;
            break;
          default: break;
        }
      }
    }

    if (send_clock && playing && !extclk_micros) {
      clk_func(CLOCK);
    }

    if (ticki == 0) {
      // do a sequence step (i.e. every "ticks_per_step" ticks)
      if (extclk_micros) {
        // do nothing, let midi clock trigger notes, but fall back to
        // internal clock if not externally clocked for a while
        if ((now_micros - extclk_micros) > tick_micros * ticks_per_quarternote) {
          extclk_micros = 0;
          Serial.println("Turning EXT CLOCK off");
        }
      } else {                // else internally clocked
        trigger(now_micros);  // delta_t);
      }
    } else {
      trellis.read();
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

    if (!playing) {
      trellis.show();
      strip.show();
      return;
    }
    if (resetflag == 1) {
      resetflag = 0;
      reset();
    }
    // Base sequencer
    pulse = pulse == 0 ? 1 : 0;
    stepi = (stepi + 1) % length;  // go to next step
    laststep = stepi - 1 < 0 ? length - 1 : stepi - 1;
    analogWrite(cvpins[0], lastdac[0]);
    analogWrite(cvpins[1], lastdac[1]);

    uint8_t trk_arr = sel_track - 1;

    // Do Probability!
    uint32_t micros_per_step = ticks_per_step * tick_micros;
    uint32_t gate_micros;
    for (uint8_t i = 0; i < numtracks; ++i) {

      divcounts[i] = divcounts[i] == divs[i] ? 0 : divcounts[i] + 1;
      
      if (divcounts[i] == 0) {
        //decouple per-track step counters from main sequencer
        uint8_t nstep = (multistepi[i] + 1) > lengths[i] - 1 ? 0 + (offsets[i] - 1 < 0 ? 0 : offsets[i] - 1) : (multistepi[i] + 1);  // go to next step

        uint8_t lstep = nstep > offsets[i] ? (nstep - 1 < 0 + (offsets[i] - 1 < 0 ? 0 : offsets[i] - 1) ? lengths[i] - 1 : nstep - 1) : (nstep - 1 < 0 ? lengths[i] - 1 : nstep - 1);

        laststeps[i] = lstep;
        multistepi[i] = nstep;

        if (probs[presets[i]][i][multistepi[i]] < 10) {
          outcomes[i] = random(10) <= (probs[presets[i]][i][multistepi[i]]);
        } else {
          outcomes[i] = 1;
        }

        gate_micros = (gates[presets[i]][i][multistepi[i]] * micros_per_step / 16) * (divs[i] + 1);

        switch (modes[i]) {
          case ARP:
            if (seqs[presets[i]][i][multistepi[i]] == 1 && mutes[i] == 0 ? outcomes[i] : false) {
              uint8_t n;
              switch (i) {
                case 4:
                  n = arp1.process(arp1pattern, arp1octave); // pattern (1-7), octaves(1-4)
                  arpn[0] = n;
                  break;
                case 5:
                  n = arp2.process(arp2pattern, arp2octave); // pattern (1-7), octaves(1-4)
                  arpn[1] = n;
                  break;
                case 6:
                  n = arp3.process(arp3pattern, arp3octave); // pattern (1-7), octaves(1-4)
                  arpn[2] = n;
                  break;
                case 7:
                  n = arp4.process(arp4pattern, arp4octave); // pattern (1-7), octaves(1-4)
                  arpn[3] = n;
                  break;
                default: break;
              }
              if (n != 0) {
                if (marci_debug) {
                  Serial.print("ArpNote: ");
                  Serial.println(n);
                }
                held_gate_millis[i] = (now_micros + gate_micros) / 1000;
                on_func(n, vels[presets[i]][i][multistepi[i]], gates[presets[i]][i][multistepi[i]], true, track_chan[i]);
              }
            }
            break;
          case TRIGATE:
            if (seqs[presets[i]][i][multistepi[i]] == 1 && mutes[i] == 0 ? outcomes[i] : false) {
              held_gate_millis[i] = (now_micros + gate_micros) / 1000;
              digitalWrite(gatepins[i], HIGH);
              on_func(track_notes[i] + transpose, vels[presets[i]][i][multistepi[i]], gates[presets[i]][i][multistepi[i]], true, track_chan[i]);
            }
            break;
          case CC:
            if (seqs[presets[i]][i][multistepi[i]] == 1 && mutes[i] == 0 ? outcomes[i] : false) {
              held_gate_millis[i] = (now_micros + gate_micros) / 1000;

              if (i > 5 && mutes[i] == 0) {
                // CV Output for track 7 & 8 on A0 & A1
                if (hzv[i - 6] == 1) {
                  // MS20 / K2 hz/v output
                  float val = constrain(map(125.0 * exp(0.0578 * ((vels[presets[i]][i][multistepi[i]] % 36) - 5)), 0, 5000, 0, dacrange), 0, dacrange);
                  if (val > 0) {
                    lastdac[i - 6] = val;
                    analogWrite(cvpins[i - 6], val);
                  }
                } else {
                  // Plain old v/oct
                  float val = constrain(map(vels[presets[i]][i][multistepi[i]] % 36, 0, 36, 0, 3708), 0, dacrange);
                  if (val > 0) {
                    lastdac[i - 6] = val;
                    analogWrite(cvpins[i - 6], val);
                  }
                }
              }
              digitalWrite(gatepins[i], HIGH);
              cc_func(track_notes[i] + transpose, vels[presets[i]][i][multistepi[trk_arr]], true, track_chan[i]);
            }
            break;
          case NOTE:
            if (seqs[presets[i]][i][multistepi[i]] == 1 && mutes[i] == 0 ? outcomes[i] : false) {
              held_gate_millis[i] = (now_micros + gate_micros) / 1000;
              held_gate_notes[i] = notes[presets[i]][i][multistepi[i]] + transpose;
              held_gate_chans[i] = track_chan[i];
              if (i > 5 && mutes[i] == 0) {
                // CV Output for track 7 & 8 on A0 & A1
                if (hzv[i - 6] == 1) {
                  // MS20 / K2 hz/v output
                  float val = constrain(map(125.0 * exp(0.0578 * ((notes[presets[i]][i][multistepi[i]] % 36) - 5)), 0, 5000, 0, dacrange), 0, dacrange);
                  if (val > 0) {
                    lastdac[i - 6] = val;
                    analogWrite(cvpins[i - 6], val);
                  }
                } else {
                  // Plain old v/oct
                  float val = constrain(map(notes[presets[i]][i][multistepi[i]] % 36, 0, 36, 0, 3708), 0, dacrange);
                  if (val > 0) {
                    lastdac[i - 6] = val;
                    analogWrite(cvpins[i - 6], val);
                  }
                }
              }
              digitalWrite(gatepins[i], HIGH);
              on_func(notes[presets[i]][i][multistepi[i]] + transpose, vels[presets[i]][i][multistepi[i]], gates[presets[i]][i][multistepi[i]], true, track_chan[i]);
            }
            break;
          default: break;
        }
      }
    }
    disp_func();
  }

  void ctrl_stop() {
    off_func(ctrl_notes[0] + transpose, 127, 5, true, ctrl_chan);
    off_func(ctrl_notes[1] + transpose, 127, 5, true, ctrl_chan);
    off_func(ctrl_notes[2] + transpose, 127, 5, true, ctrl_chan);
  }

  void toggle_play_stop() {
    if (playing) {
      on_func(ctrl_notes[1] + transpose, 127, 5, true, ctrl_chan);
      on_func(ctrl_notes[2] + transpose, 127, 5, true, ctrl_chan);
      stop();
    } else {
      on_func(ctrl_notes[2] + transpose, 127, 5, true, ctrl_chan);
      on_func(ctrl_notes[0] + transpose, 127, 5, true, ctrl_chan);
      play();
    }
  }

  // signal to sequencer/MIDI core we want to start playing
  void play() {
    for (uint8_t s = 0; s < numtracks; ++s) {
      multistepi[s] = -1;
      divcounts[s] = -1;
    }
    stepi = -1;  // hmm, but goes to 0 on first downbeat
    arp1._step = -1;
    arp2._step = -1;
    arp3._step = -1;
    arp4._step = -1;
    ticki = 0;
    playing = true;
    if (send_clock && !extclk_micros) {
      clk_func(START);
    }
    on_func(ctrl_notes[2], 127, 5, true, ctrl_chan);
    on_func(ctrl_notes[0], 127, 5, true, ctrl_chan);
    ctrl_stop();
  }

  // signal to sequencer/MIDI core we want to stop playing
  void stop() {
    if (playing) {
      playing = false;
      if (send_clock && !extclk_micros) {
        clk_func(STOP);
      }
      for (uint8_t i = 0; i < numtracks; ++i) {
        switch (modes[i]) {
          case TRIGATE:
            off_func(track_notes[i] + transpose, 0, 1, true, track_chan[i]);
            break;
          case CC:
            break;
          case NOTE:
            for (uint8_t j = 0; j < numpresets; ++j) {
              for (uint8_t k = 0; k < numsteps; ++k) {
                off_func(notes[presets[j]][i][k] + transpose, 0, 0, true, track_chan[i]);
              }
            }
            break;
          default: break;
        }
        digitalWrite(gatepins[i], LOW);
      }
      on_func(ctrl_notes[1], 127, 5, true, ctrl_chan);
      on_func(ctrl_notes[2], 127, 5, true, ctrl_chan);
      ctrl_stop();
      reset_func();
    }
  }

  void reset() {
    stepi = -1;
    arp1._step = -1;
    arp2._step = -1;
    arp3._step = -1;
    arp4._step = -1;
    for (uint8_t s = 0; s <= 7; s++) {
      multistepi[s] = -1;
      divcounts[s] = -1;
    }
    // ticki = 0;
    on_func(ctrl_notes[2], 127, 5, true, ctrl_chan);
    ctrl_stop();
  }
};
