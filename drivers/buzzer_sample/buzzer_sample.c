#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"

// configs for Maker Pico's Buzzer Pin
#define BUZZER_PIN 18

// Don't touch these.
uint buzzer_pwm_channel;
uint buzzer_pwm_slice;
uint16_t buzzer_pwm_wrap = 0; // Used for calculating volume level.

void buzzer_set_freq(float clkdiv_value, uint16_t wrap_value)
{
    // Performs value validity check.
    if (clkdiv_value < 0.0f || clkdiv_value > 255.15)
    {
        printf("Invalid clkdiv_value %f. Acceptable Range: 0.0 - 255.15\n");
        return;
    }
    pwm_set_wrap(buzzer_pwm_slice, wrap_value);
    pwm_set_clkdiv(buzzer_pwm_slice, clkdiv_value);
    buzzer_pwm_wrap = wrap_value;
    printf("Set Buzzer Frequency to: %f\n", (125000000 / clkdiv_value / wrap_value));
    pwm_set_enabled(buzzer_pwm_slice, true);
    return;
}

void buzzer_set_volume(int volume) // or also known as duty cycle setter. (well... it kinda works lmao.)
{
    // Clamps max value to 100
    volume = volume > 100 ? 100 : volume;

    float duty_cycle = (volume / 2000.0f);
    pwm_set_chan_level(buzzer_pwm_slice, buzzer_pwm_channel, (uint16_t)(buzzer_pwm_wrap * duty_cycle));
    pwm_set_enabled(buzzer_pwm_slice, true);
    printf("Set buzzer volume to %d %% \n", volume);
}

void buzzer_begin()
{
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    buzzer_pwm_channel = pwm_gpio_to_channel(BUZZER_PIN);
    buzzer_pwm_slice = pwm_gpio_to_slice_num(BUZZER_PIN);
}

int main()
{
    stdio_init_all();
    buzzer_begin();
    // You'll need to calculate the clk and wrap frequency using the following formula:
    // (125,000,000 ÷ clkdiv_value) ÷ wrap_value = ur_desired_frequency
    // clkdiv value range: 0 - 225.15
    // Here, we use the muisc note C#4 as our default note with a frequency of 440.
    // 125,000,000 ÷ 200 ÷ 2,256
    buzzer_set_freq(200.0f, 1420);

    while (true)
    {
        buzzer_set_volume(0);
        sleep_ms(1000);
        buzzer_set_volume(25);
        sleep_ms(1000);
        buzzer_set_volume(50);
        sleep_ms(1000);
        buzzer_set_volume(75);
        sleep_ms(1000);
        buzzer_set_volume(100);
        sleep_ms(1000);
    }

    return 0;
}