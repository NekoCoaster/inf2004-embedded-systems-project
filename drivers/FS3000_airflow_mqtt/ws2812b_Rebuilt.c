#include <stdio.h>
#include "pico/stdlib.h"
#include "ws2812b_Rebuilt.h"
#include "string.h"

// This driver will serve as a drop in replacement for driving both the onboard WS2812B LED on the MakerPico
// and as well as any external WS2812B LEDs

#define MKPICO_LED_PIN 28
#define MKPICO_LED_COUNT 1

#ifndef EXTERN_LED_PIN
#define EXTERN_LED_PIN 27
#endif

#ifndef EXTERN_LED_COUNT
#define EXTERN_LED_COUNT 8
#endif

#define LED_DATA_SIZE 3
#define LED_BYTE_SIZE EXTERN_LED_COUNT *LED_DATA_SIZE

static rgb_t extern_led_data[EXTERN_LED_COUNT];
static rgb_t mkpico_led_data = {0, 0, 0};

static bool _mkpico_led_enabled = false;
static bool _extern_led_enabled = false;

#pragma region common functions
void hexToRGB(const char *hexColor, int *red, int *green, int *blue)
{
    if (hexColor == NULL || strlen(hexColor) != 6)
    {
        // Invalid input, handle error accordingly
        printf("Invalid hex color string%s\n", hexColor);
        return;
    }

    // Convert hex to decimal using strtol
    sscanf(hexColor, "%02x%02x%02x", red, green, blue);
}

rgb_t hexToRGBStruct(const char *hexColor)
{
    int red, green, blue;
    hexToRGB(hexColor, &red, &green, &blue);
    rgb_t color = {red, green, blue};
    return color;
}

#pragma endregion

#pragma region library Initialisation code

void ws2812b_init(bool enable_makerpico_led, bool enable_external_led)
{
    if (enable_makerpico_led)
    {
        _mkpico_led_enabled = true;
        gpio_init(MKPICO_LED_PIN);
        gpio_set_dir(MKPICO_LED_PIN, GPIO_OUT);
        gpio_put(MKPICO_LED_PIN, false);
    }
    if (enable_external_led)
    {
        _extern_led_enabled = true;
        gpio_init(EXTERN_LED_PIN);
        gpio_set_dir(EXTERN_LED_PIN, GPIO_OUT);
        gpio_put(EXTERN_LED_PIN, false);
    }
    // Wait a little while for the LEDs to reset
    sleep_ms(10);
}

void ws2812b_init_all()
{
    ws2812b_init(true, true);
}
void ws2812b_init_external()
{
    ws2812b_init(false, true);
}
void ws2812b_init_makerpico()
{
    ws2812b_init(true, false);
}

#pragma endregion

#pragma region makerpico led specific functions

void set_makerpico_led_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    mkpico_led_data.r = r;
    mkpico_led_data.g = g;
    mkpico_led_data.b = b;
}
void set_makerpico_led(rgb_t color)
{
    set_makerpico_led_rgb(color.r, color.g, color.b);
}

void set_makerpico_led_hex(const char *hexString)
{
    int red, green, blue;
    hexToRGB(hexString, &red, &green, &blue);
    set_makerpico_led_rgb(red, green, blue);
}

/* Sends the data to the LEDs */
void show_makerpico_led()
{
    /* Disable all interrupts and save the mask */
    uint32_t interrupt_mask = disable_and_save_interrupts();

    /* Get the pin bit */
    uint32_t pin = 1UL << MKPICO_LED_PIN;

    /* Declared outside to force optimization if compiler gets any funny ideas */
    uint8_t red = mkpico_led_data.r;
    uint8_t green = mkpico_led_data.g;
    uint8_t blue = mkpico_led_data.b;
    int8_t j = 0;

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

    /* Set the level low to indicate a reset is happening */
    sio_hw->gpio_clr = pin;

    /* Enable the interrupts that got disabled */
    enable_and_restore_interrupts(interrupt_mask);

    /* Make sure to wait any amount of time after you call this function */
}
#pragma endregion

#pragma region external led specific functions
void set_external_led_rgb(uint8_t index, uint8_t r, uint8_t g, uint8_t b)
{
    if (index < EXTERN_LED_COUNT)
    {
        extern_led_data[index].r = r;
        extern_led_data[index].g = g;
        extern_led_data[index].b = b;
    }
}

void set_external_led(uint8_t index, rgb_t color)
{
    set_external_led_rgb(index, color.r, color.g, color.b);
}

void set_external_led_hex(uint8_t index, const char *hexString)
{
    int red, green, blue;
    hexToRGB(hexString, &red, &green, &blue);
    set_external_led_rgb(index, red, green, blue);
}

void set_all_external_leds_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = 0; i < EXTERN_LED_COUNT; i++)
    {
        set_external_led_rgb(i, r, g, b);
    }
}

void set_all_external_leds(rgb_t color)
{
    set_all_external_leds_rgb(color.r, color.g, color.b);
}

void set_all_external_leds_hex(const char *hexString)
{
    int red, green, blue;
    hexToRGB(hexString, &red, &green, &blue);
    set_all_external_leds_rgb(red, green, blue);
}

void show_external_led()
{
    /* Disable all interrupts and save the mask */
    uint32_t interrupt_mask = disable_and_save_interrupts();

    /* Get the pin bit */
    uint32_t pin = 1UL << EXTERN_LED_PIN;

    /* Declared outside to force optimization if compiler gets any funny ideas */
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    uint32_t i = 0;
    int8_t j = 0;

    for (i = 0; i < EXTERN_LED_COUNT; i++)
    {
        /* Send order is green, red, blue  */

        green = extern_led_data[i].g;
        red = extern_led_data[i].r;
        blue = extern_led_data[i].b;

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

    /* Make sure to wait any amount of time after you call this function */
}

#pragma endregion
