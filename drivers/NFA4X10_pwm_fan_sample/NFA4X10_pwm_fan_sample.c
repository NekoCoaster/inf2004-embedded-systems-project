/**
NOTE: Sample code for PWM Fan...

Starts from 20%, inrements to 100% before resetting back to 20%...
*/

#include <stdio.h>
#include "pico/stdlib.h"
#include "NFA4X10_Rebuilt.h"

int main()
{
    stdio_init_all();
    NFA4X10_init();

    int speed_percent = 0;
    while (1)
    {
        NFA4X10_set_fan_speed(speed_percent);

        printf("Fan speed: %d%%\n", speed_percent);
        printf("Fan speed: %.2f RPM\n", NFA4X10_get_fan_rpm());

        sleep_ms(5000);
        speed_percent += 20;
        if (speed_percent > 100)
        {
            speed_percent = 0;
        }
    };
}
