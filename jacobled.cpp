#include "jacobled.h"
#include <core/colors.h>
#include <Arduino.h>

#define NUM_CHUNKS 15
#define NUMV2_LEDS 300

static const uint8_t led_gamma_table[256] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                             0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2,
                                             2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5,
                                             6, 6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10, 10, 11, 11,
                                             11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18,
                                             19, 19, 20, 21, 21, 22, 22, 23, 23, 24, 25, 25, 26, 27, 27, 28,
                                             29, 29, 30, 31, 31, 32, 33, 34, 34, 35, 36, 37, 37, 38, 39, 40,
                                             40, 41, 42, 43, 44, 45, 46, 46, 47, 48, 49, 50, 51, 52, 53, 54,
                                             55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70,
                                             71, 72, 73, 74, 76, 77, 78, 79, 80, 81, 83, 84, 85, 86, 88, 89,
                                             90, 91, 93, 94, 95, 96, 98, 99, 100, 102, 103, 104, 106, 107, 109, 110,
                                             111, 113, 114, 116, 117, 119, 120, 121, 123, 124, 126, 128, 129, 131, 132, 134,
                                             135, 137, 138, 140, 142, 143, 145, 146, 148, 150, 151, 153, 155, 157, 158, 160,
                                             162, 163, 165, 167, 169, 170, 172, 174, 176, 178, 179, 181, 183, 185, 187, 189,
                                             191, 193, 194, 196, 198, 200, 202, 204, 206, 208, 210, 212, 214, 216, 218, 220,
                                             222, 224, 227, 229, 231, 233, 235, 237, 239, 241, 244, 246, 248, 250, 252, 255
                                            };

/* ALL YOUR BUFFER ARE BELONG TO US
   2 buffers per led strip, each 1/6 of the size of the LEDs. NUMV2_LEDS must be a multiple of 6.
   alternate between the two buffers, each on their own SEQ, using LOOP to automatically cycle between them.
   This is hard-coded for 3 strips on 3 different pins, each taking a separate PWM device */
uint16_t pwm_buffer[(NUMV2_LEDS / NUM_CHUNKS) * sizeof(Color8_t) * 8];
uint16_t pwm_buffer2[(NUMV2_LEDS / NUM_CHUNKS) * sizeof(Color8_t) * 8];
uint16_t pwm_buffer3[(NUMV2_LEDS / NUM_CHUNKS) * sizeof(Color8_t) * 8];
uint16_t pwm_buffer4[(NUMV2_LEDS / NUM_CHUNKS) * sizeof(Color8_t) * 8];
uint16_t pwm_buffer5[(NUMV2_LEDS / NUM_CHUNKS) * sizeof(Color8_t) * 8];
uint16_t pwm_buffer6[(NUMV2_LEDS / NUM_CHUNKS) * sizeof(Color8_t) * 8];


void led_set_colors(PixelMaestro::Colors::RGB *colors, uint8_t pin, PixelMaestro::Colors::RGB *colors2, uint8_t pin2, PixelMaestro::Colors::RGB *colors3, uint8_t pin3)
{

  NRF_PWM_Type* PWM[3] = {NRF_PWM0, NRF_PWM1, NRF_PWM2};

  uint16_t led_index = 0;
  
  for (int j = 0; j < 3; j++) {
    PWM[j]->COUNTERTOP            = (CTOPVAL << PWM_COUNTERTOP_COUNTERTOP_Pos);
    PWM[j]->DECODER                 = (PWM_DECODER_LOAD_Common << PWM_DECODER_LOAD_Pos) |
                                      (PWM_DECODER_MODE_RefreshCount << PWM_DECODER_MODE_Pos);
    PWM[j]->LOOP                        = ((NUM_CHUNKS / 2) << PWM_LOOP_CNT_Pos);
    PWM[j]->MODE                        = (PWM_MODE_UPDOWN_Up << PWM_MODE_UPDOWN_Pos);
    PWM[j]->PRESCALER             = (PWM_PRESCALER_PRESCALER_DIV_1 << PWM_PRESCALER_PRESCALER_Pos);
    PWM[j]->SEQ[0].ENDDELAY = 0;
    PWM[j]->SEQ[0].REFRESH    = 0;
    PWM[j]->SEQ[1].ENDDELAY = 0;
    PWM[j]->SEQ[1].REFRESH    = 0;
    PWM[j]->ENABLE = 1;
  }
  PWM[0]->PSEL.OUT[0]         = pin;
  PWM[1]->PSEL.OUT[0]         = pin2;
  PWM[2]->PSEL.OUT[0]         = pin3;

  PWM[0]->SEQ[0].PTR            = (uint32_t)(pwm_buffer) << PWM_SEQ_PTR_PTR_Pos;
  PWM[0]->SEQ[0].CNT            = (sizeof(pwm_buffer) / 2) << PWM_SEQ_CNT_CNT_Pos;
  PWM[0]->SEQ[1].PTR            = (uint32_t)(pwm_buffer2) << PWM_SEQ_PTR_PTR_Pos;
  PWM[0]->SEQ[1].CNT            = (sizeof(pwm_buffer2) / 2) << PWM_SEQ_CNT_CNT_Pos;

  PWM[1]->SEQ[0].PTR            = (uint32_t)(pwm_buffer3) << PWM_SEQ_PTR_PTR_Pos;
  PWM[1]->SEQ[0].CNT            = (sizeof(pwm_buffer3) / 2) << PWM_SEQ_CNT_CNT_Pos;
  PWM[1]->SEQ[1].PTR            = (uint32_t)(pwm_buffer4) << PWM_SEQ_PTR_PTR_Pos;
  PWM[1]->SEQ[1].CNT            = (sizeof(pwm_buffer4) / 2) << PWM_SEQ_CNT_CNT_Pos;

  PWM[1]->SEQ[0].PTR            = (uint32_t)(pwm_buffer5) << PWM_SEQ_PTR_PTR_Pos;
  PWM[1]->SEQ[0].CNT            = (sizeof(pwm_buffer5) / 2) << PWM_SEQ_CNT_CNT_Pos;
  PWM[1]->SEQ[1].PTR            = (uint32_t)(pwm_buffer6) << PWM_SEQ_PTR_PTR_Pos;
  PWM[1]->SEQ[1].CNT            = (sizeof(pwm_buffer6) / 2) << PWM_SEQ_CNT_CNT_Pos;

  for (int i = 0; i < NUM_CHUNKS; i++) {
    uint16_t byte_num;
    uint16_t bit_index = 0;
    uint16_t bit_index2 = 0;
    uint16_t bit_index3 = 0;


    // on even-numbered loops, wait for the first SEQ to end before filling the buffer
    // The odd loops just fill their buffer, all waits occur before/after filling the buffer on even oops
    if (i > 0 && (i % 2) == 0) {
      while (!PWM[0]->EVENTS_SEQEND[0]) {
        yield();
      }
      PWM[0]->EVENTS_SEQEND[0]    = 0;
    }
    // Fill all even or odd buffers at once
    for (led_index = (NUMV2_LEDS / NUM_CHUNKS * i); led_index < (NUMV2_LEDS / NUM_CHUNKS + (NUMV2_LEDS / NUM_CHUNKS * i)); led_index++) {

      uint8_t bit_mask;
      uint8_t val;
      uint8_t val2;
      uint8_t val3;
      //        Serial.println("Filling buffer ");
      // for each byte, expand each bit to a PWM duty cycle
      for (byte_num = 0; byte_num < sizeof(Color8_t); byte_num++) {
        val = led_gamma_table[colors[led_index][byte_num]];
        val2 = led_gamma_table[colors2[led_index][byte_num]];
        val3 = led_gamma_table[colors3[led_index][byte_num]];
        for (bit_mask = 0x80; bit_mask > 0x00; bit_mask >>= 1)
        {
          if (i % 2 == 0) {
            pwm_buffer[bit_index++] = (val & bit_mask) ? T1H_DUTY : T0H_DUTY;
            pwm_buffer3[bit_index2++] = (val2 & bit_mask) ? T1H_DUTY : T0H_DUTY;
            pwm_buffer5[bit_index3++] = (val3 & bit_mask) ? T1H_DUTY : T0H_DUTY;
          } else {
            pwm_buffer2[bit_index++] = (val & bit_mask) ? T1H_DUTY : T0H_DUTY;
            pwm_buffer4[bit_index2++] = (val2 & bit_mask) ? T1H_DUTY : T0H_DUTY;
            pwm_buffer6[bit_index3++] = (val3 & bit_mask) ? T1H_DUTY : T0H_DUTY;
          }
        }
      }
      //                 Serial.println("Buffer full");
    }

    // on even numbered loops, wait for the second SEQ to end before cycling
    if (i > 1 && (i % 2) == 0) {
      while (!PWM[0]->EVENTS_SEQEND[1]) {
        yield();
      }
      PWM[0]->EVENTS_SEQEND[1]    = 0;

    }

    // PWM[pwm_num] module config
    // top value as defined in header
    // loop disabled
    // mode = up/down
    // prescaler = 1

    // fire off the initial start task
    // due to LOOP == 3, the PWM will automatically cycle between SEQ[0] and SEQ[1] 3 times
    if (i == 1) {
      for (uint8_t j = 0; j < 3; j++) {
        PWM[j]->EVENTS_LOOPSDONE = 0;
        PWM[j]->EVENTS_SEQEND[0]    = 0;
        PWM[j]->EVENTS_SEQEND[1]    = 0;

        PWM[j]->TASKS_SEQSTART[0] = 1;
      }
    }
  }
  // Wait for the last round of SEQ[1] to complete
  while (!PWM[0]->EVENTS_LOOPSDONE) {
    yield();
  }



  // disable PWM modules, disconnect the pins
  PWM[0]->ENABLE = 0;
  PWM[0]->PSEL.OUT[0] = 0xFFFFFFFFUL;
  PWM[1]->ENABLE = 0;
  PWM[1]->PSEL.OUT[0] = 0xFFFFFFFFUL;
  PWM[2]->ENABLE = 0;
  PWM[2]->PSEL.OUT[0] = 0xFFFFFFFFUL;
}

