
#ifndef WS2812B_REBUILT_H
#define WS2812B_REBUILT_H

/* Libraries */

#include "pico/stdlib.h"

/* For WS2812B */
/* Datasheet used: https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf */

/* Assembler functions */
extern void cycle_delay_t0h();
extern void cycle_delay_t0l();
extern void cycle_delay_t1h();
extern void cycle_delay_t1l();
extern uint32_t disable_and_save_interrupts();       /* Used for interrupt disabling */
extern void enable_and_restore_interrupts(uint32_t); /* Used for interrupt enabling */

typedef enum
{
    WS2812B_LED_TYPE_EXTERNAL,
    WS2812B_LED_TYPE_MAKERPICO
} ws2812b_led_type_t;

void hexToRGB(const char *hexColor, uint8_t *red, uint8_t *green, uint8_t *blue);
/*Onboard LED functions*/
void set_onboard_led_rgb(uint8_t r, uint8_t g, uint8_t b);
void set_onboard_led_hex(const char *hexColor);
void show_onboard_led();

/*External LED functions*/
void set_external_led_rgb(uint32_t ledIndex, uint8_t r, uint8_t g, uint8_t b);
void set_external_led_hex(uint32_t ledIndex, const char *hexColor);
void set_all_external_leds_rgb(uint8_t r, uint8_t g, uint8_t b);
void set_all_external_leds_hex(const char *hexColor);
void show_external_leds();
/*
//Quick set Functions
void set_and_show_onboard_led_rgb(uint8_t r, uint8_t g, uint8_t b);
void set_and_show_onboard_led_hex(const char *hexColor);
void set_and_show_external_led_rgb(uint32_t ledIndex, uint8_t r, uint8_t g, uint8_t b);
void set_and_show_external_led_hex(uint32_t ledIndex, const char *hexColor);
void set_and_show_all_external_leds_rgb(uint8_t r, uint8_t g, uint8_t b);
void set_and_show_all_external_leds_hex(const char *hexColor);

// everything functions
void set_and_show_everything_rgb(uint8_t r, uint8_t g, uint8_t b);
void set_and_show_everything_hex(const char *hexColor);
void reset_all_leds();
*/
/*Init functions*/
void ws2812b_init(bool enable_onboard_led, bool enable_external_led);
void ws2812b_init_all();
void ws2812b_init_external_led();
void ws2812b_init_onboard_led();

#endif