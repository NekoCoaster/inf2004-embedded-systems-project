#include <stdio.h>
#include <pico/stdlib.h>
#include "MLX90614_rebuilt.h"

int main()
{
    stdio_init_all();
    i2c_tools_init(i2c0, PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN);
    MLX90614_I2C_init(0x5A);

    while (NO_ERR != MLX90614_I2C_begin())
    {
        printf("Communication with device failed, please check connection\n");
        sleep_ms(3000);
    }
    printf("Begin ok!");

    MLX90614_enterSleepMode(true);
    sleep_ms(50);
    MLX90614_enterSleepMode(false);
    sleep_ms(200);
    while (true)
    {
        printf("Ambient temperature: %.2f\n", MLX90614_getAmbientTempCelsius());
        printf("Object temperature: %.2f\n", MLX90614_getObjectTempCelsius());
        printf("\n");
        sleep_ms(1000);
    }
    return 0;
}