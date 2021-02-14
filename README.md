# LEDstairs
Контроллер подсветки лестницы на Arduino  

Внимание, для WS2811 кол-во светодиодов делите на 3!

## Новые функции в прошивке 1.2 (BETA)
***
### Разное количество диодов на ступеньку
```c
#define STRIP_LED_AMOUNT 256  // кол-во светодиодов на всех ступеньках
#define STEP_AMOUNT 16        // количество ступенек

// описание всех ступенек с возможностью подсветки ЛЮБЫХ ступенек в ночном режиме
Step steps[STEP_AMOUNT] = { 
{ 16, 0b0100100100100100 },   // первая ступенька 16 чипов, 0b0100100100100100 - каждый третий чип активен в ночном режиме
{ 16, 0b0000000000000000 },   // вторая ступенька 16 чипов, 0b0000000000000000 - не активен в ночном режиме
...
};
```
второе поле описывает битовую маску подсветки. Если в маске все нули - ступенька не светится, если есть ненулевые биты - светятся соответвующие диоды. 
Можно будет гибко настраивать какие стпени и с какой интенсивностью светятся ночью
### Сенсорная кнопка для переключения эффектов
```c
#define BUTTON  0      // вкл(1)/выкл(0) - сенсорная кнопка переключения эффектов

#define BUTTON_PIN 6     // пин сенсорной кнопки переключения эффектов

```
## Новые функции в прошивке 1.1
***
### поддержка **ночного режима**. Подсвечиваются крайние ступеньки.
  Настройка:
  * маска (какие диоды на ступеньке включать) NIGHT_LIGHT_BIT_MASK
  * цвет NIGHT_LIGHT_COLOR
  * яркость NIGHT_LIGHT_BRIGHT

```c
int16_t NIGHT_LIGHT_BIT_MASK = 0b0100100100100100;  // последовательность диодов в ночном режиме, чтобы диоды не выгорали
#define NIGHT_LIGHT_COLOR mCOLOR(WHITE)  // по умолчанию белый
#define NIGHT_LIGHT_BRIGHT 10  // 0 - 255
```
Чтобы не выжигать одни и теже диоды, маска инвертируется каждые 60 секунд  
Цвет ночной подсветки можно определить через функции библиотеки microLED:
* mCOLOR(WHITE) или SILVER,GRAY,BLACK,RED,MAROON,ORANGE,YELLOW,OLIVE,LIME,GREEN,AQUA,TEAL,BLUE,NAVY,MAGENTA,PURPLE
* mRGB(byte r, byte g, byte b);   // RGB 255, 255, 255
* mWHEEL(int color);              // цвета 0-1530
* mHEX(uint32_t color);           // HEX цвет
* mHSV(byte h, byte s, byte v);   // HSV 255, 255, 255

***
### включение только при минимальной освещённости

***
### включение подсветки по одному из датчиков движения, а выключение только по таймеру  
Если сработал датчик с одной стороны, то до истечения времени таймаута каждое срабатывание одного из датчиков приводит к сбросу счётчика.
В таком случае даже при нахождении на лестнице нескольких персон и движении в разных направлениях никто не останется в темноте.

***
### подсветка перил  (по умолчанию отключена)  
Настройки:
```c
#define RAILING 0      // вкл(1)/выкл(0) - подсветка перил
#define RAILING_LED_AMOUNT 50    // количество чипов WS2811 на ленте перил
#define RAILING_PIN 11    // пин ленты перил
```
Вся лента перил делится на сегменты. Их столько же, сколько и ступенек.
На каждом сегменте перил отрисовывается тот же эффект, что и на соответствующей ступеньке.
Ночная подсветка также подсвечивает крайние сегменты перил

---
Автор: Геннадий Дегтерёв  
gennadij@degterjow.de


* Основная страница оригинального проекта здесь https://alexgyver.ru/ledstairs/

