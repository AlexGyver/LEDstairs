/*
  Скетч к проекту "Подсветка лестницы"
  Страница проекта (схемы, описания): https://alexgyver.ru/ledstairs/
  Исходники на GitHub: https://github.com/AlexGyver/LEDstairs
  Нравится, как написан код? Поддержи автора! https://alexgyver.ru/support_alex/
  Автор: AlexGyver Technologies, 2019
  https://AlexGyver.ru/
*/

#define STEP_AMOUNT 8     // количество ступенек
#define STEP_LENGTH 17    // количество чипов WS2811 на ступеньку

#define AUTO_BRIGHT 0     // автояркость 0/1 вкл/выкл (с фоторезистором)
#define CUSTOM_BRIGHT 40  // ручная яркость

#define FADR_SPEED 500
#define START_EFFECT RAINBOW    // режим при старте COLOR, RAINBOW, FIRE
#define ROTATE_EFFECTS 0      // 0/1 - автосмена эффектов
#define TIMEOUT 15            // секунд, таймаут выключения ступенек, если не сработал конечный датчик

// пины
// если перепутаны сенсоры - можно поменять их местами в коде! Вот тут
#define SENSOR_START 3
#define SENSOR_END 2
#define STRIP_PIN 12    // пин ленты
#define PHOTO_PIN A0

// для разработчиков
#define ORDER_BGR       // порядок цветов ORDER_GRB / ORDER_RGB / ORDER_BRG
#define COLOR_DEBTH 2   // цветовая глубина: 1, 2, 3 (в байтах)
#include <microLED.h>
#define NUMLEDS STEP_AMOUNT * STEP_LENGTH // кол-во светодиодов
LEDdata leds[NUMLEDS];  // буфер ленты
microLED strip(leds, STRIP_PIN, STEP_LENGTH, STEP_AMOUNT, ZIGZAG, LEFT_BOTTOM, DIR_RIGHT);  // объект матрица

int effSpeed;
int8_t effDir;
byte curBright = CUSTOM_BRIGHT;
enum {S_IDLE, S_WORK} systemState = S_IDLE;
enum {COLOR, RAINBOW, FIRE} curEffect = START_EFFECT;
#define EFFECTS_AMOUNT 3
byte effectCounter;

// ==== удобные макросы ====
#define FOR_i(from, to) for(int i = (from); i < (to); i++)
#define FOR_j(from, to) for(int j = (from); j < (to); j++)
#define FOR_k(from, to) for(int k = (from); k < (to); k++)
#define EVERY_MS(x) \
  static uint32_t tmr;\
  bool flag = millis() - tmr >= (x);\
  if (flag) tmr = millis();\
  if (flag)
//===========================

// ФЛ для функции Noise
#include <FastLED.h>
int counter = 0;
CRGBPalette16 firePalette;

void setup() {
  Serial.begin(9600);
  //FastLED.addLeds<WS2811, STRIP_PIN, GRB>(leds, NUMLEDS).setCorrection( TypicalLEDStrip );
  strip.setBrightness(curBright);    // яркость (0-255)
  strip.clear();
  strip.show();
  // для кнопок
  //pinMode(SENSOR_START, INPUT_PULLUP);
  //pinMode(SENSOR_END, INPUT_PULLUP);
  firePalette = CRGBPalette16(
                  getFireColor(0 * 16),
                  getFireColor(1 * 16),
                  getFireColor(2 * 16),
                  getFireColor(3 * 16),
                  getFireColor(4 * 16),
                  getFireColor(5 * 16),
                  getFireColor(6 * 16),
                  getFireColor(7 * 16),
                  getFireColor(8 * 16),
                  getFireColor(9 * 16),
                  getFireColor(10 * 16),
                  getFireColor(11 * 16),
                  getFireColor(12 * 16),
                  getFireColor(13 * 16),
                  getFireColor(14 * 16),
                  getFireColor(15 * 16)
                );
  delay(100);
  strip.clear();
  strip.show();
}

void loop() {
  getBright();
  readSensors();
  effectFlow();
}

void getBright() {
#if (AUTO_BRIGHT == 1)
  if (systemState == S_IDLE) {  // в режиме простоя
    EVERY_MS(3000) {            // каждые 3 сек
      Serial.println(analogRead(PHOTO_PIN));
      curBright = map(analogRead(PHOTO_PIN), 30, 800, 10, 200);
      strip.setBrightness(curBright);
    }
  }
#endif
}

// крутилка эффектов в режиме активной работы
void effectFlow() {
  if (systemState == S_WORK) {
    static uint32_t tmr;
    if (millis() - tmr >= effSpeed) {
      tmr = millis();
      //EVERY_MS(effSpeed) {
      switch (curEffect) {
        case COLOR: staticColor(effDir, 0, STEP_AMOUNT); break;
        case RAINBOW: rainbowStripes(-effDir, 0, STEP_AMOUNT); break; // rainbowStripes - приёмный
        case FIRE: fireStairs(effDir, 0, 0); break;
      }
      strip.show();
    }
  }
}

// читаем сенсоры
void readSensors() {
  static bool flag1 = false, flag2 = false;
  static uint32_t timeoutCounter;

  // ТАЙМАУТ
  if (systemState == S_WORK && millis() - timeoutCounter >= (TIMEOUT * 1000L)) {
    systemState = S_IDLE;
    int changeBright = curBright;
    while (1) {
      EVERY_MS(50) {
        changeBright -= 5;
        if (changeBright < 0) break;
        strip.setBrightness(changeBright);
        strip.show();
      }
    }
    strip.clear();
    strip.setBrightness(curBright);
    strip.show();
  }

  EVERY_MS(50) {
    // СЕНСОР У НАЧАЛА ЛЕСТНИЦЫ
    if (digitalRead(SENSOR_START)) {
      if (!flag1) {
        flag1 = true;
        timeoutCounter = millis();
        if (systemState == S_IDLE) {
          effDir = 1;
          if (ROTATE_EFFECTS) {
            if (++effectCounter >= EFFECTS_AMOUNT) effectCounter = 0;
            curEffect = effectCounter;
          }
        }
        switch (systemState) {
          case S_IDLE: stepFader(0, 0); systemState = S_WORK; break;
          case S_WORK:
            if (effDir == -1) {
              stepFader(1, 1); systemState = S_IDLE;
              strip.clear(); strip.show(); return;
            } break;
        }
      }
    } else {
      if (flag1) flag1 = false;
    }

    // СЕНСОР У КОНЦА ЛЕСТНИЦЫ
    if (digitalRead(SENSOR_END)) {
      if (!flag2) {
        flag2 = true;
        timeoutCounter = millis();
        if (systemState == S_IDLE) {
          effDir = -1;
          if (ROTATE_EFFECTS) {
            if (++effectCounter >= EFFECTS_AMOUNT) effectCounter = 0;
            curEffect = effectCounter;
          }
        }
        switch (systemState) {
          case S_IDLE: stepFader(1, 0); systemState = S_WORK; break;
          case S_WORK:
            if (effDir == 1) {
              stepFader(0, 1); systemState = S_IDLE;
              strip.clear(); strip.show(); return;
            } break;
        }
      }
    } else {
      if (flag2) flag2 = false;
    }
  }
}
