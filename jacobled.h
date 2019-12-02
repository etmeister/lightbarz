#ifndef SRC_LED_LEDV2_H_
#define SRC_LED_LEDV2_H_

// #define JACOBLED_SKIP_GAMMA

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "nrf_gpio.h"
#include "nrf52_bitfields.h"
#include <core/colors.h>

// approx 400nS for T0H
// approx 800nS for T1H
// approx 1250nS period
#define T0H_DUTY  0x8006
#define T1H_DUTY  0x800D
#define CTOPVAL    0x0014

#define NUMV2_LEDS 300

typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} Color8_t;


void     led_set_colors(PixelMaestro::Colors::RGB *colors, uint8_t pin);

extern "C" void yield();
#endif /* SRC_LED_LEDV2_H_ */

