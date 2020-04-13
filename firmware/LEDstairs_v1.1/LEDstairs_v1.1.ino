/*
  Новые функции в прошивке 1.1
  1) поддержка ночного режима. Подсвечиваются крайние ступеньки.
  Настройка:
  - маска (какие диоды на ленте включать) NIGHT_LIGHT_BIT_MASK,
  - цвет NIGHT_LIGHT_COLOR
  - яркость NIGHT_LIGHT_BRIGHT
  Автор: Геннадий Дегтерёв, 2020
  https://degterjow.de/
  gennadij@degterjow.de

  Скетч к проекту "Подсветка лестницы"
  Страница проекта (схемы, описания): https://alexgyver.ru/ledstairs/
  Исходники на GitHub: https://github.com/AlexGyver/LEDstairs
  Нравится, как написан код? Поддержи автора! https://alexgyver.ru/support_alex/
  Автор: AlexGyver Technologies, 2019
  https://AlexGyver.ru/
*/

#define STEP_AMOUNT 18     // количество ступенек
#define STEP_LENGTH 15    // количество чипов WS2811 на ступеньку

#define AUTO_BRIGHT 1     // автояркость 0/1 вкл/выкл (с фоторезистором)
#define CUSTOM_BRIGHT 40  // ручная яркость

#define FADR_SPEED 500
#define START_EFFECT RAINBOW    // режим при старте COLOR, RAINBOW, FIRE
#define ROTATE_EFFECTS 1      // 0/1 - автосмена эффектов
#define TIMEOUT 15            // секунд, таймаут выключения ступенек, если не сработал конечный датчик

#define NIGHT_LIGHT_BIT_MASK 0b1000000100000001  // последовательность диодов в ночном режиме
#define NIGHT_LIGHT_COLOR 50  // 0 - 255
#define NIGHT_LIGHT_BRIGHT 50  // 0 - 255
#define NIGHT_PHOTO_MAX 500   // максимальное значение фоторезистора для отключения подсветки

// пины
// если перепутаны сенсоры - можно поменять их местами в коде! Вот тут
#define SENSOR_START 3
#define SENSOR_END 2
#define STRIP_PIN 12    // пин ленты
#define PHOTO_PIN A0

// для разработчиков
#include <microLED.h>
#include <FastLED.h> // ФЛ для функции Noise

#define ORDER_BGR       // порядок цветов ORDER_GRB / ORDER_RGB / ORDER_BRG
#define COLOR_DEBTH 2   // цветовая глубина: 1, 2, 3 (в байтах)
#define NUMLEDS STEP_AMOUNT * STEP_LENGTH // кол-во светодиодов

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

LEDdata leds[NUMLEDS];  // буфер ленты
microLED strip(leds, STRIP_PIN, STEP_LENGTH, STEP_AMOUNT, ZIGZAG, LEFT_BOTTOM, DIR_RIGHT);  // объект матрица

int effSpeed;
int8_t effectDirection;
byte curBright = CUSTOM_BRIGHT;
enum {COLOR, RAINBOW, FIRE, EFFECTS_AMOUNT} curEffect = START_EFFECT;
byte effectCounter;
uint32_t timeoutCounter;
bool systemIdleState;
bool systemOffState;
uint32_t nightLightBitMask = NIGHT_LIGHT_BIT_MASK;

struct PirSensor {
  int8_t effectDirection;
  int8_t pin;
  bool lastState;
};
PirSensor startPirSensor = { 1, SENSOR_START, false};
PirSensor endPirSensor = { 1, SENSOR_END, false};

CRGBPalette16 firePalette;

void setup() {
  Serial.begin(9600);
  strip.setBrightness(curBright);    // яркость (0-255)
  strip.clear();
  strip.show();
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
  handlePirSensor(&startPirSensor);
  handlePirSensor(&endPirSensor);
  if (systemIdleState || systemOffState) {
    handlePhotoResistor();
    nightLight();
    delay(50);
  } else {
    effectFlow();
    handleTimeout();
  }
}

void handlePhotoResistor() {
#if (AUTO_BRIGHT == 1)
  EVERY_MS(3000) {            // каждые 3 сек
    int photo = analogRead(PHOTO_PIN);
    Serial.print("Photoresistor ");
    Serial.println(photo);
    systemOffState = photo > NIGHT_PHOTO_MAX;
    if (systemOffState)
      Serial.println("System OFF");
    else
      Serial.println("System ON");
    curBright = map(photo, 30, 800, 10, 200);
    strip.setBrightness(curBright);
    Serial.print("LED bright ");
    Serial.println(curBright);
  }
#endif
}

void nightLight() {
  if (systemOffState) {
    strip.clear();
    strip.show();
    return;
  }
  EVERY_MS(15000) {
    // каждые 15 секунд сдвигаем маску на один бит, чтобы диоды не сильно грелись и не выгорали
    nightLightBitMask =  (nightLightBitMask >> 1) | (nightLightBitMask << (STEP_LENGTH - 1));
    Serial.print("Night light bit mask ");
    Serial.println(nightLightBitMask, BIN);
    strip.clear();
    strip.setBrightness(NIGHT_LIGHT_BRIGHT);
    fillStepWithBitMask(0, mHSV(NIGHT_LIGHT_COLOR, 255, NIGHT_LIGHT_BRIGHT), nightLightBitMask);
    fillStepWithBitMask(STEP_AMOUNT - 1, mHSV(NIGHT_LIGHT_COLOR, 255, NIGHT_LIGHT_BRIGHT), nightLightBitMask);
    strip.show();
    strip.setBrightness(curBright);
  }
}

void fillStepWithBitMask(int8_t num, LEDdata color, int8_t bitMask) {
  if (num >= STEP_AMOUNT || num < 0) return;
  FOR_i(num * STEP_LENGTH, num * STEP_LENGTH + STEP_LENGTH) {
    if (bitRead(i % STEP_LENGTH, bitMask)) {
      leds[i] = color;
    }
  }
}

void handleTimeout() {
  if (millis() - timeoutCounter >= (TIMEOUT * 1000L)) {
    Serial.println("Timeout!");
    systemIdleState = true;
    int changeBright = curBright;
    do {
      delay(50);
      strip.setBrightness(changeBright);
      strip.show();
      changeBright -= 5;
    } while (changeBright > 0);
    strip.clear();
    strip.setBrightness(curBright);
    strip.show();
  }
}

void handlePirSensor(PirSensor *sensor) {
  if (systemOffState) return;

  int newState = digitalRead(sensor->pin);
  if (newState && !sensor->lastState) {
    timeoutCounter = millis();
    Serial.print("PIR ON");
    Serial.println(sensor->pin);
    if (systemIdleState) {
      effectDirection = sensor->effectDirection;
      if (ROTATE_EFFECTS) {
        curEffect = ++effectCounter % EFFECTS_AMOUNT;
      }
    } else {
      if (effectDirection == 1) {
        stepFader(0, 1);
      } else {
        stepFader(1, 1);
      }
      strip.clear();
      strip.show();
    }
    systemIdleState = !systemIdleState;
  }
  sensor->lastState = newState;
}

// крутилка эффектов в режиме активной работы
void effectFlow() {
  static uint32_t tmr;
  if (millis() - tmr >= effSpeed) {
    tmr = millis();
    switch (curEffect) {
      case COLOR: staticColor(effectDirection, 0, STEP_AMOUNT); break;
      case RAINBOW: rainbowStripes(-effectDirection, 0, STEP_AMOUNT); break; // rainbowStripes - приёмный
      case FIRE: fireStairs(effectDirection, 0, 0); break;
    }
    strip.show();
  }
}
