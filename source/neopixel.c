#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "neopixel.h"

void neopixel_init(neopixel_strip_t *strip, uint8_t pin_num, uint16_t num_leds)
{
        strip->leds = (color_t *)malloc(sizeof(color_t) * num_leds);
        strip->pin_num = pin_num;
        strip->num_leds = num_leds;
        nrf_gpio_cfg_output(pin_num);
        NRF_GPIO->OUTCLR = (1UL << pin_num);

        for (int i = 0; i < num_leds; i++)
        {
                strip->leds[i].simple.g = 0;
                strip->leds[i].simple.r = 0;
                strip->leds[i].simple.b = 0;
        }
}

void neopixel_clear(neopixel_strip_t *strip)
{
        for (int i = 0; i < strip->num_leds; i++)
        {
                strip->leds[i].simple.g = 0;
                strip->leds[i].simple.r = 0;
                strip->leds[i].simple.b = 0;
        }

        neopixel_show(strip);
}

void neopixel_show(neopixel_strip_t *strip)
{
        const uint8_t PIN = strip->pin_num;
        NRF_GPIO->OUTCLR = (1UL << PIN);
        nrf_delay_us(50);

        for (int i = 0; i < strip->num_leds; i++)
        {
                for (int j = 0; j < 3; j++)
                {
                        if ((strip->leds[i].grb[j] & 128) > 0) { NEOPIXEL_SEND_ONE }
                        else { NEOPIXEL_SEND_ZERO }

                        if ((strip->leds[i].grb[j] & 64) > 0) { NEOPIXEL_SEND_ONE }
                        else { NEOPIXEL_SEND_ZERO }

                        if ((strip->leds[i].grb[j] & 32) > 0) { NEOPIXEL_SEND_ONE }
                        else { NEOPIXEL_SEND_ZERO }

                        if ((strip->leds[i].grb[j] & 16) > 0) { NEOPIXEL_SEND_ONE }
                        else { NEOPIXEL_SEND_ZERO }

                        if ((strip->leds[i].grb[j] & 8) > 0) { NEOPIXEL_SEND_ONE }
                        else { NEOPIXEL_SEND_ZERO }

                        if ((strip->leds[i].grb[j] & 4) > 0) { NEOPIXEL_SEND_ONE }
                        else { NEOPIXEL_SEND_ZERO }

                        if ((strip->leds[i].grb[j] & 2) > 0) { NEOPIXEL_SEND_ONE }
                        else { NEOPIXEL_SEND_ZERO }

                        if ((strip->leds[i].grb[j] & 1) > 0) { NEOPIXEL_SEND_ONE }
                        else { NEOPIXEL_SEND_ZERO }
                }
        }
}

uint8_t neopixel_set_color(neopixel_strip_t *strip, uint16_t index, uint8_t red, uint8_t green, uint8_t blue)
{
        if (index < strip->num_leds)
        {
                strip->leds[index].simple.r = red;
                strip->leds[index].simple.g = green;
                strip->leds[index].simple.b = blue;
        }
        else
                return 1;

        return 0;
}

uint8_t neopixel_set_color_and_show(neopixel_strip_t *strip, uint16_t index, uint8_t red, uint8_t green, uint8_t blue)
{
        if (index < strip->num_leds)
        {
                strip->leds[index].simple.r = red;
                strip->leds[index].simple.g = green;
                strip->leds[index].simple.b = blue;
                neopixel_show(strip);
        }
        else
                return 1;

        return 0;
}

void neopixel_destroy(neopixel_strip_t *strip)
{
        free(strip->leds);
        strip->num_leds = 0;
        strip->pin_num = 0;
}
