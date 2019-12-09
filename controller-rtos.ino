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

#define LOGO_OFFSET 7
#define NUM_LEDS 300
#define MIN_BRIGHT 0.2f
#define MAX_BRIGHT 1.0f
#define BRIGHT_STEP .05f
#define LOGO_BRIGHT .2f
#define DEFAULT_BRIGHT .5f
#define NUM_ROWS 12
#define NUM_COLUMNS 76
#define FONT_X 5
#define FONT_Y 5
#define REFRESH 15
#define DEFAULT_TIMER 120
#define LED_PIN 2
#define LED2_PIN 3
#define LED3_PIN 5
//    READ_BUFSIZE            Size of the read buffer for incoming packets
#define READ_BUFSIZE (20)

/* Buffer to hold incoming characters */

SemaphoreHandle_t maestroSem;
SemaphoreHandle_t pixelSem;
SemaphoreHandle_t packetSem;

Colors::RGB leds[NUM_LEDS];
Colors::RGB leds2[NUM_LEDS];
Colors::RGB leds3[NUM_LEDS];
char rpmChars[4];
uint8_t numRpmChars = 0;
int8_t rpmPos = 0; // 26
bool maestroActive = false;
float currentBright = DEFAULT_BRIGHT;
int logoOffset = LOGO_OFFSET;
bool logoSlide = false;
int8_t logoPos = 8;
int lastSlide = 0;
int logoDelay = 20;
uint16_t animDelay = DEFAULT_TIMER;

bool trackRpm = false;
uint16_t currentRpm = 0;
float maxRpm = 7600.0f;

static const uint8_t z_num_bytes = 4;
static const char z_chars[] = "370Z";
char custom_chars[16];
uint8_t num_custom_chars;

Maestro maestro(NUM_COLUMNS, NUM_ROWS);
Section *section = maestro.get_section(0);

const AnimationType animations[6] = {AnimationType::Wave, AnimationType::Fire, AnimationType::Plasma, AnimationType::Radial, AnimationType::Sparkle, AnimationType::Cycle};
const Animation::Orientation orientations[2] = {Animation::Orientation::Vertical, Animation::Orientation::Horizontal};
static const int numAnimations = 6;
static const int numOrientations = 2;
int currentAnimation = 0;
int currentOrientation = 0;
int currentPalette = 0;

char packetreceivebuffer[READ_BUFSIZE + 1];
// OTA DFU service
BLEDfu bledfu;
// Uart over BLE service
BLEUart bleuart;


/*

    Animation helpers

*/

void fade_set(int color_value, int color_index, bool forceRedraw = true)
{
  if ( xSemaphoreTake( pixelSem, ( TickType_t ) pdMS_TO_TICKS(50) ) == pdTRUE ) {

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
    xSemaphoreGive( pixelSem );
    if (forceRedraw)
    {
      redraw();
    }
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


void setBlack(bool forceRedraw = true)
{
  if ( xSemaphoreTake( pixelSem, ( TickType_t ) pdMS_TO_TICKS(50) ) == pdTRUE ) {
    for (int led = 0; led < NUM_LEDS; led++)
    {
      leds[led] = ColorPresets::Black;
      leds2[led] = ColorPresets::Black;
      leds3[led] = ColorPresets::Black;
    }
    xSemaphoreGive(pixelSem);
    if (forceRedraw)
    {
      redraw();
    }
  }
}

void callWithMutex(void (*function) ()) {
  // DEBUG Serial.print("Checking semaphore");
  if ( xSemaphoreTake( maestroSem, ( TickType_t ) pdMS_TO_TICKS(50) ) == pdTRUE ) {
    // DEBUG Serial.println(" ... obtained.");
    function();
    // DEBUG Serial.print("Releasing semaphore");
    xSemaphoreGive( maestroSem );
  }
  // DEBUG Serial.println(" ... done.");
}
void setLogo(const char* logo_bytes, const uint8_t num_bytes, uint16_t pos_x = 0, uint16_t pos_y = 0, bool blank = true, bool forceRedraw = true)
{
  float brighter = currentBright;
  if (currentBright + LOGO_BRIGHT < MAX_BRIGHT ) {
    brighter += LOGO_BRIGHT;
  } else {
    brighter = MAX_BRIGHT;
  }
  if (blank) {
    setBlack(false);
  }
  if ( xSemaphoreTake( pixelSem, ( TickType_t ) 0 ) == pdTRUE ) {
    for (uint8_t i = 0; i < num_bytes; i++)
    {
      if (logo_bytes[i] >= 0) {

        const uint8_t* current_char = font[logo_bytes[i] - 32];
        for (uint16_t column = 0; column < FONT_X; column++) {
          for (uint16_t row = 0; row < FONT_Y; row++) {
            if ((current_char[column] >> row) & 1) {
              int16_t led_pos = (pos_x + column) * NUM_ROWS + pos_y + row;
              if (led_pos >= 0)
              {
                if (led_pos < 300)
                {
                  leds[led_pos] = Colors::RGB(255 * brighter, 255 * brighter, 255 * brighter);
                }
                else if (led_pos < 602)
                {
                  if (led_pos != 300 && led_pos != 301)
                  {
                    leds2[led_pos - 302] =  Colors::RGB(255 * brighter, 255 * brighter, 255 * brighter);
                  }
                }
                else if (led_pos < 905)
                {
                  if (led_pos != 602 && led_pos != 603 && led_pos != 604)
                  {
                    leds3[led_pos - 605] =  Colors::RGB(255 * brighter, 255 * brighter, 255 * brighter);
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
    xSemaphoreGive(pixelSem);
    if (forceRedraw) {
      redraw();
    }
  }
}


void colorCheck()
{
  fade_in(1, (currentBright * 255), 4);
  fade_out(1, (currentBright * 255 - 1), 4);
  // start "off-screen" and scroll off the other side
  for (uint8_t i = 0; i < 98; i++)
  {
    setLogo(z_chars, z_num_bytes, i - 22, logoOffset);
    delay(12);
  }
}

void redraw()
{
  if (maestroActive) {
    if (trackRpm) {
      setLogo(rpmChars, numRpmChars, rpmPos, logoOffset, false, false);
    } else if (logoSlide ) {
      if (logoPos == NUM_COLUMNS + 2) logoPos = 0 - (num_custom_chars * (FONT_X + 1));
      setLogo(custom_chars, num_custom_chars, logoPos, logoOffset, false, false);
      if (millis() > (lastSlide + logoDelay)) {
        lastSlide = millis();
        logoPos++;
      }
    }
  }
  if ( xSemaphoreTake( pixelSem, ( TickType_t ) 0 ) == pdTRUE ) {
    //// DEBUG Serial.print("pre-set: ");
    //// DEBUG Serial.println(millis());
    led_set_colors(leds, LED_PIN, leds2, LED2_PIN, leds3, LED3_PIN);
    // // DEBUG Serial.print("post-set: ");
    //  // DEBUG Serial.println(millis());

    //// DEBUG Serial.println(dbgHeapTotal() - dbgHeapUsed());
    xSemaphoreGive( pixelSem );
  }
}

boolean maestroAnimator()
{
  boolean updated = false;
  if ( xSemaphoreTake( maestroSem, ( TickType_t ) 0 ) == pdTRUE ) {
    if (maestro.update(millis()))
    {
      syncMaestro();
      updated = true;
    }
    xSemaphoreGive( maestroSem );
  }
  return updated;
}

void syncMaestro()
{
  //// DEBUG Serial.println(F("Maestro update"));
  uint16_t led = 0;
  float percent;
  if ( xSemaphoreTake( pixelSem, ( TickType_t ) pdMS_TO_TICKS(50) ) == pdTRUE ) {
    if (trackRpm) {
      currentRpm = atoi(rpmChars);
      currentBright = currentRpm / maxRpm;
      if (currentBright > MAX_BRIGHT) currentBright = MAX_BRIGHT;
      if (currentBright < MIN_BRIGHT) currentBright = MIN_BRIGHT;
      section->get_animation()->set_timer(animDelay*(MAX_BRIGHT-currentBright+MIN_BRIGHT));
      }
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
    xSemaphoreGive( pixelSem );

  }

}




/*

   Command Functions

*/
bool (*animatorFunction) ();
bool noopAnimator() {
  return false;
}
void toggleMaestro() {
  maestroActive = !maestroActive;
  if (!maestroActive)
  {
    animatorFunction = &noopAnimator;
    setBlack(true);
  } else {
    animatorFunction = &maestroAnimator;

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
  // DEBUG Serial.println("removing section");
  section->remove_animation(false);
  Serial.println("Setting animation");
  Animation &animation = section->set_animation(animations[currentAnimation]);
  Serial.println("Setting palette");
  animation.set_palette(palettes[currentPalette]);
  Serial.println("Setting orientation");
  animation.set_orientation(orientations[currentOrientation]);
  Serial.println("Setting delay");
  animation.set_timer(animDelay);
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
    //bleuart.write((uint8_t)(currentBright * 255));

  }
}

void brightnessDown() {
  if (currentBright > MIN_BRIGHT) {
    currentBright = currentBright - BRIGHT_STEP;
    Serial.print(F("Brightness set to "));
    Serial.println((uint8_t)(currentBright * 255));
    //bleuart.write((uint8_t)(currentBright * 255));
  }
}


/*

   Arduino functions

*/

void setup(void)
{
  Serial.begin(115200);
  while (!Serial)
    delay(10); // for nrf52840 with native usb
  Serial.println(F("LET THERE BE LIGHT!"));
  Serial.println(F("-------------------"));
  maestroSem = xSemaphoreCreateMutex();
  pixelSem = xSemaphoreCreateMutex();
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
  maestro.set_timer(REFRESH);
  Animation &animation = section->set_animation(animations[currentAnimation]);
  animation.set_palette(palettes[currentPalette]);
  animation.set_orientation(orientations[currentOrientation]);
  animation.set_timer(animDelay);
  //animation.set_fade(false);

  animatorFunction = &noopAnimator;
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
  if (animatorFunction()) {
    redraw();
  }
}

/*

   Bluetooth LE Functions

*/
void callbackAdv() {
  Serial.println("Connected?");
}


void callbackPacket(uint16_t handle)
{
  if ( xSemaphoreTake( packetSem, ( TickType_t ) pdMS_TO_TICKS(50) ) == pdTRUE ) {

    uint8_t replyidx = 0;
    replyidx = bleuart.available();
    bleuart.read(packetreceivebuffer, replyidx);
    // DEBUG Serial.println(packetbuffer);
    handlePacket(packetreceivebuffer, replyidx);
    xSemaphoreGive( packetSem );
  }
}

void setDelay() {
  section->get_animation()->set_timer(animDelay);
}

void handlePacket(char *packetbuffer, uint8_t packetlength) {
  Serial.println(packetbuffer);
  uint8_t packetPos = 0;
  if (packetbuffer[0] == '!' && packetbuffer[1] == 'R' && !trackRpm) return;
  if (packetbuffer[0] == '!') {
    // Buttons
    if (packetbuffer[1] == 'T') {
      Serial.println(F("Toggling Maestro"));
      bleuart.write("Toggling display");
      toggleMaestro();
      packetPos = 1;

    }
    if (maestroActive) {
      switch (packetbuffer[1]) {
        case 'A':
          Serial.println("animation");
          bleuart.write("animation changed");
          callWithMutex(&cycleAnimation);
          Serial.println(dbgHeapTotal() - dbgHeapUsed());
          packetPos = 1;
          break;
        case 'P':
          Serial.println("palette");
          bleuart.write("palette changed");
          callWithMutex(&cyclePalette);
          packetPos = 1;
          break;
        case 'O':
          Serial.println("orientation");
          bleuart.write("orientation changed");
          callWithMutex(&cycleOrientation);
          packetPos = 1;
          break;
        case 'D': {
            char newDelay[4];
            for (packetPos = 2; packetPos < packetlength; packetPos++) {
              if (packetbuffer[packetPos] == '!') { packetPos--; break; }
              newDelay[packetPos - 2] = packetbuffer[packetPos];
              
            }
            animDelay = atoi(newDelay);
            callWithMutex(&setDelay);
            break;
          }
        case 'Y': {
            char yPos[3];
            for (packetPos = 2; packetPos < packetlength; packetPos++) {
              if (packetbuffer[packetPos] == '!') { packetPos--; break; }
              yPos[packetPos - 2] = packetbuffer[packetPos];
            }
            logoOffset = atoi(yPos);
            Serial.println("logo offset");
            bleuart.write("logo offset changed");
            break;
          }
        case 'B':
          Serial.print(F("Brightness "));
          if (packetbuffer[2] == 'U') {
            Serial.println("up");
            brightnessUp();
          } else {
            Serial.println("down");
            brightnessDown();
          }
          packetPos = 2;
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
          packetPos = 2;
          break;
        case 'R':
          if (trackRpm) {
            for (packetPos = 2; packetPos < packetlength; packetPos++) {
              if (packetbuffer[packetPos] == '!') { packetPos--; break; }
              rpmChars[packetPos - 2] = packetbuffer[packetPos];
            }
            numRpmChars = packetPos - 2;
            if(numRpmChars < 4) rpmChars[3] = 0;
          }
          break;
        case 'S':
          Serial.print(F("Toggle custom string "));
          if (packetlength > 2) {
            Serial.print("on:");
            for (packetPos= 2; packetPos < packetlength; packetPos++) {
              custom_chars[packetPos - 2] = packetbuffer[packetPos];
            }
            Serial.println(custom_chars);
            bleuart.write("string set");
            num_custom_chars = packetlength - 2;
            logoPos = 0 - (num_custom_chars * (FONT_X + 1));
            logoSlide = true;
          } else {
            Serial.println("off");
            logoSlide = false;
          }
      }
    }
    if ((packetPos + 1) < packetlength) {
          Serial.println("recursing");
          packetPos++;
          char *secondSet = &packetbuffer[packetPos];
          handlePacket(secondSet, packetlength-packetPos);

    }
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
            maestroActive = false;
            for (int led = 0; led < NUM_LEDS; led++) {
              leds[led].r = red;
              leds[led].g = green;
              leds[led].b = blue;
              leds2[led] = leds[led];
              leds3[led] = leds[led];
            }
            break;
          }
      }*/
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
