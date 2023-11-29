#include <stdio.h>
#include <pico/stdlib.h>
#include "AS7341_Rebuilt.h"
#include "hardware/clocks.h"

// Function to read spectral data for sensors 1 to 4
AS7341_sModeOneData_t getSensor1to4()
{
    AS7341_startMeasure(eF1F4ClearNIR);
    return AS7341_readSpectralDataOne();
}

// Function to read spectral data for sensors 5 to 8
AS7341_sModeTwoData_t getSensor5to8()
{
    AS7341_startMeasure(eF5F8ClearNIR);
    return AS7341_readSpectralDataTwo();
}

int main()
{
    stdio_init_all();
    i2c_tools_init(i2c0, PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN);
    AS7341_begin(eSpm);
    int sensorid = AS7341_readID();

    while (sensorid == 0)
    {
        printf("AS7341 Sensor Not Connected. Please check the connection.\n");
        sleep_ms(1000);
        sensorid = AS7341_readID();
    }
    printf("AS7341 Sensor Connected. id=%d\n", sensorid);

    while (true)
    {
        AS7341_sModeOneData_t sensor1to4 = getSensor1to4();
        AS7341_sModeTwoData_t sensor5to8 = getSensor5to8();

        printf("F1(405-425nm): %d\n", sensor1to4.ADF1);
        printf("F2(435-455nm): %d\n", sensor1to4.ADF2);
        printf("F3(470-490nm): %d\n", sensor1to4.ADF3);
        printf("F4(505-525nm): %d\n", sensor1to4.ADF4);
        printf("F5(545-565nm): %d\n", sensor5to8.ADF5);
        printf("F6(580-600nm): %d\n", sensor5to8.ADF6);
        printf("F7(620-640nm): %d\n", sensor5to8.ADF7);
        printf("F8(670-690nm): %d\n", sensor5to8.ADF8);
        printf("Visible Light: %d\n", sensor5to8.ADCLEAR);
        printf("Near Infrared: %d\n", sensor5to8.ADNIR);
        printf("\n");
        sleep_ms(3000);
    }
}