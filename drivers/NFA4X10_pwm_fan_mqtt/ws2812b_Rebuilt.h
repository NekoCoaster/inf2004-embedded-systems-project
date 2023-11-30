/** @file ws2812b_Rebuilt.h
 * Original WS2812B LED code by yeahimthatguy @ https://forums.raspberrypi.com/viewtopic.php?t=322218
 * Adapted into standalone library to support both the MakerPico's onboard WS2812B LED and 1 additional external WS2812B LED strip.
 *
 */
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
extern uint32_t disable_and_save_interrupts();       /* Used for disabling all interrupts */
extern void enable_and_restore_interrupts(uint32_t); /* Used for enabling all interrupts */

typedef enum
{
    WS2812B_LED_TYPE_EXTERNAL,
    WS2812B_LED_TYPE_MAKERPICO
} ws2812b_led_type_t;
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
void hexToRGB(const char *hexColor, uint8_t *red, uint8_t *green, uint8_t *blue);

#pragma region Onboard LED functions

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
void set_onboard_led_rgb(uint8_t r, uint8_t g, uint8_t b);
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
void set_onboard_led_hex(const char *hexColor);
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
void show_onboard_led();
#pragma endregion

#pragma region External LED functions
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
void set_external_led_rgb(uint32_t ledIndex, uint8_t r, uint8_t g, uint8_t b);

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
void set_external_led_hex(uint32_t ledIndex, const char *hexColor);

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
void set_all_external_leds_rgb(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Set the color of all external LEDs using a hexadecimal color string.
 *
 * @param hexColor Hexadecimal color string (e.g., "FF8800").
 *
 * @note The function sets the color of all external LEDs using a hexadecimal color string.
 *       The hexColor parameter should be a string of length 6 representing RGB values in hexadecimal format.
 *       Example usage: set_all_external_leds_hex("00FFFF") to set all external LEDs to cyan.
 */
void set_all_external_leds_hex(const char *hexColor);

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
void show_external_leds();
#pragma endregion
#pragma region Quick set functions
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
void set_and_show_onboard_led_rgb(uint8_t r, uint8_t g, uint8_t b);

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
void set_and_show_onboard_led_hex(const char *hexColor);

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
void set_and_show_external_led_rgb(uint32_t ledIndex, uint8_t r, uint8_t g, uint8_t b);

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
void set_and_show_external_led_hex(uint32_t ledIndex, const char *hexColor);

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
void set_and_show_all_external_leds_rgb(uint8_t r, uint8_t g, uint8_t b);

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
void set_and_show_all_external_leds_hex(const char *hexColor);

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
void set_and_show_everything_rgb(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Set and display the color of both the onboard LED and all external LEDs using a hexadecimal color string.
 *
 * @param hexColor Hexadecimal color string (e.g., "FF8800").
 *
 * @note This function sets and displays the color of both the onboard LED and all external LEDs using a hexadecimal color string.
 *       The hexColor parameter should be a string of length 6 representing RGB values in hexadecimal format.
 *       Example usage: set_and_show_everything_hex("00FFFF") to set and display all LEDs in cyan.
 */
void set_and_show_everything_hex(const char *hexColor);

/**
 * @brief Cycle through all colors and then reset the LEDs.
 *
 * @note This function cycles through red, green, blue, and turns off all LEDs with a short delay between each color change.
 *       It uses the set_and_show_everything_rgb function for each color change.
 */
void reset_all_leds();
#pragma endregion
#pragma Initalization functions

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
void ws2812b_init(bool enable_onboard_led, bool enable_external_led);

/**
 * @brief Initialize all WS2812B LEDs.
 *
 * @note This function initializes both the onboard and external WS2812B LED pins.
 *       It is a shorthand for calling ws2812b_init(true, true).
 */
void ws2812b_init_all();

/**
 * @brief Initialize only the external WS2812B LED.
 *
 * @note This function initializes only the external WS2812B LED pin.
 *       It is a shorthand for calling ws2812b_init(false, true).
 */
void ws2812b_init_external_led();

/**
 * @brief Initialize only the onboard WS2812B LED.
 *
 * @note This function initializes only the onboard WS2812B LED pin.
 *       It is a shorthand for calling ws2812b_init(true, false).
 */
void ws2812b_init_onboard_led();
#pragma endregion

#endif