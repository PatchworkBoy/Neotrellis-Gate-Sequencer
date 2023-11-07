# Neotrellis-Trigger-Sequencer
[![YouTube Demo Video](http://img.youtube.com/vi/L5sNkB95-T4/0.jpg)](http://www.youtube.com/watch?v=L5sNkB95-T4 "Demo Video")

An 8 track 32 step MIDI (over USB) gate/trigger sequencer and MIDI Clock Generator for Feather M4 Express / Neotrellis 8x8
Adapted from https://github.com/todbot/picostepseq/

Designed for use with the Adafruit 8x8 Neotrellis Feather M4 Kit, no additional hardware required - 
- US:  https://www.adafruit.com/product/1929
- UK: https://thepihut.com/products/adafruit-untztrument-open-source-8x8-grid-controller-kit-8x8-white-leds

NeoTrellis Surface...

![Neotrellis Keys](https://apatchworkboy.com/wp-content/uploads/2023/11/Screenshot-2023-11-07-at-14.37.27.png)

- Row 1 - 4: Steps 1 thru 32 (pattern edit: on/off, velocity edit: cycle thru velocity 40 / 80 / 127, Length: select end step, Probability: 10% - 100% in 10% increments)
- Row 5: Pattern Edit - Trk1 | Trk2 | Trk3 | Trk4 | Trk5 | Trk6 | Trk7 | Trk8
- Row 6: Velocity Map Edit - Trk1 | Trk2 | Trk3 | Trk4 | Trk5 | Trk6 | Trk7 | Trk8
- Row 7: - | - | - | - | Probability Map | Octave 0/+1/+2 (only while stopped) | Global Pattern Length | toggle step size - quarter / eighth / sixteenth
- Bottom row: Toggle Play/Stop | Stop | Reset | SAVE | FACTORY RESET | MIDICLOCK Send On/Off | Tempo down | Tempo up

- SAVE: store all patterns, velocity maps, current step-size and tempo to flash.
- FACTORY RESET: resets all patterns & velocity maps to default, step size to sixteenths, tempo to 120.

Outputs self-generated MIDI Clock (24 PPQN), Play/Stop/Cont (ideal for use in VCV rack with MIDI > CV module)
- Default BPM: 120, adjustable via Tempo down / up buttons in -/+ 1 increments
- OR can be driven with a 24PPQN external midi clock (eg: Impromptu Clocked x24 to CV>MIDI clock)

Mapping for VCVRack MIDI > Gate module:
- Trk1: Note C2 (36)
- Trk2: Note C#2 (37)
- Trk3: Note D2 (38)
- Trk4: Note D#2 (39)
- Trk5: Note E2 (40)
- Trk6: Note F2 (41)
- Trk7: Note F#2 (42)
- Trk8: Note G2 (43)
- Play: Note C3 (48)
- Stop: Note C#3 (49)
- Reset: Note D#3 (51)

Octave button shifts ALL notes up +1ve or +2ve

FAR from perfect. Open to improvements - throw me a pull request.
