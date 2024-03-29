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
#define LOGO_BRIGHT .15f
#define DEFAULT_BRIGHT 0.8f
#define NUM_ROWS 12
#define NUM_COLUMNS 76
#define FONT_X 5
#define FONT_Y 5
#define REFRESH 15
#define DEFAULT_TIMER 120
#define LED_PIN 2
#define LED2_PIN 3
#define LED3_PIN 5
#define MAX_RPM 7600.0f
#define RPM_POS 26
#define NUM_ANIMATIONS 6
#define NUM_ORIENTATIONS 2


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
float currentBright = DEFAULT_BRIGHT;
int logoOffset = LOGO_OFFSET;
bool logoSlide = true;
bool logoEnabled = false;
int8_t logoDelay = 50;
int8_t logoPos = NUM_COLUMNS + 2;
int nextSlide = 0;
uint8_t lastRedraw = 0;

uint16_t animDelay = DEFAULT_TIMER;
bool invertStrips = false;
bool trackRpm = false;
uint16_t currentRpm = 0;

static const uint8_t z_num_bytes = 4;
static const char z_chars[] = "370Z";
char custom_chars[16];
uint8_t num_custom_chars;

Maestro maestro(NUM_COLUMNS, NUM_ROWS);
Section *section = maestro.get_section(0);

const AnimationType animations[NUM_ANIMATIONS] = {AnimationType::Wave, AnimationType::Fire, AnimationType::Plasma, AnimationType::Radial, AnimationType::Sparkle, AnimationType::Cycle};
const Animation::Orientation orientations[NUM_ORIENTATIONS] = {Animation::Orientation::Vertical, Animation::Orientation::Horizontal};
int currentAnimation = 0;
int currentOrientation = 0;
int currentPalette = 0;
int currentCycle = 0;
char packetreceivebuffer[READ_BUFSIZE + 1];

// Uart over BLE service
BLEUart bleuart;


/*

    Animation helpers

*/


// Move cursor to the location of the next letter based on the font size.



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





void setColor(Colors::RGB color, bool forceRedraw = true) {

  if ( xSemaphoreTake( pixelSem, ( TickType_t ) pdMS_TO_TICKS(50) ) == pdTRUE ) {
    for (int led = 0; led < NUM_LEDS; led++)
    {
      leds[led] = color;
      leds2[led] = color;
      leds3[led] = color;
    }
    xSemaphoreGive(pixelSem);
    if (forceRedraw)
    {
      redraw();
    }
  }
}
void setBlack(bool forceRedraw = true) {
  setColor(ColorPresets::Black, forceRedraw);
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

void callCharWithMutex(void (*function) (char*, uint8_t&), char *arguments, uint8_t &argumentLength) {
  // DEBUG Serial.print("Checking semaphore");
  if ( xSemaphoreTake( maestroSem, ( TickType_t ) pdMS_TO_TICKS(50) ) == pdTRUE ) {
    // DEBUG Serial.println(" ... obtained.");
    function(arguments, argumentLength);
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
              setLedColor(led_pos, Colors::RGB(255 * brighter, 255 * brighter, 255 * brighter));
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

void setLedColor(int16_t led_pos, Colors::RGB color) {
  if (invertStrips) led_pos = 905 - led_pos;
  if (led_pos >= 0) {
    if (led_pos < 300) {
      leds[led_pos] = color;
    } else if (led_pos < 602) {
      if (led_pos != 300 && led_pos != 301) {
        leds2[led_pos - 302] =  color;
      }
    } else if (led_pos < 905) {
      if (led_pos != 602 && led_pos != 603 && led_pos != 604) {
        leds3[led_pos - 605] =  color;
      }
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
  lastRedraw = currentCycle;
  if (trackRpm) {
    setLogo(rpmChars, numRpmChars, RPM_POS, logoOffset, false, false);
  } else if (logoEnabled) {
    if(logoSlide) {
      if (logoPos <= (0 - (num_custom_chars * (FONT_X + 1)))) logoPos = NUM_COLUMNS + 2;
      setLogo(custom_chars, num_custom_chars, logoPos, logoOffset, false, false);
      if (currentCycle >= nextSlide) {
        nextSlide = currentCycle + logoDelay;
        logoPos--;
      }
    } else {
      logoPos = (NUM_COLUMNS - (num_custom_chars * (FONT_X + 1)))/2;
      setLogo(custom_chars, num_custom_chars, logoPos, logoOffset, false, false);
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
boolean logoAnimator() {
    bool updated = false;
    if (trackRpm) {
      if (currentCycle >= (lastRedraw + REFRESH)) updated = true;
    } else if (logoSlide) {
      if (currentCycle >= nextSlide) updated = true;
    }
    if (updated) {
      setBlack(false);
    }
    return updated;
}

boolean maestroAnimator()
{
  boolean updated = false;
  if ( xSemaphoreTake( maestroSem, ( TickType_t ) 0 ) == pdTRUE ) {
    if (maestro.update(currentCycle))
    {
      syncMaestro();
      updated = true;
    }
    xSemaphoreGive( maestroSem );
  }
  return updated;
}

uint8_t kittColorChannel = 0;
uint8_t kittTailLength = 20;
uint8_t currentKittTailLength = 0;
uint8_t kittDelay = 50;
uint8_t kittHeight = 12;
float kittEndFactor = 6;
int8_t kittStartPosition = 20;
uint8_t kittEndPosition = 54;
uint8_t kittShrinking = 0;
boolean kittDirection = true;
int8_t kittPosition = kittStartPosition;
int kittNextRun = 0;

boolean kitt() {
  if ( kittNextRun <= currentCycle) {
    setBlack(false);
    if (kittShrinking > 0) {
      currentKittTailLength = kittTailLength / kittEndFactor * kittShrinking;
      if (!kittDirection) {
          kittPosition = kittStartPosition + currentKittTailLength;
      }
    } else {
      currentKittTailLength = kittTailLength;
    }
    if (xSemaphoreTake( pixelSem, ( TickType_t ) 0 ) == pdTRUE ) {
      uint8_t colorValue;
      if (kittDirection) {
        colorValue = 255;
      } else {
        colorValue = 0;
      }
      Colors::RGB color = {0, 0, 0};

      for (int8_t x = kittPosition; x >= (kittPosition - currentKittTailLength); x--)
      {
        if (x >= kittStartPosition && x <= kittEndPosition) {
          switch (kittColorChannel)
          {
            case 0:
              color.r = colorValue * currentBright;
              break;
            case 1:
              color.g = colorValue * currentBright;
              break;
            case 2:
              color.b = colorValue * currentBright;
              break;
          }
          for (uint8_t y = logoOffset; y < logoOffset + kittHeight; y++) {
            int16_t led_pos = x * NUM_ROWS + y;
            setLedColor(led_pos, color);
          }
        }
        if (kittDirection) {
          colorValue = colorValue - (255 / currentKittTailLength);
        } else {
          colorValue = colorValue + (255 / currentKittTailLength);
        }
      }
      xSemaphoreGive(pixelSem);
      kittNextRun = currentCycle + kittDelay;

      if (kittShrinking > 1) {
        kittShrinking -= 1;
      } else if (kittShrinking == 1) {
          kittShrinking = 0;
          if (!kittDirection) {
            kittPosition = kittStartPosition;
          } else if (kittDirection) {
            kittPosition = kittEndPosition + kittTailLength;
          }
          kittDirection = !kittDirection;
      } else {
        if (!kittDirection && kittPosition <= (kittTailLength + kittStartPosition)) {
          kittShrinking = kittEndFactor;
        } else if (kittDirection && kittPosition >= kittEndPosition) {
          kittShrinking = kittEndFactor;
        } else {
          if (kittDirection) {
            kittPosition++;
          } else {
            kittPosition--;
          } 
        }
      }
      return true;

    }
  }
  return false;
}

void syncMaestro()
{
  //// DEBUG Serial.println(F("Maestro update"));
  uint16_t led = 0;
  float percent;
  if ( xSemaphoreTake( pixelSem, ( TickType_t ) pdMS_TO_TICKS(50) ) == pdTRUE ) {
    if (trackRpm) {
      currentRpm = atoi(rpmChars);
      currentBright = currentRpm / MAX_RPM;
      if (currentBright > MAX_BRIGHT) currentBright = MAX_BRIGHT;
      if (currentBright < MIN_BRIGHT) currentBright = MIN_BRIGHT;
      section->get_animation()->set_timer(max(REFRESH, animDelay * (MAX_BRIGHT - currentBright)));
    }
    for (uint8_t x = 0; x < section->get_dimensions().x; x++) {
      for (uint8_t y = 0; y < section->get_dimensions().y; y++) {
        Colors::RGB color = section->get_pixel_color(x, y);
        color.r = min(255, color.r * currentBright);
        color.g = min(255, color.g * currentBright);
        color.b = min(255, color.b * currentBright);
        setLedColor(led, color);
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

void disableAnimator(bool clearExtra = true) {
  if (clearExtra) { 
    trackRpm = false;
    logoEnabled = false;
  }
  if (trackRpm || logoEnabled) {
    animatorFunction = &logoAnimator;
  } else {
    animatorFunction = &noopAnimator;
  }
  setBlack(true);
}
void toggleLogo(bool softEnable = false) {
  if (animatorFunction != &logoAnimator) {
    logoEnabled = true;
    if (!softEnable || animatorFunction == &noopAnimator) {
       animatorFunction = &logoAnimator;
    }
  } else if (!softEnable) {
    logoEnabled = false;
    if (!trackRpm) disableAnimator(false);
  }


}
void toggleMaestro() {
  if (animatorFunction != &maestroAnimator) {
    animatorFunction = &maestroAnimator;
  } else {
    disableAnimator(false);
  }
}

void handleKitt(char* arguments, uint8_t &argumentLength) {
  if (argumentLength > 0) {
    Serial.println("processing kitt settings");
    bleuart.write("kitt updated");  
    switch (arguments[0]) {
      case 'C':
          kittColorChannel = atoi(&arguments[1]);
          break;
      case 'D':
          kittDelay = atoi(&arguments[1]);
          break;
      case 'E':
          kittEndPosition = atoi(&arguments[1]);
          break;
      case 'H':
          kittHeight = atoi(&arguments[1]);;
          break;
      case 'M':
          kittEndFactor = atoi(&arguments[1]);
          break;
      case 'S':
          kittStartPosition = atoi(&arguments[1]);
          break;
      case 'T':
          kittTailLength = atoi(&arguments[1]);
          break;
    }
  } else {
    Serial.println("Toggling kitt");
    bleuart.write("kitt toggled");
    toggleKitt();
  }
}

void toggleKitt() {
  if (animatorFunction != &kitt) {
    kittNextRun = 0;
    animatorFunction = &kitt;
  } else {
    disableAnimator(false);
  }
}

void cycleAnimation() {
  if (currentAnimation < (NUM_ANIMATIONS - 1))
  {
    currentAnimation++;
  }
  else
  {
    currentAnimation = 0;
  }
  updateAnimation();
}

void setAnimation(char* arguments, uint8_t &argumentLength) {
  currentAnimation = (atoi(arguments) % NUM_ANIMATIONS);
  updateAnimation();
}

void updateAnimation() {
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

void setPalette(char* arguments, uint8_t &argumentLength) {
  currentPalette = (atoi(arguments) % numPalettes);
  section->get_animation()->set_palette(palettes[currentPalette]);
}

void cycleOrientation() {
  if (currentOrientation < (NUM_ORIENTATIONS - 1))
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
  currentCycle = millis();
  if (animatorFunction()) {
    redraw();
  } else if (logoEnabled && logoSlide && currentCycle >= nextSlide) {
    nextSlide = currentCycle + logoDelay;
    logoPos--;
  }
}

/*

   Bluetooth LE Functions

*/

void callbackPacket(uint16_t handle)
{
  if ( xSemaphoreTake( packetSem, ( TickType_t ) pdMS_TO_TICKS(50) ) == pdTRUE ) {

    uint8_t replyidx = bleuart.available();
    if (replyidx > READ_BUFSIZE) replyidx = READ_BUFSIZE;
    memset(packetreceivebuffer, 0, READ_BUFSIZE);
    bleuart.read(packetreceivebuffer, READ_BUFSIZE);
    // DEBUG Serial.println(packetbuffer);
    handlePacket(packetreceivebuffer, replyidx);
    xSemaphoreGive( packetSem );
  }
}

void setDelay() {
  section->get_animation()->set_timer(animDelay);
}

void handlePacket(char *packetbuffer, uint8_t packetlength) {
  uint8_t packetPos = 0;
  if (packetbuffer[0] == '!' && packetbuffer[1] == 'R' && !trackRpm) return;
  if (packetbuffer[0] == '!' && packetlength > 1) {
    // Buttons
    char command = packetbuffer[1];
    packetPos = 1;
    uint8_t argumentLength = 0;
    char *arguments;
    if (packetlength > 2) {
      arguments = &packetbuffer[2];
      packetPos = 2;
      for (argumentLength = 1; (argumentLength + packetPos) < packetlength; argumentLength++) {
        if (packetbuffer[packetPos + argumentLength - 1] == '!'  ) {
          argumentLength--;
          break;
        }
      }
      packetPos = packetPos + argumentLength;
    }
    switch (command) {
      case '!':
        if (packetbuffer[2] == 'D' && packetbuffer[3] == 'F' && packetbuffer[4] == 'U') {
          Serial.println("PREPARING FOR OTA");
          bleuart.write("PREPARING FOR OTA");
          disableAnimator();
          delay(10);
          fade_in(0, (currentBright * 255), 4);
          delay(10);
          fade_out(0, (currentBright * 255 - 1), 4);
          enterOTADfu();
          break;
        }
      case 'A':
        Serial.println("animation");
        bleuart.write("animation changed");
        if (argumentLength > 0) {
          callCharWithMutex(&setAnimation, arguments, argumentLength);
        } else {
          callWithMutex(&cycleAnimation);
        }
        Serial.println(dbgHeapTotal() - dbgHeapUsed());
        break;
      case 'B':
        Serial.print(F("Brightness "));
        if (arguments[0] == 'U') {
          Serial.println("up");
          brightnessUp();
        } else {
          Serial.println("down");
          brightnessDown();
        }
        break;
      case 'C':  {
          uint8_t red = arguments[0];
          uint8_t green = arguments[1];
          uint8_t blue = arguments[2];
          disableAnimator();
          break;
        }
      case 'D': {

          Serial.println("delay");
          bleuart.write("delay changed");
          animDelay = atoi(arguments);
          callWithMutex(&setDelay);
          break;
        }
      case 'I':
        Serial.println("invert strip");
        bleuart.write("strip inverted");
        invertStrips = !invertStrips;
        break;
      case 'K':
        handleKitt(arguments, argumentLength);
        break;
      case 'L':
        if (argumentLength > 0) {
          Serial.println("toggle slide delay");
          bleuart.write("logo scrolling delay set");
          if (!logoSlide) logoSlide = true;
          logoDelay = atoi(arguments);
          //logoPos = NUM_COLUMNS + 2;
        } else {
          Serial.println("toggle slide");
          bleuart.write("logo scrolling toggled");
          logoSlide = !logoSlide;
          //if (logoSlide) logoPos = NUM_COLUMNS + 2;
        }
        break;
      case 'M':
        Serial.println(F("Toggling Maestro"));
        bleuart.write("Toggling display");
        toggleMaestro();
        break;
      case 'O':
        Serial.println("orientation");
        bleuart.write("orientation changed");
        callWithMutex(&cycleOrientation);
        break;
      case 'P':
        Serial.println("palette");
        bleuart.write("palette changed");
        if (argumentLength > 0) {
          callCharWithMutex(&setPalette, arguments, argumentLength);
        } else {
          callWithMutex(&cyclePalette);
        }
        break;
      case 'R':
        if (trackRpm) {
          memcpy(rpmChars, arguments, min(4, argumentLength));
          numRpmChars = min(4, argumentLength);
          if (numRpmChars < 4) rpmChars[3] = 0;
        }
        break;
      case 'S':
        Serial.print(F("Toggle custom string "));
        if (argumentLength > 0) {
          Serial.print("on");
          memcpy(custom_chars, arguments, min(18, argumentLength));
          num_custom_chars = min(18, argumentLength);
          bleuart.write(" string set");
          //logoPos = NUM_COLUMNS + 2;
          toggleLogo(true);
        } else {
          Serial.println("toggled");
          toggleLogo();
        }
        break;
      case 'V':
        Serial.println(F("Toggling rpm tracking"));
        bleuart.write("tracking rpm toggled");
        trackRpm = !trackRpm;
        if (trackRpm && animatorFunction == &noopAnimator) toggleLogo(true);
        break;
      case 'X': {
         disableAnimator();
         setBlack(true);
         break;
      }
      case 'Y': {
          logoOffset = atoi(arguments);
          Serial.println("logo offset");
          Serial.println(logoOffset);
          bleuart.write("logo offset changed");
          break;
        }
    }
    if (packetPos < packetlength) {
      Serial.println("recursing");
      char *secondSet = &packetbuffer[packetPos];
      handlePacket(secondSet, packetlength - packetPos);
    }

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