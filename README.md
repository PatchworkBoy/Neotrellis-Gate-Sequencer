# Neotrellis-Trigger-Sequencer
[![YouTube Demo Video](http://img.youtube.com/vi/L5sNkB95-T4/0.jpg)](http://www.youtube.com/watch?v=L5sNkB95-T4 "Demo Video")

An 8 track 32 step MIDI (over USB) gate/trigger sequencer and MIDI Clock Generator (with swing) for Feather M4 Express / Neotrellis 8x8 featuring per-step per-track per-pattern velocity / probability & gatelength layers, performance mutes and overall loop-length control.
16 storable preset slots per track (all layers stored). Customisable note-per-track and channel-per-track.
Adapted from https://github.com/todbot/picostepseq/

Designed for use with the Adafruit 8x8 Neotrellis Feather M4 Kit, no additional hardware required - 
- US:  https://www.adafruit.com/product/1929
- UK: https://thepihut.com/products/adafruit-untztrument-open-source-8x8-grid-controller-kit-8x8-white-leds

NeoTrellis Surface...

![Neotrellis Keys](https://apatchworkboy.com/wp-content/uploads/2023/11/Screenshot-2023-11-12-at-00.12.05.png)

- Row 1 - 4: Steps 1 thru 32 (pattern edit: on/off, velocity edit: cycle thru velocity 40 / 80 / 127, Length: select end step, Probability: 10% - 100% in 10% increments)
- Row 5: Track Select - Trk1 | Trk2 | Trk3 | Trk4 | Trk5 | Trk6 | Trk7 | Trk8
- Row 6: Track Mutes - Trk1 | Trk2 | Trk3 | Trk4 | Trk5 | Trk6 | Trk7 | Trk8
- Row 7: Pattern Edit | Velocity Edit | Probability Edit | Gate Length Edit | SHIFT (^) | Global Octave 0/+1/+2 (only while stopped) / ^ MIDI Channel Config (only while stopped) | Global Pattern Loop-Length | toggle step size - quarter / eighth / sixteenth / ^swing
- Row 8: Toggle Play/Stop | Stop | Reset | SAVE | PRESETS / ^Factory Reset | MIDICLOCK Send On/Off | param - | param +

PARAM -/+
- Pattern Edit - param = tempo, step = on/off
- Velocity Edit - param = all velocities (+/- 10), step = step velocity cycle (40/180/127)
- Probability Edit - param = all probabilities (+/- 10%), step = step probability cycle (+10%)
- Gate Length Edit - param = all gate lengths (+/- 10%), step = gate length cycle (+10%)
- SHIFT - param = note (+/- 1)
- Swing (SHIFT + StepSize) - param = +/- 1% (30% max)

PRESETS mode:
- Row 1 & 2 - change preset for selected track, 1-16
- Row 3 & 4 - change ALL tracks to selected preset, 1-16
- SAVE: store all patterns, velocity, probability & gate length maps, current step-size, track notes, track midi channels and tempo to flash. DO NOT power down whilst saving. Wait for button to cycle from Red back to Cyan.
- FACTORY RESET (SHIFT + Presets): resets all patterns & velocity & probability & gate maps (both in memory & on disk (flash)) to default, step size to sixteenths, tempo to 120, transpose to 0. DO NOT power down whilst saving. Wait for button to cycle from Red back to Cyan.

Outputs optional self-generated MIDI Clock (24 PPQN), Play/Stop/Reset (ideal for use in VCV rack with MIDI > CV module)
- Default BPM: 120, adjustable via param buttons in -/+ 1 increments. Swing (+/- 30% max) is also applied to clock output.
- OR can be driven with a 24PPQN external midi clock (eg: Impromptu Clocked x24 to CV>MIDI clock)

Default Mapping for VCVRack MIDI > Gate module:
- Trk1: Note C2 (36) - MIDI Ch1
- Trk2: Note C#2 (37) - MIDI Ch1
- Trk3: Note D2 (38) - MIDI Ch1
- Trk4: Note D#2 (39) - MIDI Ch1
- Trk5: Note E2 (40) - MIDI Ch2
- Trk6: Note F2 (41) - MIDI Ch3
- Trk7: Note F#2 (42) - MIDI Ch4
- Trk8: Note G2 (43) - MIDI Ch5

- Play: Note C0 (12) - MIDI Ch16
- Stop: Note C#0 (13) - MIDI Ch16
- Reset: Note D#0 (14) - MIDI Ch16

Each track is transposable to set custom note. Each track can be set to any MIDI Channel.

Octave button globally shifts ALL track notes up +1ve or +2ve ON TOP of any existing per-track transposition.

FAR from perfect. Open to improvements - throw me a pull request.
