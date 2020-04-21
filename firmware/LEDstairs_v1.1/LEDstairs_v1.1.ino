/*
  Новые функции в прошивке 1.1
  1) поддержка ночного режима. Подсвечиваются крайние ступеньки.
  Настройка:
  - маска (какие диоды на ленте включать) NIGHT_LIGHT_BIT_MASK
  - цвет NIGHT_LIGHT_COLOR
  - яркость NIGHT_LIGHT_BRIGHT
  - максимальное значение фоторезистора NIGHT_PHOTO_MAX для отключения подсветки
  
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

#define STEP_AMOUNT 5     // количество ступенек
#define STEP_LENGTH 16    // количество чипов WS2811 на ступеньку

#define AUTO_BRIGHT 1     // автояркость 0/1 вкл/выкл (с фоторезистором)
#define CUSTOM_BRIGHT 100  // ручная яркость

#define FADR_SPEED 300         // скорость переключения с одной ступеньки на другую
#define START_EFFECT RAINBOW   // режим при старте COLOR, RAINBOW, FIRE
#define ROTATE_EFFECTS 1      // 0/1 - автосмена эффектов
#define TIMEOUT 15            // секунд, таймаут выключения ступенек, если не сработал конечный датчик

#define NIGHT_LIGHT_BIT_MASK 0b10101010101010101010101010101010  // последовательность диодов в ночном режиме
#define NIGHT_LIGHT_COLOR 100  // 0 - 255
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
    handleNightLight();
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

void handleNightLight() {
  EVERY_MS(60000) {
    nightLight();
  }
}

void nightLight() {
  if (systemOffState) {
    strip.clear();
    strip.show();
    return;
  }
  // инвертируем маску, чтобы диоды не выгорали
  nightLightBitMask = ~nightLightBitMask;
  Serial.print("Night light bit mask ");
  Serial.println(nightLightBitMask, BIN);
  animatedSwitchOff(NIGHT_LIGHT_BRIGHT);
  strip.clear();
  fillStepWithBitMask(0, mHSV(NIGHT_LIGHT_COLOR, 255, NIGHT_LIGHT_BRIGHT), nightLightBitMask);
  fillStepWithBitMask(STEP_AMOUNT - 1, mHSV(NIGHT_LIGHT_COLOR, 255, NIGHT_LIGHT_BRIGHT), nightLightBitMask);
  animatedSwitchOn(NIGHT_LIGHT_BRIGHT);
}

void handleTimeout() {
  if (millis() - timeoutCounter >= (TIMEOUT * 1000L)) {
    Serial.println("Timeout");
    systemIdleState = true;
      if (effectDirection == 1) {
          stepFader(0, 1);
      } else {
          stepFader(1, 1);
      }
      nightLight();
  }
}

void handlePirSensor(PirSensor *sensor) {
  if (systemOffState) return;

  int newState = digitalRead(sensor->pin);
  if (systemIdleState && newState && !sensor->lastState) {
    timeoutCounter = millis();
    Serial.print("PIR ");
    Serial.println(sensor->pin);
      effectDirection = sensor->effectDirection;
      if (ROTATE_EFFECTS) {
        curEffect = ++effectCounter % EFFECTS_AMOUNT;
      }
    systemIdleState = false;
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
