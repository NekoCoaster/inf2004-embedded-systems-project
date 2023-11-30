#include <stdio.h>
#include <pico/stdlib.h>
#include "i2c_tools.h"
#include "FS3000_Rebuilt.h"

static uint8_t _buff[FS3000_TO_READ];                                                 //	5 Bytes Buffer
static uint8_t _range = AIRFLOW_RANGE_7_MPS;                                          // defaults to FS3000-1005 range
static float _mpsDataPoint[13] = {0, 1.07, 2.01, 3.00, 3.97, 4.96, 5.98, 6.99, 7.23}; // defaults to FS3000-1005 datapoints
static int _rawDataPoint[13] = {409, 915, 1522, 2066, 2523, 2908, 3256, 3572, 3686};  // defaults to FS3000-1005 datapoints

// Make sure to initialize the I2C bus before calling this function
// Initializes the sensor (no settings to adjust)
// Returns false if sensor is not detected
bool FS3000_begin()
{
    return FS3000_isConnected();
}
// Returns true if I2C device ack's
bool FS3000_isConnected()
{
    i2c_tools_beginTransmission((uint8_t)FS3000_DEVICE_ADDRESS);
    return (i2c_tools_endTransmission() == 0);
}
/*************************** SET RANGE OF SENSOR ****************/
/*  There are two varieties of this sensor (1) FS3000-1005 (0-7.23 m/sec)
and (2) FS3000-1015 (0-15 m/sec)

Valid input arguments are:
AIRFLOW_RANGE_7_MPS
AIRPLOW_RANGE_15_MPS

Note, this also sets the datapoints (from the graphs in the datasheet pages 6 and 7).
These datapoints are used to convert from raw values to m/sec - and then mph.

*/
void FS3000_setRange(uint8_t range)
{
    _range = range;
    const float mpsDataPoint_7_mps[9] = {0, 1.07, 2.01, 3.00, 3.97, 4.96, 5.98, 6.99, 7.23}; // FS3000-1005 datapoints
    const int rawDataPoint_7_mps[9] = {409, 915, 1522, 2066, 2523, 2908, 3256, 3572, 3686};  // FS3000-1005 datapoints

    const float mpsDataPoint_15_mps[13] = {0, 2.00, 3.00, 4.00, 5.00, 6.00, 7.00, 8.00, 9.00, 10.00, 11.00, 13.00, 15.00}; // FS3000-1015 datapoints
    const int rawDataPoint_15_mps[13] = {409, 1203, 1597, 1908, 2187, 2400, 2629, 2801, 3006, 3178, 3309, 3563, 3686};     // FS3000-1015 datapoints

    if (_range == AIRFLOW_RANGE_7_MPS)
    {
        for (int i = 0; i < 9; i++)
        {
            _mpsDataPoint[i] = mpsDataPoint_7_mps[i];
            _rawDataPoint[i] = rawDataPoint_7_mps[i];
        }
    }
    else if (_range == AIRFLOW_RANGE_15_MPS)
    {
        for (int i = 0; i < 13; i++)
        {
            _mpsDataPoint[i] = mpsDataPoint_15_mps[i];
            _rawDataPoint[i] = rawDataPoint_15_mps[i];
        }
    }
}
/*************************** READ RAW **************************/
/*  Read from sensor, checksum, return raw data (409-3686)     */
uint16_t FS3000_readRaw()
{
    FS3000_readData(_buff);
    bool checksum_result = FS3000_checksum(_buff, false); // debug off
    if (checksum_result == false)                         // checksum error
    {
        // return 9999;
    }

    uint16_t airflowRaw = 0;
    uint8_t data_high_byte = _buff[1];
    uint8_t data_low_byte = _buff[2];

    // The flow data is a 12-bit integer.
    // Only the least significant four bits in the high byte are valid.
    // clear out (mask out) the unnecessary bits
    data_high_byte &= (0b00001111);

    // combine high and low bytes, to express sensors 12-bit value
    airflowRaw |= data_low_byte;
    airflowRaw |= (data_high_byte << 8);

    return airflowRaw;
}

/*************************** READ METERS PER SECOND****************/
/*  Read from sensor, checksum, return m/s (0-7.23)               */
float FS3000_readMetersPerSecond()
{
    float airflowMps = 0.0;
    int airflowRaw = FS3000_readRaw();
    uint8_t dataPointsNum = 9; // Default to FS3000_1005 AIRFLOW_RANGE_7_MPS
    if (_range == AIRFLOW_RANGE_7_MPS)
    {
        dataPointsNum = 9;
    }
    else if (_range == AIRFLOW_RANGE_15_MPS)
    {
        dataPointsNum = 13;
    }

    // find out where our raw data fits in the arrays
    int data_position = 0;

    for (int i = 0; i < dataPointsNum; i++) // cound be an array of datapoints 9 or 13 long...
    {
        if (airflowRaw > _rawDataPoint[i])
        {
            data_position = i;
            // printfln(data_position);
        }
    }

    // set limits on min and max.
    // if we are at or below 409, we'll bypass conversion and report 0.
    // if we are at or above 3686, we'll bypass conversion and report max (7.23 or 15)
    if (airflowRaw <= 409)
        return 0;
    if (airflowRaw >= 3686)
    {
        if (_range == AIRFLOW_RANGE_7_MPS)
            return 7.23;
        if (_range == AIRFLOW_RANGE_15_MPS)
            return 15.00;
    }

    // look at where we are between the two data points in the array.
    // now use the percentage of that window to calculate m/s

    // calculate the percentage of the window we are at.
    // using the data_position, we can find the "edges" of the data window we are in
    // and find the window size

    int window_size = (_rawDataPoint[data_position + 1] - _rawDataPoint[data_position]);
    // printf("window_size: ");
    // printf(window_size);

    // diff is the amount (difference) above the bottom of the window
    int diff = (airflowRaw - _rawDataPoint[data_position]);
    // printf("diff: ");
    // printf(diff);
    float percentage_of_window = ((float)diff / (float)window_size);
    // printf("\tpercent: ");
    // printf(percentage_of_window);

    // calculate window size from MPS data points
    float window_size_mps = (_mpsDataPoint[data_position + 1] - _mpsDataPoint[data_position]);
    // printf("\twindow_size_mps: ");
    // printf(window_size_mps);

    // add percentage of window_mps to mps.
    airflowMps = _mpsDataPoint[data_position] + (window_size_mps * percentage_of_window);

    return airflowMps;
}
/*************************** READ MILES PER HOUR****************/
/*  Read from sensor, checksum, return mph (0-33ish)     */
float FS3000_readMilesPerHour()
{
    return (FS3000_readMetersPerSecond() * 2.2369362912);
}

/*************************** READ DATA *************************/
/*                Read 5 bytes from sensor, put it at a pointer (given as argument)                  */
void FS3000_readData(uint8_t *buffer_in)
{

    // i2c_tools_reqeustFrom contains the beginTransmission and endTransmission in it.
    // LMAO, NO IT DOESN'T. Dug up the original Arduino Wire.cpp code and patched it here:
    i2c_tools_beginTransmission((uint8_t)FS3000_DEVICE_ADDRESS);
    i2c_tools_write((uint8_t)FS3000_DEVICE_ADDRESS);
    i2c_tools_endTransmission();
    // end of patch

    i2c_tools_requestFrom(FS3000_DEVICE_ADDRESS, FS3000_TO_READ); // Request 5 Bytes

    uint8_t i = 0;
    while (i2c_tools_available())
    {
        buffer_in[i] = i2c_tools_read(); // Receive Byte
        i += 1;
    }
    // printf("i:%d\n", i);
}

/****************************** CHECKSUM *****************************
 * Check to see that the CheckSum is correct, and data is good
 * Return true if all is good, return false if something is off
 * The entire response from the FS3000 is 5 bytes.
 * [0]Checksum
 * [1]data high
 * [2]data low
 * [3]generic checksum data
 * [4]generic checksum data
 */

/**
 * @brief Calculates and verifies the checksum for FS3000 sensor data.
 *
 * This function takes an array of 5 data bytes, calculates the checksum,
 * and verifies it against the received checksum byte. The calculated checksum
 * is displayed if the `show_debug` parameter is set to true. The function returns
 * true if the checksum is valid, indicating that the data integrity is maintained,
 * and false otherwise.
 *
 * @param data_in An array containing 5 data bytes, including the received checksum byte.
 * 
 * @param show_debug A boolean flag indicating whether to display debug information,
 * including the calculated checksum and other intermediate values.
 * 
 * @return Returns true if the calculated checksum matches the received checksum, indicating
 * valid data integrity. Returns false otherwise.
 */
bool FS3000_checksum(uint8_t *data_in, bool show_debug)
{
    uint8_t sum = 0;

    // Calculate the sum of the data bytes excluding the checksum byte
    for (int i = 1; i <= 4; i++)
    {
        sum += (uint8_t)(data_in[i]);
    }

    if (show_debug)
    {
        // Display the received data bytes and the calculated sum
        for (int i = 0; i < 5; i++)
        {
            hexToAscii(_buff[i]);
            printf(" ");
        }
        printf("\n\rSum of received data bytes                       = ");
        FS3000_printHexByte(sum);
    }

    // Calculate the checksum and verify it against the received checksum byte
    sum %= 256;
    uint8_t calculated_cksum = (~(sum) + 1);
    uint8_t crcbyte = data_in[0];
    uint8_t overall = sum + crcbyte;

    if (show_debug)
    {
        // Display the calculated checksum, received checksum byte, and the overall sum
        printf("Calculated checksum                              = ");
        FS3000_printHexByte(calculated_cksum);
        printf("Received checksum byte                           = ");
        FS3000_printHexByte(crcbyte);
        printf("Sum of received data bytes and received checksum = ");
        FS3000_printHexByte(overall);
        printf("\n");
    }

    // Return true if the overall sum is 0, indicating valid data integrity
    if (overall != 0x00)
    {
        return false;
    }
    return true;
}

/**
 * @brief Prints a byte in hexadecimal format with a "0x" prefix.
 *
 * This function takes a single byte, `x`, and prints its hexadecimal representation
 * with a "0x" prefix. If the byte is less than 16, a leading zero is added for clarity.
 * The function then prints a newline character for formatting purposes.
 *
 * @param x The byte to be printed in hexadecimal format.
 * 
 * @return void
 */
void FS3000_printHexByte(uint8_t x)
{
    // Print "0x" prefix
    printf("0x");

    // Add leading zero if the byte is less than 16
    if (x < 16)
    {
        printf("0");
    }

    // Print the hexadecimal representation of the byte
    hexToAscii(x);

    // Print a newline character for formatting
    printf("\n");
}
