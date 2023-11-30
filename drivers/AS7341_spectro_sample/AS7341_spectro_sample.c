#include <stdio.h>
#include <pico/stdlib.h>
#include "AS7341_Rebuilt.h"
#include "hardware/clocks.h"

// Initialization and Setup Segment
// This segment initializes the AS7341 sensor and reads its ID for verification
int main()
{
    stdio_init_all(); // Initializing standard I/O
    i2c_tools_init(i2c0, PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN); // Initializing I2C communication
    AS7341_begin(eSpm); // Initializing the AS7341 sensor with specified mode
    int sensorid = AS7341_readID(); // Reading the ID of the AS7341 sensor

    while (sensorid == 0) // Loop to check if sensor is connected
    {
        printf("AS7341 Sensor Not Connected. Please check the connection.\n"); // Printing error message if sensor is not connected
        sleep_ms(1000); // Delay for 1 second
        sensorid = AS7341_readID(); // Attempting to read sensor ID again
    }

    printf("AS7341 Sensor Connected. id=%d\n", sensorid); // Printing sensor ID if connected

    // Continuous Measurement Segment
    // This segment continuously retrieves spectral data from the AS7341 sensor
    while (true) // Infinite loop for continuous data retrieval
    {
        AS7341_sModeOneData_t sensor1to4 = getSensor1to4(); // Reading spectral data for sensors 1 to 4
        AS7341_sModeTwoData_t sensor5to8 = getSensor5to8(); // Reading spectral data for sensors 5 to 8

        // Printing spectral data for sensors 1 to 4
        printf("F1(405-425nm): %d\n", sensor1to4.ADF1);
        printf("F2(435-455nm): %d\n", sensor1to4.ADF2);
        printf("F3(470-490nm): %d\n", sensor1to4.ADF3);
        printf("F4(505-525nm): %d\n", sensor1to4.ADF4);

        // Printing spectral data for sensors 5 to 8
        printf("F5(545-565nm): %d\n", sensor5to8.ADF5);
        printf("F6(580-600nm): %d\n", sensor5to8.ADF6);
        printf("F7(620-640nm): %d\n", sensor5to8.ADF7);
        printf("F8(670-690nm): %d\n", sensor5to8.ADF8);

        // Printing additional sensor data
        printf("Visible Light: %d\n", sensor5to8.ADCLEAR);
        printf("Near Infrared: %d\n", sensor5to8.ADNIR);
        printf("\n"); // Adding a new line for better readability
        sleep_ms(3000); // Delay for 3 seconds before the next data retrieval
    }
}
