#ifndef NEOPIXEL_H
#define NEOPIXEL_H

#include <stdbool.h>
#include <stdint.h>
#include "nrf_delay.h"
#include "nrf_gpio.h"

// Timings calibres pour l'implementation de neopixel.c.
#define NEOPIXEL_SEND_ONE NRF_GPIO->OUTSET = (1UL << PIN); \
                __ASM ( \
                                " NOP\n\t" \
                                " NOP\n\t" \
                                " NOP\n\t" \
                                " NOP\n\t" \
                                " NOP\n\t" \
                                " NOP\n\t" \
                                " NOP\n\t" \
                                " NOP\n\t" \
                                " NOP\n\t" \
                        ); \
                NRF_GPIO->OUTCLR = (1UL << PIN);

#define NEOPIXEL_SEND_ZERO NRF_GPIO->OUTSET = (1UL << PIN); \
                __ASM (  \
                                " NOP\n\t"  \
                        );  \
                NRF_GPIO->OUTCLR = (1UL << PIN);  \
                __ASM ( \
                                " NOP\n\t" \
                                " NOP\n\t" \
                                " NOP\n\t" \
                                " NOP\n\t" \
                                " NOP\n\t" \
                                " NOP\n\t" \
                                " NOP\n\t" \
                                " NOP\n\t" \
                        );

extern void nrf_delay_us(uint32_t volatile number_of_us);

typedef union {
        struct {
                uint8_t g, r, b;
        } simple;
        uint8_t grb[3];
} color_t;

typedef struct {
        uint8_t pin_num;
        uint16_t num_leds;
        color_t *leds;
} neopixel_strip_t;

#ifdef __cplusplus
extern "C" {
#endif

void neopixel_init(neopixel_strip_t *strip, uint8_t pin_num, uint16_t num_leds);
void neopixel_clear(neopixel_strip_t *strip);
void neopixel_show(neopixel_strip_t *strip);
uint8_t neopixel_set_color(neopixel_strip_t *strip, uint16_t index, uint8_t red, uint8_t green, uint8_t blue);
uint8_t neopixel_set_color_and_show(neopixel_strip_t *strip, uint16_t index, uint8_t red, uint8_t green, uint8_t blue);
void neopixel_destroy(neopixel_strip_t *strip);

#ifdef __cplusplus
}
#endif

#endif
