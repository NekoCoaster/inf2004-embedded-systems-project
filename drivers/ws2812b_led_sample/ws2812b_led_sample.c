#include <stdlib.h>
#include "pico/stdlib.h"
#include "ws2812b_Rebuilt.h"

void running_light(uint8_t r, uint8_t g, uint8_t b, uint8_t wait)
{
    for (int i = 0; i < 8; i++)
    {
        set_external_led_rgb(i, r, g, b);
        show_external_led();
        sleep_ms(wait);
    }
}

void reset_all_leds()
{
    set_all_external_leds_rgb(0, 0, 0);
    show_external_led();
}

int main()
{
    stdio_init_all();
    ws2812b_init_all();
    while (true)
    {
        set_makerpico_led_rgb(255, 0, 0);
        show_makerpico_led();
        sleep_ms(500);
        running_light(255, 0, 0, 50);
        sleep_ms(500);
        set_makerpico_led_rgb(0, 255, 0);
        show_makerpico_led();
        sleep_ms(500);
        running_light(0, 255, 0, 50);
        sleep_ms(500);
        set_makerpico_led_rgb(0, 0, 255);
        show_makerpico_led();
        sleep_ms(500);
        running_light(0, 0, 255, 50);
        sleep_ms(500);
    }
}