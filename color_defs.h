/**
 * color_defs.h -- NeoPixel color definitions for Multitrack Sequencer UI
 * Part of https://github.com/PatchworkBoy/Neotrellis-Gate-Sequencer
 * 04 Nov 2023 - @apatchworkboy / Marci
 */
#ifndef MULTI_SEQUENCER_COLOR_DEFS
#define MULTI_SEQUENCER_COLOR_DEFS

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
#define W40 seesaw_NeoPixel::Color(40, 40, 40)
#define W10 seesaw_NeoPixel::Color(4, 4, 4)

uint32_t seq_col(int seq) {
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

uint32_t seq_dim(uint8_t seq, uint8_t level) {
  switch (seq) {
    case 1:
      return (level <= 20) ? W10 : (level <= 40)  ? R40
                                 : (level <= 80)  ? R80
                                 : (level <= 127) ? R127
                                                  : 0;
    case 2:
      return (level <= 20) ? W10 : (level <= 40)  ? O40
                                 : (level <= 80)  ? O80
                                 : (level <= 127) ? O127
                                                  : 0;
    case 3:
      return (level <= 20) ? W10 : (level <= 40)  ? Y40
                                 : (level <= 80)  ? Y80
                                 : (level <= 127) ? Y127
                                                  : 0;
    case 4:
      return (level <= 20) ? W10 : (level <= 40)  ? G40
                                 : (level <= 80)  ? G80
                                 : (level <= 127) ? G127
                                                  : 0;
    case 5:
      return (level <= 20) ? W10 : (level <= 40)  ? C40
                                 : (level <= 80)  ? C80
                                 : (level <= 127) ? C127
                                                  : 0;
    case 6:
      return (level <= 20) ? W10 : (level <= 40)  ? B40
                                 : (level <= 80)  ? B80
                                 : (level <= 127) ? B127
                                                  : 0;
    case 7:
      return (level <= 20) ? W10 : (level <= 40)  ? P40
                                 : (level <= 80)  ? P80
                                 : (level <= 127) ? P127
                                                  : 0;
    case 8:
      return (level <= 20) ? W10 : (level <= 40)  ? PK40
                                 : (level <= 80)  ? PK80
                                 : (level <= 127) ? PK127
                                                  : 0;
    default:
      return 0;
  }
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
#endif