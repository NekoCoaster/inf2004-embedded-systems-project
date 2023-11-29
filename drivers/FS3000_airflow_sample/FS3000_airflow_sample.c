#include <stdio.h>
#include <pico/stdlib.h>
#include "FS3000_Rebuilt.h"

int main()
{
    stdio_init_all();
    i2c_tools_init(i2c0, PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN);
    i2c_tools_begin();

    if (!FS3000_begin())
    {
        printf("FS3000 Not Detected. Please check wiring! Retrying in 3 seconds...\n");
        sleep_ms(1000);
    }
    FS3000_setRange(AIRFLOW_RANGE_15_MPS);

    printf("FS3000 Sensor Connected. Reading Data...\n");

    while (1)
    {
        printf("FS3000 Readings \tRaw: ");
        printf("%d\n", FS3000_readRaw()); // note, this returns an int from 0-3686

        printf("\tm/s: ");
        printf("%f\n", FS3000_readMetersPerSecond()); // note, this returns a float from 0-7.23 for the FS3000-1005, and 0-15 for the FS3000-1015

        printf("\tmph: ");
        printf("%f\n", FS3000_readMilesPerHour()); // note, this returns a float from 0-16.17 for the FS3000-1005, and 0-33.55 for the FS3000-1015

        sleep_ms(1000); // note, reponse time on the sensor is 125ms
    }

    return 0;
}