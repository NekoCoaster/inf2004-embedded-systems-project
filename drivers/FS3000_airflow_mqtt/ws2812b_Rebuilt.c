#include <stdio.h>
#include "pico/stdlib.h"
#include "ws2812b_Rebuilt.h"
#include "string.h"

// This driver will serve as a drop in replacement for driving both the onboard WS2812B LED on the MakerPico
// and as well as any external WS2812B LEDs

#define ONBOARD_LED_PIN 28
#define ONBOARD_LED_COUNT 1

#ifndef EXTERNAL_LED_PIN
#define EXTERNAL_LED_PIN 27
#endif

#ifndef EXTERNAL_LED_COUNT
#define EXTERNAL_LED_COUNT 8
#endif

#define WS2812B_USE_100_SCALE // Comment this out if you want to use the 0-255 scale for RGB colours
#define COOLDOWN_DELAY 10
#define LED_DATA_SIZE 3

#define ONBOARD_LED_TOTAL_DATA_SIZE ONBOARD_LED_COUNT *LED_DATA_SIZE
#define EXTERNAL_LED_TOTAL_DATA_SIZE EXTERNAL_LED_COUNT *LED_DATA_SIZE

uint8_t onboard_led_data[ONBOARD_LED_TOTAL_DATA_SIZE];
uint8_t external_led_data[EXTERNAL_LED_TOTAL_DATA_SIZE];

#pragma region common functions
static uint8_t mapTo100(int inputValue)
{
    // Ensure the input value is within the valid range (0-255)
    if (inputValue <= 0)
    {
        return 0;
    }
    else if (inputValue >= 255)
    {
        return 100;
    }

    // Map the input value to the range 0-100
    return (uint8_t)(((double)inputValue / 255) * 100);
}

void hexToRGB(const char *hexColor, uint8_t *red, uint8_t *green, uint8_t *blue)
{
    if (hexColor == NULL || strlen(hexColor) != 6)
    {
        // Invalid input, handle error accordingly
        printf("Invalid hex color string%s\n", hexColor);
        return;
    }
    // printf("all params: %s, %d, %d, %d\n", hexColor, *red, *green, *blue);
    //  Convert hex to decimal using strtol
    //  For signed ints
    //  sscanf(hexColor, "%02x%02x%02x", red, green, blue);
    //  For unsigned ints
    sscanf(hexColor, "%02hhx%02hhx%02hhx", red, green, blue);
    // printf("Hex: %s, RGB: %d, %d, %d\n", hexColor, *red, *green, *blue);
#ifdef WS2812B_USE_100_SCALE
    *red = mapTo100(*red);
    *green = mapTo100(*green);
    *blue = mapTo100(*blue);
#endif
}

#pragma endregion
#pragma region onboard led functions

void set_onboard_led_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    onboard_led_data[0] = g; /* Green */
    onboard_led_data[1] = r; /* Red */
    onboard_led_data[2] = b; /* Blue */
}
void set_onboard_led_hex(const char *hexColor)
{
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    hexToRGB(hexColor, &red, &green, &blue);
    set_onboard_led_rgb(red, green, blue);
}

void show_onboard_led()
{
    /* Disable all interrupts and save the mask */
    uint32_t interrupt_mask = disable_and_save_interrupts();

    /* Get the pin bit */
    uint32_t pin = 1UL << ONBOARD_LED_PIN;

    /* Declared outside to force optimization if compiler gets any funny ideas */
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    uint32_t i = 0;
    int8_t j = 0;

    for (i = 0; i < ONBOARD_LED_TOTAL_DATA_SIZE; i += LED_DATA_SIZE)
    {
        /* Send order is green, red, blue  */
        green = onboard_led_data[i];
        red = onboard_led_data[i + 1];
        blue = onboard_led_data[i + 2];

        for (j = 7; j >= 0; j--) /* Handle the 8 green bits */
        {
            /* Get Nth bit */
            if (((green >> j) & 1) == 1) /* The bit is 1 */
            {
                sio_hw->gpio_set = pin; /* This sets the specific pin to high */
                cycle_delay_t1h();      /* Delay by datasheet amount (800ns give or take) */
                sio_hw->gpio_clr = pin; /* This sets the specific pin to low */
                cycle_delay_t1l();      /* Delay by datasheet amount (450ns give or take) */
            }
            else /* The bit is 0 */
            {
                sio_hw->gpio_set = pin;
                cycle_delay_t0h();
                sio_hw->gpio_clr = pin;
                cycle_delay_t0l();
            }
        }

        for (j = 7; j >= 0; j--) /* Handle the 8 red bits */
        {
            if (((red >> j) & 1) == 1)
            {
                sio_hw->gpio_set = pin;
                cycle_delay_t1h();
                sio_hw->gpio_clr = pin;
                cycle_delay_t1l();
            }
            else
            {
                sio_hw->gpio_set = pin;
                cycle_delay_t0h();
                sio_hw->gpio_clr = pin;
                cycle_delay_t0l();
            }
        }

        for (j = 7; j >= 0; j--) /* Handle the 8 blue bits */
        {
            if (((blue >> j) & 1) == 1)
            {
                sio_hw->gpio_set = pin;
                cycle_delay_t1h();
                sio_hw->gpio_clr = pin;
                cycle_delay_t1l();
            }
            else
            {
                sio_hw->gpio_set = pin;
                cycle_delay_t0h();
                sio_hw->gpio_clr = pin;
                cycle_delay_t0l();
            }
        }
    }
    /* Set the level low to indicate a reset is happening */
    sio_hw->gpio_clr = pin;

    /* Enable the interrupts that got disabled */
    enable_and_restore_interrupts(interrupt_mask);

    /*Waits for things to settle down*/
    sleep_ms(COOLDOWN_DELAY);
}

#pragma endregion

#pragma region external led functions

/* Sets a specific LED to a certain color */
/* LEDs start at 0 */
void set_external_led_rgb(uint32_t ledIndex, uint8_t r, uint8_t g, uint8_t b)
{
    if (ledIndex >= EXTERNAL_LED_COUNT)
    {
        printf("Invalid LED index: %u\n", ledIndex);
        return;
    }

    uint32_t ledDataIndex = ledIndex * LED_DATA_SIZE;
    external_led_data[ledDataIndex] = g;     /* Green */
    external_led_data[ledDataIndex + 1] = r; /* Red */
    external_led_data[ledDataIndex + 2] = b; /* Blue */
}

void set_external_led_hex(uint32_t ledIndex, const char *hexColor)
{
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    hexToRGB(hexColor, &red, &green, &blue);
    set_external_led_rgb(ledIndex, red, green, blue);
}
/* Sets all the LEDs to a certain color */
void set_all_external_leds_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    for (uint32_t i = 0; i < EXTERNAL_LED_TOTAL_DATA_SIZE; i += LED_DATA_SIZE)
    {
        external_led_data[i] = g;     /* Green */
        external_led_data[i + 1] = r; /* Red */
        external_led_data[i + 2] = b; /* Blue */
    }
}

void set_all_external_leds_hex(const char *hexColor)
{
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    hexToRGB(hexColor, &red, &green, &blue);
    set_all_external_leds_rgb(red, green, blue);
}

void show_external_leds()
{
    /* Disable all interrupts and save the mask */
    uint32_t interrupt_mask = disable_and_save_interrupts();

    /* Get the pin bit */
    uint32_t pin = 1UL << EXTERNAL_LED_PIN;

    /* Declared outside to force optimization if compiler gets any funny ideas */
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    uint32_t i = 0;
    int8_t j = 0;
    for (i = 0; i < EXTERNAL_LED_TOTAL_DATA_SIZE; i += LED_DATA_SIZE)
    {
        /* Send order is green, red, blue  */
        green = external_led_data[i];
        red = external_led_data[i + 1];
        blue = external_led_data[i + 2];

        for (j = 7; j >= 0; j--) /* Handle the 8 green bits */
        {
            /* Get Nth bit */
            if (((green >> j) & 1) == 1) /* The bit is 1 */
            {
                sio_hw->gpio_set = pin; /* This sets the specific pin to high */
                cycle_delay_t1h();      /* Delay by datasheet amount (800ns give or take) */
                sio_hw->gpio_clr = pin; /* This sets the specific pin to low */
                cycle_delay_t1l();      /* Delay by datasheet amount (450ns give or take) */
            }
            else /* The bit is 0 */
            {
                sio_hw->gpio_set = pin;
                cycle_delay_t0h();
                sio_hw->gpio_clr = pin;
                cycle_delay_t0l();
            }
        }

        for (j = 7; j >= 0; j--) /* Handle the 8 red bits */
        {
            if (((red >> j) & 1) == 1)
            {
                sio_hw->gpio_set = pin;
                cycle_delay_t1h();
                sio_hw->gpio_clr = pin;
                cycle_delay_t1l();
            }
            else
            {
                sio_hw->gpio_set = pin;
                cycle_delay_t0h();
                sio_hw->gpio_clr = pin;
                cycle_delay_t0l();
            }
        }

        for (j = 7; j >= 0; j--) /* Handle the 8 blue bits */
        {
            if (((blue >> j) & 1) == 1)
            {
                sio_hw->gpio_set = pin;
                cycle_delay_t1h();
                sio_hw->gpio_clr = pin;
                cycle_delay_t1l();
            }
            else
            {
                sio_hw->gpio_set = pin;
                cycle_delay_t0h();
                sio_hw->gpio_clr = pin;
                cycle_delay_t0l();
            }
        }
    }
    /* Set the level low to indicate a reset is happening */
    sio_hw->gpio_clr = pin;

    /* Enable the interrupts that got disabled */
    enable_and_restore_interrupts(interrupt_mask);

    /*Waits for things to settle down*/
    sleep_ms(COOLDOWN_DELAY);
}

#pragma endregion
/*
#pragma region quick set functions
//for onboard LED
void set_and_show_onboard_led_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    set_onboard_led_rgb(r, g, b);
    show_onboard_led();
}
void set_and_show_onboard_led_hex(const char *hexColor)
{
    set_onboard_led_hex(hexColor);
    show_onboard_led();
}
//for 1 specific external LED
void set_and_show_external_led_rgb(uint32_t ledIndex, uint8_t r, uint8_t g, uint8_t b)
{
    set_external_led_rgb(ledIndex, r, g, b);
    show_external_leds();
}
void set_and_show_external_led_hex(uint32_t ledIndex, const char *hexColor)
{
    set_external_led_hex(ledIndex, hexColor);
    show_external_leds();
}

//for all external LEDs
void set_and_show_all_external_leds_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    set_all_external_leds_rgb(r, g, b);
    show_external_leds();
}
void set_and_show_all_external_leds_hex(const char *hexColor)
{
    set_all_external_leds_hex(hexColor);
    show_external_leds();
}

#pragma endregion

#pragma region reset functions
void set_and_show_everything_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    set_and_show_onboard_led_rgb(r, g, b);
    set_and_show_all_external_leds_rgb(r, g, b);
}
void set_and_show_everything_hex(const char *hexColor)
{
    set_and_show_onboard_led_hex(hexColor);
    set_and_show_all_external_leds_hex(hexColor);
}
// cycles through all the colors and then resets the LEDs
void reset_all_leds()
{
    set_and_show_everything_rgb(255, 0, 0);
    sleep_ms(100);
    set_and_show_everything_rgb(0, 255, 0);
    sleep_ms(100);
    set_and_show_everything_rgb(0, 0, 255);
    sleep_ms(100);
    set_and_show_everything_rgb(0, 0, 0);
    sleep_ms(100);
}

*/
#pragma region init functions
void ws2812b_init(bool enable_onboard_led, bool enable_external_led)
{
    if (enable_onboard_led)
    {
        gpio_init(ONBOARD_LED_PIN);
        gpio_set_dir(ONBOARD_LED_PIN, GPIO_OUT);
        gpio_put(ONBOARD_LED_PIN, false);
    }
    if (enable_external_led)
    {
        gpio_init(EXTERNAL_LED_PIN);
        gpio_set_dir(EXTERNAL_LED_PIN, GPIO_OUT);
        gpio_put(EXTERNAL_LED_PIN, false);
    }
    // Wait a little while for the LEDs to reset
    sleep_ms(100);
}

void ws2812b_init_all()
{
    ws2812b_init(true, true);
}
void ws2812b_init_external_led()
{
    ws2812b_init(false, true);
}
void ws2812b_init_onboard_led()
{
    ws2812b_init(true, false);
}

#pragma endregion