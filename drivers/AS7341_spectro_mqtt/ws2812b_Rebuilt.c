//  Original WS2812B LED code by yeahimthatguy @ https://forums.raspberrypi.com/viewtopic.php?t=322218
//  Adapted into standalone library to support both the MakerPico's onboard WS2812B LED and 1 additional external WS2812B LED strip.

// DISCLAIMER: The original code was written by yeahimthatguy as a quick and simple way to control WS2812B LEDs using the RP2040's PIO.
// I've simply adapted the code to be used as a standalone library to support both the MakerPico's onboard WS2812B LED and 1 additional external WS2812B LED strip.
//
//
#include <stdio.h>
#include "pico/stdlib.h"
#include "ws2812b_Rebuilt.h"
#include "string.h"

#define ONBOARD_LED_PIN 28 // onboard WS2812B LED pin
#define ONBOARD_LED_COUNT 1

#ifndef EXTERNAL_LED_PIN
#define EXTERNAL_LED_PIN 27 // external WS2812B LED pin (Change this to the pin you want to use)
#endif

#ifndef EXTERNAL_LED_COUNT
#define EXTERNAL_LED_COUNT 8 // Number of external LEDs (Change this to the number of LEDs you have)
#endif

#define WS2812B_USE_100_SCALE // Comment this out if you want to use the 0-255 scale for RGB colours
#define COOLDOWN_DELAY 10     // Delay in ms between each LED update to allow the LEDs to settle down.
#define LED_DATA_SIZE 3

#define ONBOARD_LED_TOTAL_DATA_SIZE ONBOARD_LED_COUNT *LED_DATA_SIZE
#define EXTERNAL_LED_TOTAL_DATA_SIZE EXTERNAL_LED_COUNT *LED_DATA_SIZE

uint8_t onboard_led_data[ONBOARD_LED_TOTAL_DATA_SIZE];   // Array to hold the data for the onboard LED (Not that an array is needed for 1 LED, but it makes development easier)
uint8_t external_led_data[EXTERNAL_LED_TOTAL_DATA_SIZE]; // Array to hold the data for the external LED

#pragma region common functions
/**
 * @brief Map an input value from the range 0-255 to 0-100.
 *
 * @param inputValue Input value to be mapped (0-255).
 * @return Mapped value in the range 0-100.
 *
 * @note The function ensures that the input value is within the valid range (0-255).
 *       If the input value is less than or equal to 0, the function returns 0.
 *       If the input value is greater than or equal to 255, the function returns 100.
 *       Otherwise, the function maps the input value to the range 0-100 using a linear scaling formula.
 *       Example usage: mappedValue = mapTo100(inputValue);
 */
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

/**
 * @brief Convert a hexadecimal color string to RGB values.
 *
 * @param hexColor Hexadecimal color string (e.g., "FF8800").
 * @param red Pointer to store the red color component (0-255 or 0-100 if using 100 scale).
 * @param green Pointer to store the green color component (0-255 or 0-100 if using 100 scale).
 * @param blue Pointer to store the blue color component (0-255 or 0-100 if using 100 scale).
 *
 * @note The function converts a hexadecimal color string to its corresponding RGB values.
 *       The hexColor parameter should be a string of length 6 representing RGB values in hexadecimal format.
 *       If the input is invalid or the string length is not 6, an error message is printed, and no conversion is performed.
 *       The RGB values are stored in the memory locations pointed to by red, green, and blue.
 *       If using a 100 scale (defined by WS2812B_USE_100_SCALE), the values are mapped to the range 0-100.
 *       Example usage: hexToRGB("FF8800", &redValue, &greenValue, &blueValue);
 */
void hexToRGB(const char *hexColor, uint8_t *red, uint8_t *green, uint8_t *blue)
{
    if (hexColor == NULL || strlen(hexColor) != 6)
    {
        // Invalid input, handle error accordingly
        printf("Invalid hex color string%s\n", hexColor);
        return;
    }
    //  Convert hex to decimal using strtol
    //  sscanf(hexColor, "%02x%02x%02x", red, green, blue); //Use this for signed ints
    sscanf(hexColor, "%02hhx%02hhx%02hhx", red, green, blue); // Use this For unsigned ints

    // Finally, map the values to 0-100 if using 100 scale is defined.
#ifdef WS2812B_USE_100_SCALE
    *red = mapTo100(*red);
    *green = mapTo100(*green);
    *blue = mapTo100(*blue);
#endif
}

#pragma endregion

#pragma region onboard led functions

/**
 * @brief Set the RGB values for the onboard LED.
 *
 * @param r Red color component (0-255).
 * @param g Green color component (0-255).
 * @param b Blue color component (0-255).
 *
 * @note The function sets the RGB values for the onboard LED by updating the corresponding data array.
 *       Each parameter represents the intensity of the respective color (0 for minimum intensity, 255 for maximum).
 *       Example usage: set_onboard_led_rgb(255, 0, 0) for red, set_onboard_led_rgb(0, 255, 0) for green, etc.
 */
void set_onboard_led_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    onboard_led_data[0] = g; /* Green */
    onboard_led_data[1] = r; /* Red */
    onboard_led_data[2] = b; /* Blue */
}

/**
 * @brief Set the onboard LED color using a hexadecimal color string.
 *
 * @param hexColor Hexadecimal color string (e.g., "FF8800").
 *
 * @note The function sets the color of the onboard LED by converting the hexadecimal color string
 *       to RGB values using the hexToRGB function and then passing those RGB values to the set_onboard_led_rgb function.
 *       The hexColor parameter should be a string of length 6 representing RGB values in hexadecimal format.
 *       Example usage: set_onboard_led_hex("FF8800") to set the onboard LED to an orange color.
 */
void set_onboard_led_hex(const char *hexColor)
{
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    hexToRGB(hexColor, &red, &green, &blue);
    set_onboard_led_rgb(red, green, blue);
}

/**
 * @brief Display RGB data on the onboard LED using a custom bit-banging method.
 *
 * @note The function uses a custom bit-banging method to send RGB data to the onboard LED.
 *       The RGB data is stored in the global array onboard_led_data.
 *       The function sequentially processes each set of RGB values (green, red, blue) in the array
 *       and sends the corresponding bits to the onboard LED pin.
 *       It handles each bit individually, setting and clearing the pin based on the bit value and applying
 *       specific delays as per the datasheet.
 *       After sending all the RGB data, it sets the level low to indicate a reset.
 *       Interrupts are temporarily disabled during the process to ensure precise timing.
 *       A cooldown delay is added after the process to allow things to settle down.
 */
void show_onboard_led()
{
    /* Disable all interrupts and save the mask */
    uint32_t interrupt_mask = disable_and_save_interrupts();

    /* Get the pin bit */
    uint32_t pin = 1UL << ONBOARD_LED_PIN;

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

    /* Restore the interrupts that got disabled */
    enable_and_restore_interrupts(interrupt_mask);

    /*Waits for the data sent out to settle down before resuming other operations*/
    sleep_ms(COOLDOWN_DELAY);
}

#pragma endregion

#pragma region external led functions
/**
 * @brief Set RGB values for an external LED at a specified index.
 *
 * @param ledIndex Index of the external LED (0 to EXTERNAL_LED_COUNT - 1).
 * @param r Red color component (0-255).
 * @param g Green color component (0-255).
 * @param b Blue color component (0-255).
 *
 * @note The function sets the RGB values for a specific external LED at the given index in the array.
 *       Each parameter represents the intensity of the respective color (0 for minimum intensity, 255 for maximum).
 *       Example usage: set_external_led_rgb(0, 255, 0, 0) to set the first external LED to red.
 */
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
/**
 * @brief Set the color of an external LED at a specified index using a hexadecimal color string.
 *
 * @param ledIndex Index of the external LED (0 to EXTERNAL_LED_COUNT - 1).
 * @param hexColor Hexadecimal color string (e.g., "FF8800").
 *
 * @note The function sets the color of a specific external LED at the given index using a hexadecimal color string.
 *       The hexColor parameter should be a string of length 6 representing RGB values in hexadecimal format.
 *       Example usage: set_external_led_hex(1, "00FF00") to set the second external LED to green.
 */
void set_external_led_hex(uint32_t ledIndex, const char *hexColor)
{
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    hexToRGB(hexColor, &red, &green, &blue);
    set_external_led_rgb(ledIndex, red, green, blue);
}
/**
 * @brief Set RGB values for all external LEDs to a specified color.
 *
 * @param r Red color component (0-255).
 * @param g Green color component (0-255).
 * @param b Blue color component (0-255).
 *
 * @note The function sets the RGB values for all external LEDs to the specified color.
 *       Each parameter represents the intensity of the respective color (0 for minimum intensity, 255 for maximum).
 *       Example usage: set_all_external_leds_rgb(255, 0, 0) to set all external LEDs to red.
 */
void set_all_external_leds_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    for (uint32_t i = 0; i < EXTERNAL_LED_TOTAL_DATA_SIZE; i += LED_DATA_SIZE)
    {
        external_led_data[i] = g;     /* Green */
        external_led_data[i + 1] = r; /* Red */
        external_led_data[i + 2] = b; /* Blue */
    }
}
/**
 * @brief Set the color of all external LEDs using a hexadecimal color string.
 *
 * @param hexColor Hexadecimal color string (e.g., "FF8800").
 *
 * @note The function sets the color of all external LEDs using a hexadecimal color string.
 *       The hexColor parameter should be a string of length 6 representing RGB values in hexadecimal format.
 *       Example usage: set_all_external_leds_hex("00FFFF") to set all external LEDs to cyan.
 */
void set_all_external_leds_hex(const char *hexColor)
{
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    hexToRGB(hexColor, &red, &green, &blue);
    set_all_external_leds_rgb(red, green, blue);
}
/**
 * @brief Display RGB data on external LEDs using a custom bit-banging method.
 *
 * @note The function uses a custom bit-banging method to send RGB data to external LEDs.
 *       The RGB data is stored in the global array external_led_data.
 *       The function sequentially processes each set of RGB values (green, red, blue) in the array
 *       and sends the corresponding bits to the external LED pin.
 *       It handles each bit individually, setting and clearing the pin based on the bit value and applying
 *       specific delays as per the datasheet.
 *       After sending all RGB data, it sets the level low to indicate a reset.
 *       Interrupts are temporarily disabled during the process to ensure precise timing.
 *       A cooldown delay is added after the process to allow things to settle down.
 */
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
        /* Send order is green, red, blue for the WS2812B. You may wish to change this if you have a different LED type */
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

    /* Restore the interrupts that got disabled */
    enable_and_restore_interrupts(interrupt_mask);

    /*Waits for the data sent out to settle down before resuming other operations*/
    sleep_ms(COOLDOWN_DELAY);
}

#pragma endregion
#pragma region quick set functions

/**
 * @brief Set RGB values for the onboard LED and display the result.
 *
 * @param r Red color component (0-255).
 * @param g Green color component (0-255).
 * @param b Blue color component (0-255).
 *
 * @note This function sets the RGB values for the onboard LED using the set_onboard_led_rgb function
 *       and then displays the result using the show_onboard_led function.
 *       Each parameter represents the intensity of the respective color (0 for minimum intensity, 255 for maximum).
 *       Example usage: set_and_show_onboard_led_rgb(255, 0, 0) to set and display the onboard LED in red.
 */
void set_and_show_onboard_led_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    set_onboard_led_rgb(r, g, b);
    show_onboard_led();
}

/**
 * @brief Set the color of the onboard LED using a hexadecimal color string and display the result.
 *
 * @param hexColor Hexadecimal color string (e.g., "FF8800").
 *
 * @note This function sets the color of the onboard LED using the set_onboard_led_hex function
 *       and then displays the result using the show_onboard_led function.
 *       The hexColor parameter should be a string of length 6 representing RGB values in hexadecimal format.
 *       Example usage: set_and_show_onboard_led_hex("00FFFF") to set and display the onboard LED in cyan.
 */
void set_and_show_onboard_led_hex(const char *hexColor)
{
    set_onboard_led_hex(hexColor);
    show_onboard_led();
}

/**
 * @brief Set RGB values for a specific external LED and display the result.
 *
 * @param ledIndex Index of the external LED (0 to EXTERNAL_LED_COUNT - 1).
 * @param r Red color component (0-255).
 * @param g Green color component (0-255).
 * @param b Blue color component (0-255).
 *
 * @note This function sets the RGB values for a specific external LED using the set_external_led_rgb function
 *       and then displays the result using the show_external_leds function.
 *       If the provided LED index is invalid, an error message is printed, and no update is performed.
 *       Each parameter represents the intensity of the respective color (0 for minimum intensity, 255 for maximum).
 *       Example usage: set_and_show_external_led_rgb(1, 255, 0, 0) to set and display the second external LED in red.
 */
void set_and_show_external_led_rgb(uint32_t ledIndex, uint8_t r, uint8_t g, uint8_t b)
{
    set_external_led_rgb(ledIndex, r, g, b);
    show_external_leds();
}

/**
 * @brief Set the color of a specific external LED using a hexadecimal color string and display the result.
 *
 * @param ledIndex Index of the external LED (0 to EXTERNAL_LED_COUNT - 1).
 * @param hexColor Hexadecimal color string (e.g., "FF8800").
 *
 * @note This function sets the color of a specific external LED using the set_external_led_hex function
 *       and then displays the result using the show_external_leds function.
 *       If the provided LED index is invalid, an error message is printed, and no update is performed.
 *       The hexColor parameter should be a string of length 6 representing RGB values in hexadecimal format.
 *       Example usage: set_and_show_external_led_hex(0, "00FFFF") to set and display the first external LED in cyan.
 */
void set_and_show_external_led_hex(uint32_t ledIndex, const char *hexColor)
{
    set_external_led_hex(ledIndex, hexColor);
    show_external_leds();
}

/**
 * @brief Set RGB values for all external LEDs and display the result.
 *
 * @param r Red color component (0-255).
 * @param g Green color component (0-255).
 * @param b Blue color component (0-255).
 *
 * @note This function sets the RGB values for all external LEDs using the set_all_external_leds_rgb function
 *       and then displays the result using the show_external_leds function.
 *       Each parameter represents the intensity of the respective color (0 for minimum intensity, 255 for maximum).
 *       Example usage: set_and_show_all_external_leds_rgb(0, 255, 0) to set and display all external LEDs in green.
 */
void set_and_show_all_external_leds_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    set_all_external_leds_rgb(r, g, b);
    show_external_leds();
}

/**
 * @brief Set the color of all external LEDs using a hexadecimal color string and display the result.
 *
 * @param hexColor Hexadecimal color string (e.g., "FF8800").
 *
 * @note This function sets the color of all external LEDs using the set_all_external_leds_hex function
 *       and then displays the result using the show_external_leds function.
 *       The hexColor parameter should be a string of length 6 representing RGB values in hexadecimal format.
 *       Example usage: set_and_show_all_external_leds_hex("0000FF") to set and display all external LEDs in blue.
 */
void set_and_show_all_external_leds_hex(const char *hexColor)
{
    set_all_external_leds_hex(hexColor);
    show_external_leds();
}

#pragma endregion

#pragma region reset functions
/**
 * @brief Set and display RGB values for both the onboard LED and all external LEDs.
 *
 * @param r Red color component (0-255).
 * @param g Green color component (0-255).
 * @param b Blue color component (0-255).
 *
 * @note This function sets and displays RGB values for both the onboard LED and all external LEDs.
 *       Each parameter represents the intensity of the respective color (0 for minimum intensity, 255 for maximum).
 *       Example usage: set_and_show_everything_rgb(255, 0, 0) to set and display all LEDs in red.
 */
void set_and_show_everything_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    set_and_show_onboard_led_rgb(r, g, b);
    set_and_show_all_external_leds_rgb(r, g, b);
}

/**
 * @brief Set and display the color of both the onboard LED and all external LEDs using a hexadecimal color string.
 *
 * @param hexColor Hexadecimal color string (e.g., "FF8800").
 *
 * @note This function sets and displays the color of both the onboard LED and all external LEDs using a hexadecimal color string.
 *       The hexColor parameter should be a string of length 6 representing RGB values in hexadecimal format.
 *       Example usage: set_and_show_everything_hex("00FFFF") to set and display all LEDs in cyan.
 */
void set_and_show_everything_hex(const char *hexColor)
{
    set_and_show_onboard_led_hex(hexColor);
    set_and_show_all_external_leds_hex(hexColor);
}

/**
 * @brief Cycle through all colors and then reset the LEDs.
 *
 * @note This function cycles through red, green, blue, and turns off all LEDs with a short delay between each color change.
 *       It uses the set_and_show_everything_rgb function for each color change.
 */
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

#pragma region init functions

/**
 * @brief Initialize the WS2812B LEDs.
 *
 * @param enable_onboard_led Whether to enable initialization for the onboard LED that comes with the Maker Pico Board.
 * @param enable_external_led Whether to enable initialization for external LED strip(s).
 *
 * @note This function initializes the WS2812B LEDs by configuring the GPIO pins for the onboard and external LEDs.
 *       If enable_onboard_led is true, it initializes the onboard LED pin to get ready for output.
 *       If enable_external_led is true, it initializes the external LED pin to get ready for output.
 *       A short delay (100ms) is added after initialization to allow the LEDs to reset.
 */
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

/**
 * @brief Initialize all WS2812B LEDs.
 *
 * @note This function initializes both the onboard and external WS2812B LED pins.
 *       It is a shorthand for calling ws2812b_init(true, true).
 */
void ws2812b_init_all()
{
    ws2812b_init(true, true);
}

/**
 * @brief Initialize only the external WS2812B LED.
 *
 * @note This function initializes only the external WS2812B LED pin.
 *       It is a shorthand for calling ws2812b_init(false, true).
 */
void ws2812b_init_external_led()
{
    ws2812b_init(false, true);
}

/**
 * @brief Initialize only the onboard WS2812B LED.
 *
 * @note This function initializes only the onboard WS2812B LED pin.
 *       It is a shorthand for calling ws2812b_init(true, false).
 */
void ws2812b_init_onboard_led()
{
    ws2812b_init(true, false);
}

#pragma endregion