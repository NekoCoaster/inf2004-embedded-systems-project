#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/pwm.h"

#define FAN_TACHO_PIN 15
#define FAN_PWM_PIN 14

#define DEFAULT_DUTY_CYCLE 100

// NOTE: Use tacho count measure rotational speed of fan...
volatile uint32_t tacho_count = 0;
volatile float last_fan_speed = 0;

static uint fan_pwm_slice_num;
static uint fan_pwm_chan;

struct repeating_timer periodic_fan_speed_check_timer;

// Called everytime the fan's internal tachometer sends a LOW pulse.
static void tacho_callback(uint gpio, uint32_t events)
{
    tacho_count++;
}

static float calculate_fan_speed(uint32_t tacho_count)
{
    return (float)tacho_count * 60.0 / 2.0;
}

static bool tacho_speed_check_callback(struct repeating_timer *t)
{
    last_fan_speed = calculate_fan_speed(tacho_count);
    tacho_count = 0;
    return true;
}

// Calculate the duty cycle based on the wrap value and the duty cycle percentage.
static float get_pwm_dc_level(int duty_cycle_percent)
{
    // Clamp the duty cycle percentage, between 20% to 100%
    duty_cycle_percent = duty_cycle_percent < 0 ? 0 : duty_cycle_percent;
    duty_cycle_percent = duty_cycle_percent > 100 ? 100 : duty_cycle_percent;

    return (float)(50 * (duty_cycle_percent / 100.0));
}

static void set_fan_speed(int speed_percent)
{
    uint fan_pwm_slice_num = pwm_gpio_to_slice_num(FAN_PWM_PIN);
    pwm_set_chan_level(fan_pwm_slice_num, fan_pwm_chan, get_pwm_dc_level(speed_percent));
}

float NFA4X10_get_fan_rpm()
{
    return last_fan_speed;
}

void NFA4X10_set_fan_speed(int speed_percent)
{
    set_fan_speed(speed_percent);
}

void NFA4X10_init()
{

    // Sets up tachometer pin
    gpio_pull_up(FAN_TACHO_PIN);
    gpio_set_irq_enabled_with_callback(FAN_TACHO_PIN, GPIO_IRQ_EDGE_FALL, true, &tacho_callback);

    // Sets up pwm pin; Latches pin to LOW state...
    gpio_pull_down(FAN_PWM_PIN);
    gpio_set_function(FAN_PWM_PIN, GPIO_FUNC_PWM);

    // get the slice number and channel number from the GPIO number
    fan_pwm_slice_num = pwm_gpio_to_slice_num(FAN_PWM_PIN);
    fan_pwm_chan = pwm_gpio_to_channel(FAN_PWM_PIN);
    // Step down clock diver to 1.25MHz, then further divide by 50 to get 25kHz
    pwm_set_clkdiv(fan_pwm_slice_num, 100);
    pwm_set_wrap(fan_pwm_slice_num, 50);

    // Set the initial duty cycle to 100%
    set_fan_speed(DEFAULT_DUTY_CYCLE);
    // Enable the pwm channel
    pwm_set_enabled(fan_pwm_slice_num, true);

    add_repeating_timer_ms(-1000, tacho_speed_check_callback, NULL, &periodic_fan_speed_check_timer);
}
