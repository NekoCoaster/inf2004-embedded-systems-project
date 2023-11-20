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

/* The GPIO pin for the LED data */
#define LED_PIN 28

/* Number of individual LEDs */
/* You can have up to 80,000 LEDs before you run out of memory */
#define LED_NUM 1

/* Leave alone, only defined to hammer it into the compilers head */
#define LED_DATA_SIZE 3
#define LED_BYTE_SIZE LED_NUM *LED_DATA_SIZE

/* 3 bytes per LED */
uint8_t led_data[LED_BYTE_SIZE];

/* Sets a specific LED to a certain color */
/* LEDs start at 0 */
void rgb_led_set_one(uint32_t led, uint8_t r, uint8_t g, uint8_t b)
{
    led_data[led * LED_DATA_SIZE] = g;       /* Green */
    led_data[(led * LED_DATA_SIZE) + 1] = r; /* Red */
    led_data[(led * LED_DATA_SIZE) + 2] = b; /* Blue */
}

/* Sets all the LEDs to a certain color */
void rgb_led_set_all(uint8_t r, uint8_t g, uint8_t b)
{
    for (uint32_t i = 0; i < LED_BYTE_SIZE; i += LED_DATA_SIZE)
    {
        led_data[i] = g;     /* Green */
        led_data[i + 1] = r; /* Red */
        led_data[i + 2] = b; /* Blue */
    }
}

/* Sends the data to the LEDs */
void rgb_led_show()
{
    /* Disable all interrupts and save the mask */
    uint32_t interrupt_mask = disable_and_save_interrupts();

    /* Get the pin bit */
    uint32_t pin = 1UL << LED_PIN;

    /* Declared outside to force optimization if compiler gets any funny ideas */
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    uint32_t i = 0;
    int8_t j = 0;

    for (i = 0; i < LED_BYTE_SIZE; i += LED_DATA_SIZE)
    {
        /* Send order is green, red, blue  */
        green = led_data[i];
        red = led_data[i + 1];
        blue = led_data[i + 2];

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

void rgb_led_begin()
{
    /* Set the pin to output */
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, false); /* Important to start low to tell the LEDs that it's time for new data */
    /*Wait a bit to force LEDs to reset*/
    sleep_ms(10);
}

int main()
{
    stdio_init_all();
    printf("WS2812B LED Sample\n");

    rgb_led_begin();

    while (true)
    {
        rgb_led_set_all(255, 0, 0); /* Red */
        rgb_led_show();
        sleep_ms(1000);
        rgb_led_set_all(255, 255, 0); /* Yellow */
        rgb_led_show();
        sleep_ms(1000);
        rgb_led_set_all(0, 255, 0); /* Green */
        rgb_led_show();
        sleep_ms(1000);
        rgb_led_set_all(0, 255, 255); /* Cyan */
        rgb_led_show();
        sleep_ms(1000);
        rgb_led_set_all(0, 0, 255); /* Blue */
        rgb_led_show();
        sleep_ms(1000);
        rgb_led_set_all(255, 0, 255); /* Magenta */
        rgb_led_show();
        sleep_ms(1000);
        rgb_led_set_all(255, 255, 255); /* White */
        rgb_led_show();
        sleep_ms(1000);
    }

    return 0;
}
