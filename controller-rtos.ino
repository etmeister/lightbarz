/*********************************************************************
  This is an example for our nRF52 based Bluefruit LE modules

  Pick one up today in the adafruit shop!

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  MIT license, check LICENSE for more information
  All text above, and the splash screen below must be included in
  any redistribution
*********************************************************************/
#include <PixelMaestro.h>
#include <core/maestro.h>
#include <colorpresets.h>
#include <core/colors.h>
#include <core/palette.h>
//#include <canvas/fonts/font3x5.h>
#include "jacobled.h"
#include "palettes.h"
#include "font.h"
#include <bluefruit.h>
#include <Arduino.h>
#define LOGO_OFFSET 5
#define NUM_LEDS 300
#define MIN_BRIGHT 0.2f
#define MAX_BRIGHT 1.0f
#define BRIGHT_STEP .05f
#define DEFAULT_BRIGHT .5f
#define NUM_ROWS 12
#define NUM_COLUMNS 76
#define FONT_X 5
#define FONT_Y 5

#define LED_PIN 2
#define LED2_PIN 3
#define LED3_PIN 5

//    READ_BUFSIZE            Size of the read buffer for incoming packets
#define READ_BUFSIZE (20)

/* Buffer to hold incoming characters */
uint8_t packetbuffer[READ_BUFSIZE + 1];

bool needsRedraw = false;
SemaphoreHandle_t maestroSem;
SemaphoreHandle_t packetSem;

Colors::RGB leds[NUM_LEDS];
Colors::RGB leds2[NUM_LEDS];
Colors::RGB leds3[NUM_LEDS];

bool logoSlide = false;
int8_t logoPos = 0;

// OTA DFU service
BLEDfu bledfu;

// Uart over BLE service
BLEUart bleuart;
SoftwareTimer Animator;

// Function prototypes for packetparser.cpp

// Packet buffer

Maestro maestro(NUM_COLUMNS, NUM_ROWS);
Section *section = maestro.get_section(0);

bool trackRpm = false;

//Adafruit_NeoPixel strip(NUM_LEDS, DATA_PIN, NEO_GRB + NEO_KHZ800);
const AnimationType animations[6] = {AnimationType::Wave, AnimationType::Fire, AnimationType::Plasma, AnimationType::Radial, AnimationType::Sparkle, AnimationType::Cycle};
Animation::Orientation orientations[2] = {Animation::Orientation::Vertical, Animation::Orientation::Horizontal};

// palettes.h - numPalettes
static const int numAnimations = 6;
static const int numOrientations = 2;

int currentAnimation = 0;
int currentOrientation = 0;
int currentPalette = 0;

bool preset = false;
float currentBright = DEFAULT_BRIGHT;

static const uint8_t z_num_bytes = 4;
static const char z_chars[] = {'3','7','0','Z'};
char custom_chars[16];
uint8_t num_custom_chars;

void fade_set(int color_value, int color_index, bool forceRedraw = true)
{
  Colors::RGB color = {0, 0, 0};
  switch (color_index)
  {
    case 0:
      color.r = color_value;
      break;
    case 1:
      color.g = color_value;
      break;
    case 2:
      color.b = color_value;
      break;
  }
  for (int led = 0; led < NUM_LEDS; led++)
  {
    leds[led] = color;
    leds2[led] = color;
    leds3[led] = color;
  }
  if (forceRedraw)
  {
    redraw();
  }
}

void fade_in(int color_index, int stop, int step = 1, int start = 0)
{
  if (start > stop)
    return;
  for (int i = start; i <= stop; i = i + step)
  {
    if (i > stop)
      i = stop;
    fade_set(i, color_index);
  }
}

void fade_out(int color_index, int start, int step = 1, int stop = 0)
{
  if (start < stop)
    return;
  for (int i = start; i >= stop; i = i - step)
  {
    if (i < stop)
      i = stop;
    fade_set(i, color_index);
  }
}

void setLogo(const char* logo_bytes, const uint8_t num_bytes, uint16_t pos_x = 0, uint16_t pos_y = 0, bool blank = true, bool forceRedraw = true)
{
  if (blank) {
    for (int led = 0; led < NUM_LEDS; led++)
    {
      leds[led] = ColorPresets::Black;
      leds2[led] = ColorPresets::Black;
      leds3[led] = ColorPresets::Black;
    }
  }
  for (uint8_t i = 0; i < num_bytes; i++)
  {
    if (logo_bytes[i] >= 0) {

      const uint8_t* current_char = font[logo_bytes[i]-32];
      for (uint16_t column = 0; column < FONT_X; column++) {
        for (uint16_t row = 0; row < FONT_Y; row++) {
          if ((current_char[column] >> row) & 1) {
            int16_t led_pos = (pos_x + column) * NUM_ROWS + pos_y + row;
            if (led_pos >= 0)
            {
              if (led_pos < 300)
              {
                leds[led_pos] = Colors::RGB(255, 255, 255);
              }
              else if (led_pos < 602)
              {
                if (led_pos != 300 && led_pos != 301)
                {
                  leds2[led_pos - 302] = Colors::RGB(255, 255, 255);
                }
              }
              else if (led_pos < 905)
              {
                if (led_pos != 602 && led_pos != 603 && led_pos != 604)
                {
                  leds3[led_pos - 605] = Colors::RGB(255, 255, 255);
                }
              }
            }
          }
        }
      }
    }
    // Move cursor to the location of the next letter based on the font size.
    pos_x += (FONT_X + 1  );
  }

  if (forceRedraw) {
    redraw();
  }
}

void setBlack(bool forceRedraw = true)
{
  for (int led = 0; led < NUM_LEDS; led++)
  {
    leds[led] = ColorPresets::Black;
    leds2[led] = ColorPresets::Black;
    leds3[led] = ColorPresets::Black;
  }
  if (forceRedraw)
  {
    redraw();
  }
}

void colorCheck()
{
  fade_in(1, (currentBright * 255), 4);
  fade_out(1, (currentBright * 255 - 1), 4);
  // start "off-screen" and scroll off the other side
  for (uint8_t i = 0; i < 98; i++)
  {
    setLogo(z_chars, z_num_bytes, i - 22, LOGO_OFFSET);
    delay(6);
  }
}

void redraw()
{

  if ( xSemaphoreTake( maestroSem, ( TickType_t ) pdMS_TO_TICKS(50) ) == pdTRUE ) {
    if (logoSlide && preset) {
      if (logoPos == NUM_COLUMNS+2) logoPos = 0-(num_custom_chars*(FONT_X+1));
      setLogo(custom_chars, num_custom_chars, logoPos, LOGO_OFFSET, false, false);
      logoPos++;
    }
    led_set_colors(leds, LED_PIN);
    led_set_colors(leds2, LED2_PIN);
    led_set_colors(leds3, LED3_PIN);
    xSemaphoreGive( maestroSem );
  }
}

void taskAnimate(TimerHandle_t Timer)
{
  (void)Timer;
  if ( xSemaphoreTake( maestroSem, ( TickType_t ) 0 ) == pdTRUE ) {

    if (preset)
    {
      if (maestro.update(millis()))
      {
        syncMaestro();
      }
    }
    xSemaphoreGive( maestroSem );

  }
}

void syncMaestro()
{
  //Serial.println(F("Maestro update"));
  uint16_t led = 0;
  for (uint8_t x = 0; x < section->get_dimensions().x; x++) {
    for (uint8_t y = 0; y < section->get_dimensions().y; y++) {
      Colors::RGB color = section->get_pixel_color(x, y);
      color.r = min(255, color.r * currentBright);
      color.g = min(255, color.g * currentBright);
      color.b = min(255, color.b * currentBright);
      if (led < 300) {
        leds[led] = color;
      } else if (led < 602) {
        if (led != 300 && led != 301) {
          leds2[led - 302] = color;
        }
      } else if (led < 905) {
        if (led != 602 && led != 603 && led != 604) {
          leds3[led - 605] = color;
        }
      }
      led++;
    }
  }
  needsRedraw = true;

}

void setup(void)
{
  Serial.begin(115200);
  while (!Serial)
    delay(10); // for nrf52840 with native usb
  Serial.println(F("LET THERE BE LIGHT!"));
  Serial.println(F("-------------------"));
  maestroSem = xSemaphoreCreateMutex();
  packetSem = xSemaphoreCreateMutex();

  nrf_gpio_cfg(LED_PIN,
               NRF_GPIO_PIN_DIR_INPUT,
               NRF_GPIO_PIN_INPUT_DISCONNECT,
               NRF_GPIO_PIN_PULLDOWN,
               NRF_GPIO_PIN_H0H1,
               NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_cfg(LED2_PIN,
               NRF_GPIO_PIN_DIR_INPUT,
               NRF_GPIO_PIN_INPUT_DISCONNECT,
               NRF_GPIO_PIN_PULLDOWN,
               NRF_GPIO_PIN_H0H1,
               NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_cfg(LED3_PIN,
               NRF_GPIO_PIN_DIR_INPUT,
               NRF_GPIO_PIN_INPUT_DISCONNECT,
               NRF_GPIO_PIN_PULLDOWN,
               NRF_GPIO_PIN_H0H1,
               NRF_GPIO_PIN_NOSENSE);

  colorCheck();
  maestro.set_brightness(255);
  maestro.set_timer(8);
  Animation &animation = section->set_animation(animations[currentAnimation]);
  animation.set_palette(palettes[currentPalette]);
  animation.set_orientation(orientations[currentOrientation]);
  animation.set_timer(240);
  //animation.set_fade(false);

  Animator.begin(2, taskAnimate);
  Animator.start();

  Bluefruit.begin();
  Bluefruit.setTxPower(4); // Check bluefruit.h for supported values
  Bluefruit.setName("LIGHTBARZ");
  // Configure and start the BLE Uart service
  bleuart.begin();
  bleuart.setRxCallback(callbackPacket);
  // Set up and start advertising
  startAdv();

  //logoSlide = true;
  Serial.println(F("And there was light."));
  //  Scheduler.startLoop(taskBLE,256*5);
}

void loop(void)
{
  while (!needsRedraw)
    yield();
  needsRedraw = false;
  redraw();
}

void callbackPacket(uint16_t handle)
{
  if ( xSemaphoreTake( packetSem, ( TickType_t ) pdMS_TO_TICKS(50) ) == pdTRUE ) {

    uint8_t replyidx = 0;
    memset(packetbuffer, 0, READ_BUFSIZE);
    char c = bleuart.read();
    if (c == '!')
    {
      packetbuffer[replyidx] = c;
      Serial.print(c);
      replyidx++;
    while (bleuart.available())
    {
    char c = bleuart.read();
      packetbuffer[replyidx] = c;
      Serial.print(c);
      replyidx++;
    }
    }
    packetbuffer[replyidx] = 0; // null term

    if (!replyidx) // no data or timeout
      return;
    Serial.println("");
    handlePacket(replyidx);
    xSemaphoreGive( packetSem );
  }
}



uint8_t rpm;
uint8_t buttnum;
boolean pressed;
void handlePacket(uint8_t packetlength) {
      Serial.println(dbgHeapTotal() - dbgHeapUsed());

  if ( xSemaphoreTake( maestroSem, ( TickType_t ) pdMS_TO_TICKS(50) ) == pdTRUE ) {
    if (packetbuffer[0] == '!') {
      // Buttons
      if (packetbuffer[1] == 'T') {
        Serial.println(F("Toggling Maestro"));
        bleuart.write("Toggling display");
        toggleMaestro();
      }
      Serial.println((char*) packetbuffer);
      if (preset) {
        Serial.println(F("Preset enabled, checking button command"));
        switch (packetbuffer[1]) {
          case 'A':
            Serial.println("animation");
            bleuart.write("animation changed");
            cycleAnimation();
            break;
          case 'P':
            Serial.println("palette");
            bleuart.write("palette changed");
            cyclePalette();
            break;
          case 'O':
            Serial.println("orientation");
            bleuart.write("orientation changed");
            cycleOrientation();
            break;
          case 'B':
            Serial.print(F("Brightness "));
            if (packetbuffer[2] == 'U') {
              Serial.println("up");
              brightnessUp();
            } else {
              Serial.println("down");
              brightnessDown();
            }
            break;
          case 'V':
            if (packetbuffer[2] == '1') {
              Serial.println(F("Tracking RPM for brightness"));
              bleuart.write("tracking rpm");

              trackRpm = true;
            } else {
              Serial.println(F("Disabling rpm tracking"));
              bleuart.write("not tracking");
              trackRpm = false;
            }
            break;
          case 'R':
            rpm = packetbuffer[2];
            if (trackRpm) {
              Serial.print(F("RPM "));
              Serial.println(rpm);
              currentBright = (float(rpm) / 255.0f);
              if (currentBright > 1.0f) currentBright = 1.0f;
              needsRedraw = true;
            }
            break;
          case 'S':
            Serial.print(F("Toggle custom string "));
          if (packetlength > 2) {
                Serial.print("on:");
                for(uint8_t i = 0;i<= packetlength; i++) {
                  custom_chars[i] = packetbuffer[i+2];
                }
                Serial.println(custom_chars);
                bleuart.write("string set");
                num_custom_chars = packetlength-2;
                logoPos = 0-(num_custom_chars*(FONT_X+1));
                logoSlide = true;
                needsRedraw = true;
          } else {
            Serial.println("off");
                logoSlide = false;
                needsRedraw = true;
          }
        }
      }
    }
    xSemaphoreGive( maestroSem );
  }
  /*
        case 'C':  {
            uint8_t red = packetbuffer[2];
            uint8_t green = packetbuffer[3];
            uint8_t blue = packetbuffer[4];
            Serial.print("RGB #");
            if (red < 0x10)
              Serial.print("0");
            Serial.print(red, HEX);
            if (green < 0x10)
              Serial.print("0");
            Serial.print(green, HEX);
            if (blue < 0x10)
              Serial.print("0");
            Serial.println(blue, HEX);
            preset = false;
            for (int led = 0; led < NUM_LEDS; led++) {
              leds[led].r = red;
              leds[led].g = green;
              leds[led].b = blue;
              leds2[led] = leds[led];
              leds3[led] = leds[led];
            }
            needsRedraw = true;
            break;
          }
      }*/
}

void toggleMaestro() {
  //if (!preset)
  preset = !preset;
  if (!preset)
  {
    setBlack(false);
    needsRedraw = true;
  }
}



void cycleAnimation() {
  if (currentAnimation < (numAnimations - 1))
  {
    currentAnimation++;
  }
  else
  {
    currentAnimation = 0;
  }

  section->remove_animation(false);
  Animation &animation = section->set_animation(animations[currentAnimation]);
  animation.set_palette(palettes[currentPalette]);
  animation.set_orientation(orientations[currentOrientation]);
  animation.set_timer(240);
}

void cyclePalette() {
  if (currentPalette < (numPalettes - 1))
  {
    currentPalette++;
  }
  else
  {
    currentPalette = 0;
  }
  section->get_animation()->set_palette(palettes[currentPalette]);
}

void cycleOrientation() {
  if (currentOrientation < (numOrientations - 1))
  {
    currentOrientation++;
  }
  else
  {
    currentOrientation = 0;
  }
  section->get_animation()->set_orientation(orientations[currentOrientation]);
}

void brightnessUp() {
  if (currentBright < MAX_BRIGHT)
  {

    currentBright = currentBright + BRIGHT_STEP;
    Serial.print(F("Brightness set to "));
    Serial.println((uint8_t)(currentBright * 255));
    bleuart.write((uint8_t)(currentBright * 255));

    needsRedraw = true;
  }
}

void brightnessDown() {
  if (currentBright > MIN_BRIGHT) {
    currentBright = currentBright - BRIGHT_STEP;
    Serial.print(F("Brightness set to "));
    Serial.println((uint8_t)(currentBright * 255));
    bleuart.write((uint8_t)(currentBright * 255));
    needsRedraw = true;
  }
}

void startAdv(void)
{
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();

  // Include the BLE UART (AKA 'NUS') 128-bit UUID
  Bluefruit.Advertising.addService(bleuart);

  // Secondary Scan Response packet (optional)
  // Since there is no room for 'Name' in Advertising packet
  Bluefruit.ScanResponse.addName();

  /* Start Advertising
    - Enable auto advertising if disconnected
    - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
    - Timeout for fast mode is 30 seconds
    - Start(timeout) with timeout = 0 will advertise forever (until connected)

    For recommended advertising interval
    https://developer.apple.com/library/content/qa/qa1931/_index.html
  */
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244); // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);   // number of seconds in fast mode
  Bluefruit.Advertising.start(0);             // 0 = Don't stop advertising after n seconds
}
