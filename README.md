# Neotrellis-Trigger-Sequencer
An 8 track 32 step MIDI (over USB) gate/trigger sequencer and MIDI Clock Generator for Feather M4 Express / Neotrellis 8x8
Adapted from https://github.com/todbot/picostepseq/

Designed for use with the Adafruit 8x8 Neotrellis Feather M4 Kit, no additional hardware required - 
- US:  https://www.adafruit.com/product/1929
- UK: https://thepihut.com/products/adafruit-untztrument-open-source-8x8-grid-controller-kit-8x8-white-leds

NeoTrellis Surface...
- Row 1 - 4: Steps 1 thru 32
- Row 5: Trk1 | Trk2 | Trk3 | Trk4 | Trk5 | Trk6 | Trk7 | Trk8
- Bottom row: Toggle Play/Stop | Stop | Pause (iffy, don't bother) | Reset | - | - | Tempo down | Tempo up

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

To Do: Save & Load 16 sets of pattern-banks.

Map Save & Load buttons to remaining bottom row slots. Use Row 6 & 7 for pattern-bank selection.

FAR from perfect. Open to improvements - throw me a pull request.
