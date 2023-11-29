
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

typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb_t;

typedef enum
{
    WS2812B_LED_TYPE_EXTERNAL,
    WS2812B_LED_TYPE_MAKERPICO
} ws2812b_led_type_t;

/*Common Colour Utility Functions*/
void hexToRGB(const char *hexColor, int *red, int *green, int *blue);
rgb_t hexToRGBStruct(const char *hexColor);

/*Library Initialization Functions*/
void ws2812b_init(bool enable_makerpico_led, bool enable_external_led);
void ws2812b_init_all();
void ws2812b_init_external();
void ws2812b_init_makerpico();

/*Maker Pico (AKA Onboard WS2812B) specific functions*/
void set_makerpico_led_rgb(uint8_t r, uint8_t g, uint8_t b);
void set_makerpico_led(rgb_t color);
void set_makerpico_led_hex(const char *hexString);
void show_makerpico_led();

/*External WS2812B specific functions*/
void set_external_led_rgb(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
void set_external_led(uint8_t index, rgb_t color);
void set_external_led_hex(uint8_t index, const char *hexString);
// Shortcut for setting all external LEDs to the same color
void set_all_external_leds_rgb(uint8_t r, uint8_t g, uint8_t b);
void set_all_external_leds(rgb_t color);
void set_all_external_leds_hex(const char *hexString);
void show_external_led();

#endif