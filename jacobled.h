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
#define T1H_DUTY  0x800E
#define CTOPVAL    0x0014


typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} Color8_t;


void led_set_colors(PixelMaestro::Colors::RGB *colors, uint8_t pin, PixelMaestro::Colors::RGB *colors2, uint8_t pin2, PixelMaestro::Colors::RGB *colors3, uint8_t pin3);
extern "C" void yield();
#endif /* SRC_LED_LEDV2_H_ */

