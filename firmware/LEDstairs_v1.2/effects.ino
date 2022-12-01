// плавный включатель-выключатель эффектов
void stepFader(bool dir, bool state) {
  // dir 0 на себя, 1 от себя
  // state 0 рост, 1 выкл
  // 0 0
  // 0 1
  // 1 0
  // 1 1
  byte mode = state | (dir << 1);
  byte counter = 0;
  while (1) {
    EVERY_MS(FADR_SPEED) {
      counter++;
      switch (curEffect) {
        case COLOR:
          switch (mode) {
            case 0: staticColor(1, 0, counter); break;
            case 1: staticColor(1, counter, STEP_AMOUNT); break;
            case 2: staticColor(-1, STEP_AMOUNT - counter, STEP_AMOUNT); break;
            case 3: staticColor(-1, 0, STEP_AMOUNT - counter); break;
          }
          break;
        case RAINBOW:
          switch (mode) {
            case 0: rainbowStripes(-1, STEP_AMOUNT - counter, STEP_AMOUNT); break;
            case 1: rainbowStripes(-1, 0, STEP_AMOUNT - counter); break;
            case 2: rainbowStripes(1, STEP_AMOUNT - counter, STEP_AMOUNT); break;
            case 3: rainbowStripes(1, 0, STEP_AMOUNT - counter); break;
          }
          break;
        case FIRE:
          if (state) {
            int changeBright = curBright;
            while (1) {
              EVERY_MS(50) {
                changeBright -= 5;
                if (changeBright < 0) break;
                setBrightness(changeBright);
                fireStairs(0, 0, 0);
                show();
              }
            }
            clear();
            setBrightness(curBright);
            show();
          } else {
            int changeBright = 0;
            setBrightness(0);
            while (1) {
              EVERY_MS(50) {
                changeBright += 5;
                if (changeBright > curBright) break;
                setBrightness(changeBright);
                fireStairs(0, 0, 0);
                show();
              }
              setBrightness(curBright);
            }
          }
          return;
      }
      show();
      if (counter == STEP_AMOUNT) break;
    }
  }
  if (state == 1) {
    clear();
    show();
  }
}

// ============== ЭФФЕКТЫ =============
// ========= огонь
// настройки пламени
#define HUE_GAP 45      // заброс по hue
#define FIRE_STEP 90    // шаг изменения "языков" пламени
#define HUE_START 2     // начальный цвет огня (0 красный, 80 зелёный, 140 молния, 190 розовый)
#define MIN_BRIGHT 150  // мин. яркость огня
#define MAX_BRIGHT 255  // макс. яркость огня
#define MIN_SAT 220     // мин. насыщенность
#define MAX_SAT 255     // макс. насыщенность

void fireStairs(int8_t dir, byte from, byte to) {
  effSpeed = 30;
  static uint16_t counter = 0;
  int start_led_index = 0;

  FOR_i(0, minStepLength) {
    start_led_index = 0;
    FOR_j(0, STEP_AMOUNT) {
      if (j > 0) start_led_index += steps[j-1].led_amount;
      // из за разных длин ступенек теперь это не матрица, а лента
      strip.setLED(start_led_index + i, mHEX(getPixColor(ColorFromPalette(
                                            firePalette,
                                            (inoise8(i * FIRE_STEP, j * FIRE_STEP, counter)),
                                            255,
                                            LINEARBLEND
                                          ))));
    }
  }
#if (RAILING == 1)
  FOR_i(0, railingSegmentLength) {
    FOR_j(0, STEP_AMOUNT) {
      railing.setPix(i, j, mHEX(getPixColor(ColorFromPalette(
                                              firePalette,
                                              (inoise8(i * FIRE_STEP, j * FIRE_STEP, counter)),
                                              255,
                                              LINEARBLEND
                                            ))));
    }
  }
#endif
  counter += 10;
}

uint32_t getPixColor(CRGB thisPixel) {
  return (((uint32_t)thisPixel.r << 16) | (thisPixel.g << 8) | thisPixel.b);
}

CRGB getFireColor(int val) {
  // чем больше val, тем сильнее сдвигается цвет, падает насыщеность и растёт яркость
  return CHSV(
           HUE_START + map(val, 0, 255, 0, HUE_GAP),                    // H
           constrain(map(val, 0, 255, MAX_SAT, MIN_SAT), 0, 255),       // S
           constrain(map(val, 0, 255, MIN_BRIGHT, MAX_BRIGHT), 0, 255)  // V
         );
}

// ========= смена цвета общая
void staticColor(int8_t dir, byte from, byte to) {
  effSpeed = 100;
  byte thisBright;
  static byte colorCounter = 0;
  colorCounter += 2;
  FOR_i(0, STEP_AMOUNT) {
    thisBright = 255;
    if (i < from || i >= to) thisBright = 0;
    fillStep(i, mHSV(colorCounter, 255, thisBright));
  }
}

// ========= полоски радужные
void rainbowStripes(int8_t dir, byte from, byte to) {
  effSpeed = 40;
  static byte colorCounter = 0;
  colorCounter += 2;
  byte thisBright;
  FOR_i(0, STEP_AMOUNT) {
    thisBright = 255;
    if (i < from || i >= to) thisBright = 0;
    fillStep((dir > 0) ? (i) : (STEP_AMOUNT - 1 - i), mHSV(colorCounter + (float)i * 255 / STEP_AMOUNT, 255, thisBright));
  }
}

// ========= залить ступеньку цветом (служебное)
void fillStep(int8_t num, LEDdata color) {
  if (num >= STEP_AMOUNT || num < 0) return;
  for(int i = steps_start[num]; i < steps_start[num] + steps[num].led_amount; i++) {
    stripLEDs[i] = color;
  }
#if (RAILING == 1)
  FOR_i(num * railingSegmentLength, num * railingSegmentLength + railingSegmentLength) {
    railingLEDs[i] = color;
  }
#endif
}

void fillStepWithBitMask(int8_t num, LEDdata color, uint32_t bitMask) {
  if (num >= STEP_AMOUNT || num < 0) return;
  for(int i = steps_start[num]; i < steps_start[num] + steps[num].led_amount; i++) {
    if (bitRead(bitMask, (i - steps_start[num]) % 16)) {
      stripLEDs[i] = color;
    }
  }
#if (RAILING == 1)
  FOR_i(num * railingSegmentLength, num * railingSegmentLength + railingSegmentLength) {
    if (bitRead(bitMask, i % railingSegmentLength)) {
      railingLEDs[i] = color;
    }
  }
#endif
}

void animatedSwitchOff(int bright) {
  int changeBright = bright;
  do {
    delay(50);
    setBrightness(changeBright);
    show();
    changeBright -= 5;
  } while (changeBright > 0);
}

void animatedSwitchOn(int bright) {
  int changeBright = 0;
  do {
    delay(50);
    setBrightness(changeBright);
    show();
    changeBright += 5;
  } while (changeBright < bright);
}

void setBrightness(int brightness) {
  strip.setBrightness(brightness);
#if (RAILING == 1)
  railing.setBrightness(brightness);
#endif
}

void show() {
  strip.show();
#if (RAILING == 1)
  railing.show();
#endif
}

void clear() {
  strip.clear();
#if (RAILING == 1)
  railing.clear();
#endif
}
