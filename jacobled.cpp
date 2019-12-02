#include "jacobled.h"
#include <core/colors.h>
#include <Arduino.h>

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


uint16_t pwm_buffer[(NUMV2_LEDS / 6) * sizeof(Color8_t) * 8];
uint16_t pwm_buffer2[(NUMV2_LEDS / 6) * sizeof(Color8_t) * 8];
uint16_t buffsize = (sizeof(pwm_buffer) / 2) << PWM_SEQ_CNT_CNT_Pos;
uint16_t buffsizeend = (2) << PWM_SEQ_CNT_CNT_Pos;
uint32_t buffptr = (uint32_t)(pwm_buffer) << PWM_SEQ_PTR_PTR_Pos;
uint32_t buffptr2 = (uint32_t)(pwm_buffer2) << PWM_SEQ_PTR_PTR_Pos;
void led_set_colors(PixelMaestro::Colors::RGB *colors, uint8_t pin)
{
    // find a free pwm module
    NRF_PWM_Type* pwm = NULL;
    NRF_PWM_Type* PWM[3] = {NRF_PWM0, NRF_PWM1, NRF_PWM2};

    for (uint8_t module_num = 0; module_num < 3; module_num++)
    {
        // check if the module is not enabled and the psel's are not set
        if ( (PWM[module_num]->ENABLE == 0)                                                                &&
                 (PWM[module_num]->PSEL.OUT[0] & PWM_PSEL_OUT_CONNECT_Msk) &&
                 (PWM[module_num]->PSEL.OUT[1] & PWM_PSEL_OUT_CONNECT_Msk) &&
                 (PWM[module_num]->PSEL.OUT[2] & PWM_PSEL_OUT_CONNECT_Msk) &&
                 (PWM[module_num]->PSEL.OUT[3] & PWM_PSEL_OUT_CONNECT_Msk))
        {
            // this is the module to use
            pwm = PWM[module_num];
            break;
        }
    }


    // if a free pwm module was found
    if (pwm != NULL)
    {
        // for each byte, expand each bit to a PWM duty cycle
        uint16_t led_index = 0;
        for (int i = 0; i <= 6; i++) {
            uint16_t byte_num;
            uint16_t bit_index = 0;

            if (i < 6) {
                for (led_index = (50 * i); led_index < (NUMV2_LEDS / 6 + (50 * i)); led_index++) {

                    uint8_t bit_mask;
                    uint8_t val;
                    //        Serial.println("Filling buffer ");
                    for (byte_num = 0; byte_num < sizeof(Color8_t); byte_num++) {
                        val = led_gamma_table[colors[led_index][byte_num]];
                        for (bit_mask = 0x80; bit_mask > 0x00; bit_mask >>= 1)
                        {
                            if (i % 2 == 0) {
                                pwm_buffer[bit_index++] = (val & bit_mask) ? T1H_DUTY : T0H_DUTY;
                            } else {
                                pwm_buffer2[bit_index++] = (val & bit_mask) ? T1H_DUTY : T0H_DUTY;
                            }
                        }
                    }
                    //                 Serial.println("Buffer full");
                }

            } else {
                if (i % 2 == 0) {
                    pwm_buffer[bit_index++] = 0x0000;
                    pwm_buffer[bit_index++] = 0x0000;
                } else {
                    pwm_buffer2[bit_index++] = 0x0000;
                    pwm_buffer2[bit_index++] = 0x0000;
                }
            }


            // PWM module config
            // top value as defined in header
            // loop disabled
            // mode = up/down
            // prescaler = 1
            if (i > 0) {
                while (!pwm->EVENTS_LOOPSDONE) {
                  //Serial.print('.');
                    //yield();
                }
            } else {
            pwm->COUNTERTOP            = (CTOPVAL << PWM_COUNTERTOP_COUNTERTOP_Pos);
            pwm->DECODER                 = (PWM_DECODER_LOAD_Common << PWM_DECODER_LOAD_Pos) |
                                                         (PWM_DECODER_MODE_RefreshCount << PWM_DECODER_MODE_Pos);
            pwm->LOOP                        = (1 << PWM_LOOP_CNT_Pos);
            pwm->MODE                        = (PWM_MODE_UPDOWN_Up << PWM_MODE_UPDOWN_Pos);
            pwm->PRESCALER             = (PWM_PRESCALER_PRESCALER_DIV_1 << PWM_PRESCALER_PRESCALER_Pos);
            pwm->PSEL.OUT[0]         = pin;
            pwm->SEQ[0].ENDDELAY = 0;
            pwm->SEQ[0].REFRESH    = 0;
            pwm->SEQ[1].ENDDELAY = 0;
            pwm->SEQ[1].REFRESH    = 0;
                pwm->SEQ[0].PTR            = buffptr;
                    pwm->SEQ[0].CNT            = buffsize;
                pwm->SEQ[1].PTR            = buffptr2;
                    pwm->SEQ[1].CNT            = buffsize;
            pwm->ENABLE = 1;
            }
            if (i == 6) {
                pwm->SEQ[0].CNT == buffsizeend;              
            }
            // fire off the start task
            if ((i % 2) == 0) {
            pwm->EVENTS_LOOPSDONE = 0;
            pwm->EVENTS_SEQEND[(i % 2)]    = 0;
            pwm->TASKS_SEQSTART[(i % 2)] = 1;
            }
        }
        // blocking on sequence end?
        while (!pwm->EVENTS_SEQEND[0]) {
            yield();
        }

        // clear the end of sequence event flag
        pwm->EVENTS_SEQEND[0] = 0;

        // disable pwm module, disconnect the pin
        pwm->ENABLE = 0;
        pwm->PSEL.OUT[0] = 0xFFFFFFFFUL;
    }
}

