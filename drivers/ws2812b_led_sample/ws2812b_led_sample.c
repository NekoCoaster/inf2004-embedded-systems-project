/** @file WS2812B_led_sample.c
 * This is a sample program showcasing the use of the WS2812B library.
 */

#include <stdlib.h>
#include "pico/stdlib.h"
#include "ws2812b_Rebuilt.h"

/**
 * @brief Runs a light sequence on external LEDs.
 *
 * This function runs a light sequence on external LEDs by setting the RGB color for each LED,
 * showing the LEDs, and then sleeping for a specified duration before moving to the next LED.
 *
 * @param r The red component of the RGB color.
 * @param g The green component of the RGB color.
 * @param b The blue component of the RGB color.
 * @param wait The duration to sleep between LED transitions in milliseconds.
 */
void running_light(uint8_t r, uint8_t g, uint8_t b, uint8_t wait)
{
    // Iterate through each external LED.
    for (int i = 0; i < 8; i++)
    {
        // Set the RGB color for the current LED.
        set_external_led_rgb(i, r, g, b);

        // Show the external LEDs.
        show_external_led();

        // Sleep for the specified duration before moving to the next LED.
        sleep_ms(wait);
    }
}

/**
 * @brief Main function for the program.
 *
 * This function initializes the standard I/O and WS2812B LEDs, and then runs a continuous loop
 * of changing the color of the onboard LED and running a light sequence on external LEDs.
 */
int main()
{
    // Initialize standard I/O for printf.
    stdio_init_all();

    // Initialize WS2812B LEDs.
    ws2812b_init_all();

    // Continuous loop.
    while (true)
    {
        // Set onboard LED color to red and display it.
        set_makerpico_led_rgb(255, 0, 0);
        show_makerpico_led();

        // Sleep for 500 milliseconds.
        sleep_ms(500);

        // Run a light sequence on external LEDs with red color and short intervals.
        running_light(255, 0, 0, 50);

        // Sleep for 500 milliseconds.
        sleep_ms(500);

        // Set onboard LED color to green and display it.
        set_makerpico_led_rgb(0, 255, 0);
        show_makerpico_led();

        // Sleep for 500 milliseconds.
        sleep_ms(500);

        // Run a light sequence on external LEDs with green color and short intervals.
        running_light(0, 255, 0, 50);

        // Sleep for 500 milliseconds.
        sleep_ms(500);

        // Set onboard LED color to blue and display it.
        set_makerpico_led_rgb(0, 0, 255);
        show_makerpico_led();

        // Sleep for 500 milliseconds.
        sleep_ms(500);

        // Run a light sequence on external LEDs with blue color and short intervals.
        running_light(0, 0, 255, 50);

        // Sleep for 500 milliseconds.
        sleep_ms(500);
    }

    // The program should not reach this point.
    return 0;
}
