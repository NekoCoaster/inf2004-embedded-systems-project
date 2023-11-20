#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define FIRST_LED_PIN 2
#define TOTAL_LEDS 8

void begin_leds()
{
    for (int i = FIRST_LED_PIN; i < FIRST_LED_PIN + TOTAL_LEDS; i++)
    {
        gpio_init(i);
        gpio_set_dir(i, GPIO_OUT);
        gpio_put(i, 0);
    }
}

void project_bits_on_led(uint8_t value)
{
    for (int i = FIRST_LED_PIN; i < FIRST_LED_PIN + TOTAL_LEDS; i++)
    {
        gpio_put(i, value & 1); // masks the value with 1 so led on/off is determined by the last bit to the right
        value >>= 1;            // shift bits to the right
    }
}

int main()
{
    stdio_init_all();
    begin_leds();
    while (true)
    {
        for (uint8_t l = 0; l < 256; l++)
        {
            project_bits_on_led(l);
            printf("value = %d\n", l);
            sleep_ms(100);
        }
    }
}