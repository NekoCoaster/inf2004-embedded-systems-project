/*!
 *@file DFRobot_AS7341.cpp
 *@brief Define the basic structure of class DFRobot_AS7341, the implementation of the basic methods
 *@copyright   Copyright (c) 2010 DFRobot Co.Ltd (http://www.dfrobot.com)
 *@license     The MIT license (MIT)
 *@author [fengli](li.feng@dfrobot.com)
 *@version  V1.0
 *@date  2020-07-16
 *@url https://github.com/DFRobot/DFRobot_AS7341
 */

//Including necessary header files
#include <stdio.h>
#include <pico/stdlib.h>
#include "AS7341_Rebuilt.h"
#include "i2c_tools.h"
#include <stdint.h>

static uint8_t _address; //Static variable for I2C address
static uint8_t _mode; //Static variable for mode 
static AS7341_eMode_t measureMode; //Enum type variable for measuring mode

#define AS7341_DEBUG


#ifdef AS7341_DEBUG
void AS7341_debugPrint(const char *str) 
{ 
	//Function to print debug message
    printf("%s\n", str);
}
#else
void AS7341_debugPrint(const char *str)
{
    // Do nothing (Dummy function if debug is disabled)
}
#endif
void AS7341_init()
{
}

// Function to write data to a register via I2C
void AS7341_writeReg(uint8_t reg, void *pBuf, size_t size)
{
    if (pBuf == NULL)
    {
        AS7341_debugPrint("pBuf ERROR!! : null pointer"); // Error message if buffer is NULL
    }
    uint8_t *_pBuf = (uint8_t *)pBuf; // Casting the buffer pointer to uint8_t
    i2c_tools_beginTransmission((uint8_t)_address); // Begin I2C transmission
    i2c_tools_write(reg); // Write register address
    for (uint16_t i = 0; i < size; i++)
    {
        i2c_tools_write(_pBuf[i]); // Write data to the register
    }
    i2c_tools_endTransmission(); // End I2C transmission
}

// Function to write data directly to a register via I2C
void AS7341_writeReg_direct(uint8_t reg, uint8_t data) {
    AS7341_writeReg(reg, &data, 1); // Call AS7341_writeReg function with data
}

// Function to read data from a register via I2C
uint8_t AS7341_readReg(uint8_t reg, void *pBuf, size_t size) 
{
    if (pBuf == NULL) 
	{
        AS7341_debugPrint("pBuf ERROR!! : null pointer"); // Error message if buffer is NULL
    }
    uint8_t *_pBuf = (uint8_t *)pBuf; // Casting the buffer pointer to uint8_t
    i2c_tools_beginTransmission(_address); // Begin I2C transmission
    i2c_tools_write(reg); // Write register address
    if (i2c_tools_endTransmission() != 0) 
	{
        return 0;
    }
    busy_wait_ms(10); // Delay for 10 milliseconds
    i2c_tools_requestFrom(_address, size); // Request data from the register
    for (uint16_t i = 0; i < size; i++) 
	{
        _pBuf[i] = i2c_tools_read(); // Read data from the register
    }
    return size; // Return size of data read
}

// Function to read data directly from a register via I2C
uint8_t AS7341_readReg_direct(uint8_t reg) 
{
    uint8_t data;
    AS7341_readReg(reg, &data, 1); // Call AS7341_readReg function
    return data; // Return the read data
}

// Function to initialize the AS7341 sensor
int AS7341_begin(AS7341_eMode_t mode) 
{
    _address = 0x39; // Set the I2C address
    i2c_tools_begin(); // Begin I2C communication
    i2c_tools_beginTransmission(_address); // Begin I2C transmission

    /*  Section commented out due to bugs in the code
	 *	 Skip this section because it's all bugged all to high hell.
     *   For some ****ing reason, the i2c_tools library that I've ported over from Wire.h works with the other I2C sensors
     *   And even after debugging line by line, stepping into each functions and comparing the code and onboard memory,
     *   They both look the same, but for some reason, _clockStrech returns true on the arduino framework, but false on the pico C SDK
     *   So as a last resort, I tried skipping this test section and somehow, the rest of the sensor code still works just fine.
     *   so fk it. I'm leaving it out.
     */

    /*
    if (i2c_tools_endTransmission() != 0)
    {
        AS7341_debugPrint("");
        AS7341_debugPrint("bus data access error");
        AS7341_debugPrint("");
        return ERR_DATA_BUS;
    }
    */
    AS7341_enableAS7341(true); // Enable AS7341 sensor
    measureMode = mode; // Set the measure mode
    return ERR_OK; // Return OK status
}


// Function to read the ID of the AS7341 sensor
uint8_t AS7341_readID() 
{
    uint8_t id;
    if (AS7341_readReg(REG_AS7341_ID, &id, 1) == 0) 
	{
        AS7341_debugPrint("id read error"); // Error message if ID read fails
        return 0;
    } 
	else 
	{
        return id; // Return the read ID
    }
}

// Function to enable or disable AS7341
void AS7341_enableAS7341(bool on)
{
    uint8_t data; // Variable to store register data
    AS7341_readReg(REG_AS7341_ENABLE, &data, 1); // Reading data from specified register

    // Checking the condition for enabling or disabling based on boolean parameter 'on'
    if (on == true)
    {
        data = data | (1 << 0); // Setting specific bit to enable
    }
    else
    {
        data = data & (~1); // Clearing specific bit to disable
    }
    AS7341_writeReg(REG_AS7341_ENABLE, &data, 1); // Writing modified data to register
}

// Function to enable or disable spectral measurement
void AS7341_enableSpectralMeasure(bool on)
{
    uint8_t data; // Variable to store register data
    AS7341_readReg(REG_AS7341_ENABLE, &data, 1); // Reading data from specified register

    // Checking the condition for enabling or disabling based on boolean parameter 'on'
    if (on == true)
    {
        data = data | (1 << 1); // Setting specific bit to enable
    }
    else
    {
        data = data & (~(1 << 1)); // Clearing specific bit to disable
    }
    AS7341_writeReg(REG_AS7341_ENABLE, &data, 1); // Writing modified data to register
}

// Function to enable or disable wait
void AS7341_enableWait(bool on)
{
    uint8_t data; // Variable to store register data
    AS7341_readReg(REG_AS7341_ENABLE, &data, 1); // Reading data from specified register

    // Checking the condition for enabling or disabling based on boolean parameter 'on'
    if (on == true)
    {
        data = data | (1 << 3); // Setting specific bit to enable
    }
    else
    {
        data = data & (~(1 << 3)); // Clearing specific bit to disable
    }
    AS7341_writeReg(REG_AS7341_ENABLE, &data, 1); // Writing modified data to register
}

// Function to enable or disable SMUX
void AS7341_enableSMUX(bool on)
{
    uint8_t data; // Variable to store register data
    AS7341_readReg(REG_AS7341_ENABLE, &data, 1); // Reading data from specified register

    // Checking the condition for enabling or disabling based on boolean parameter 'on'
    if (on == true)
    {
        data = data | (1 << 4); // Setting specific bit to enable
    }
    else
    {
        data = data & (~(1 << 4)); // Clearing specific bit to disable
    }
    AS7341_writeReg(REG_AS7341_ENABLE, &data, 1); // Writing modified data to register
}

// Function to enable or disable flicker detection
void AS7341_enableFlickerDetection(bool on)
{
    uint8_t data; // Variable to store register data
    AS7341_readReg(REG_AS7341_ENABLE, &data, 1); // Reading data from specified register

    // Checking the condition for enabling or disabling based on boolean parameter 'on'
    if (on == true)
    {
        data = data | (1 << 6); // Setting specific bit to enable
    }
    else
    {
        data = data & (~(1 << 6)); // Clearing specific bit to disable
    }
    AS7341_writeReg(REG_AS7341_ENABLE, &data, 1); // Writing modified data to register
}


// Function to configure AS7341 mode
void AS7341_config(AS7341_eMode_t mode)
{
    uint8_t data; // Variable to store register data
    AS7341_setBank(1); // Set the register bank
    AS7341_readReg(REG_AS7341_CONFIG, &data, 1); // Reading data from specified register

    // Switch case to set the mode based on the enumeration value
    switch (mode)
    {
        // Set the mode accordingly
        case eSpm:
        {
            data = (data & (~3)) | eSpm; // Modify specific bits for mode
        };
        break;
        case eSyns:
        {
            data = (data & (~3)) | eSyns; // Modify specific bits for mode
        };
        break;
        case eSynd:
        {
            data = (data & (~3)) | eSynd; // Modify specific bits for mode
        };
        break;
        default:
            break;
    }
    AS7341_writeReg(REG_AS7341_CONFIG, &data, 1); // Writing modified data to register
    AS7341_setBank(0); // Reset the register bank
}

// Function to clear specific spectral data registers related to F1 to F4 and NIR
void AS7341_F1F4_Clear_NIR()
{
    // Direct register writes to clear F1 to F4 and NIR data registers
    AS7341_writeReg_direct(0x00, 0x30);
    AS7341_writeReg_direct(0x01, 0x01);
    AS7341_writeReg_direct(0x02, 0x00);
    AS7341_writeReg_direct(0x03, 0x00);
    AS7341_writeReg_direct(0x04, 0x00);
    AS7341_writeReg_direct(0x05, 0x42);
    AS7341_writeReg_direct(0x06, 0x00);
    AS7341_writeReg_direct(0x07, 0x00);
    AS7341_writeReg_direct(0x08, 0x50);
    AS7341_writeReg_direct(0x09, 0x00);
    AS7341_writeReg_direct(0x0A, 0x00);
    AS7341_writeReg_direct(0x0B, 0x00);
    AS7341_writeReg_direct(0x0C, 0x20);
    AS7341_writeReg_direct(0x0D, 0x04);
    AS7341_writeReg_direct(0x0E, 0x00);
    AS7341_writeReg_direct(0x0F, 0x30);
    AS7341_writeReg_direct(0x10, 0x01);
    AS7341_writeReg_direct(0x11, 0x50);
    AS7341_writeReg_direct(0x12, 0x00);
    AS7341_writeReg_direct(0x13, 0x06);
}

// Function to clear specific spectral data registers related to F5 to F8 and NIR
void AS7341_F5F8_Clear_NIR()
{
    // Direct register writes to clear F5 to F8 and NIR data registers
    AS7341_writeReg_direct(0x00, 0x00);
    AS7341_writeReg_direct(0x01, 0x00);
    AS7341_writeReg_direct(0x02, 0x00);
    AS7341_writeReg_direct(0x03, 0x40);
    AS7341_writeReg_direct(0x04, 0x02);
    AS7341_writeReg_direct(0x05, 0x00);
    AS7341_writeReg_direct(0x06, 0x10);
    AS7341_writeReg_direct(0x07, 0x03);
    AS7341_writeReg_direct(0x08, 0x50);
    AS7341_writeReg_direct(0x09, 0x10);
    AS7341_writeReg_direct(0x0A, 0x03);
    AS7341_writeReg_direct(0x0B, 0x00);
    AS7341_writeReg_direct(0x0C, 0x00);
    AS7341_writeReg_direct(0x0D, 0x00);
    AS7341_writeReg_direct(0x0E, 0x24);
    AS7341_writeReg_direct(0x0F, 0x00);
    AS7341_writeReg_direct(0x10, 0x00);
    AS7341_writeReg_direct(0x11, 0x50);
    AS7341_writeReg_direct(0x12, 0x00);
    AS7341_writeReg_direct(0x13, 0x06);
}

// Function to configure flicker detection specific registers
void AS7341_FDConfig()
{
    // Direct register writes to configure flicker detection specific registers
    AS7341_writeReg_direct(0x00, 0x00);
    AS7341_writeReg_direct(0x01, 0x00);
    AS7341_writeReg_direct(0x02, 0x00);
    AS7341_writeReg_direct(0x03, 0x00);
    AS7341_writeReg_direct(0x04, 0x00);
    AS7341_writeReg_direct(0x05, 0x00);
    AS7341_writeReg_direct(0x06, 0x00);
    AS7341_writeReg_direct(0x07, 0x00);
    AS7341_writeReg_direct(0x08, 0x00);
    AS7341_writeReg_direct(0x09, 0x00);
    AS7341_writeReg_direct(0x0A, 0x00);
    AS7341_writeReg_direct(0x0B, 0x00);
    AS7341_writeReg_direct(0x0C, 0x00);
    AS7341_writeReg_direct(0x0D, 0x00);
    AS7341_writeReg_direct(0x0E, 0x00);
    AS7341_writeReg_direct(0x0F, 0x00);
    AS7341_writeReg_direct(0x10, 0x00);
    AS7341_writeReg_direct(0x11, 0x00);
    AS7341_writeReg_direct(0x12, 0x00);
    AS7341_writeReg_direct(0x13, 0x60);
}

// Function to start spectral measurement based on chosen mode
void AS7341_startMeasure(AS7341_eChChoose_t mode)
{
    uint8_t data = 0; // Variable to hold data value initialized to 0

    // Reading register value from a specific register related to configuration
    AS7341_readReg(REG_AS7341_CFG_0, &data, 1);

    // Unsetting the bit responsible for starting the measurement
    data = data & (~(1 << 4)); 
    AS7341_writeReg(REG_AS7341_CFG_0, &data, 1); // Writing back the modified configuration to the register

    // Disabling spectral measurement temporarily
    AS7341_enableSpectralMeasure(false);

    // Writing a specific value to a particular register
    AS7341_writeReg_direct(0xAF, 0x10);

    // Selecting and executing actions based on the chosen mode (eF1F4ClearNIR or eF5F8ClearNIR)
    if (mode == eF1F4ClearNIR)
        AS7341_F1F4_Clear_NIR(); // Clearing specific spectral data registers related to F1 to F4 and NIR
    else if (mode == eF5F8ClearNIR)
        AS7341_F5F8_Clear_NIR(); // Clearing specific spectral data registers related to F5 to F8 and NIR

    // Enabling spectral multiplexer
    AS7341_enableSMUX(true);

    // Checking and configuring based on the measurement mode
    if (measureMode == eSyns)
    {
        AS7341_setGpioMode(INPUT); // Configuring GPIO mode to INPUT
        AS7341_config(eSyns); // Configuring AS7341 to the eSyns mode
        // AS7341_writeReg(0xA9), 0)); // Potential incomplete commented line
    }
    else if (measureMode == eSpm)
    {
        AS7341_config(eSpm); // Configuring AS7341 to the eSpm mode
    }

    // Enabling spectral measurement again
    AS7341_enableSpectralMeasure(true);

    // Waiting for measurement completion based on the chosen mode
    if (measureMode == eSpm)
    {
        while (!AS7341_measureComplete()) // Looping until the measurement is complete
        {
            busy_wait_ms(1); // Delay for 1 millisecond
        }
    }
}


// Function to read flicker data from the AS7341 sensor
uint8_t AS7341_readFlickerData()
{
    uint8_t flicker; // Variable to store flicker data
    uint8_t data = 0; // Variable to hold data value initialized to 0

    // Reading register value from a specific register related to configuration
    AS7341_readReg(REG_AS7341_CFG_0, &data, 1);

    // Unsetting the bit responsible for starting the measurement
    data = data & (~(1 << 4));
    AS7341_writeReg(REG_AS7341_CFG_0, &data, 1); // Writing back the modified configuration to the register

    // Disabling spectral measurement temporarily
    AS7341_enableSpectralMeasure(false);

    // Writing specific values to certain registers
    AS7341_writeReg_direct(0xAF, 0x10);
    AS7341_FDConfig(); // Configuring flicker detection settings

    // Enabling spectral multiplexer
    AS7341_enableSMUX(true);

    // Enabling spectral measurement
    AS7341_enableSpectralMeasure(true);

    uint8_t retry = 100; // Variable to hold retry count

    // Check if retry count is 0
    if (retry == 0)
        AS7341_debugPrint(" data access error"); // Print an error message if the retry count is 0

    AS7341_enableFlickerDetection(true); // Enable flicker detection
    busy_wait_ms(600); // Wait for 600 milliseconds for flicker detection

    // Reading flicker data from a specific status register
    AS7341_readReg(REG_AS7341_STATUS, &flicker, 1);

    AS7341_enableFlickerDetection(false); // Disable flicker detection

    // Switch statement based on the flicker value read
    switch (flicker)
    {
    case 44:
        flicker = 1; // Set flicker value to 1 if the status is 44
        break;
    case 45:
        flicker = 50; // Set flicker value to 50 if the status is 45
        break;
    case 46:
        flicker = 60; // Set flicker value to 60 if the status is 46
        break;
    default:
        flicker = 0; // Set flicker value to 0 for any other status
    }
    return flicker; // Return the flicker value
}

// Function to check if spectral measurement is complete
bool AS7341_measureComplete()
{
    uint8_t status; // Variable to store status
    AS7341_readReg(REG_AS7341_STATUS_2, &status, 1); // Reading status register value

    // Checking a specific bit in the status register
    if ((status & (1 << 6)))
    {
        return true; // Return true if the bit is set
    }
    else
    {
        return false; // Return false if the bit is not set
    }
}

// Function to retrieve channel data from specified channel
uint16_t AS7341_getChannelData(uint8_t channel)
{
    uint8_t data[2]; // Array to store data read from the sensor
    uint16_t channelData = 0x0000; // Variable to hold channel data initialized to 0

    // Reading low byte data for the specified channel
    AS7341_readReg(REG_AS7341_CH0_DATA_L + channel * 2, data, 1);
    // Reading high byte data for the specified channel
    AS7341_readReg(REG_AS7341_CH0_DATA_H + channel * 2, data + 1, 1);

    // Extracting the channel data by combining high and low bytes
    channelData = data[1];
    channelData = (channelData << 8) | data[0];

    busy_wait_ms(50); // Waiting for 50 milliseconds

    return channelData; // Returning the channel data
}

// Function to read spectral data for Mode 1
AS7341_sModeOneData_t AS7341_readSpectralDataOne()
{
    AS7341_sModeOneData_t data; // Structure to hold spectral data for Mode 1

    // Reading channel data for respective channels and storing in the structure
    data.ADF1 = AS7341_getChannelData(0);
    data.ADF2 = AS7341_getChannelData(1);
    data.ADF3 = AS7341_getChannelData(2);
    data.ADF4 = AS7341_getChannelData(3);
    data.ADCLEAR = AS7341_getChannelData(4);
    data.ADNIR = AS7341_getChannelData(5);

    return data; // Returning the spectral data structure
}


// Function to read spectral data for Mode 2
AS7341_sModeTwoData_t AS7341_readSpectralDataTwo()
{
    AS7341_sModeTwoData_t data; // Structure to hold spectral data for Mode 2

    // Reading channel data for respective channels and storing in the structure
    data.ADF5 = AS7341_getChannelData(0);
    data.ADF6 = AS7341_getChannelData(1);
    data.ADF7 = AS7341_getChannelData(2);
    data.ADF8 = AS7341_getChannelData(3);
    data.ADCLEAR = AS7341_getChannelData(4);
    data.ADNIR = AS7341_getChannelData(5);

    return data; // Returning the spectral data structure
}

// Function to set GPIO pin connectivity
void AS7341_setGpio(bool connect)
{
    uint8_t data; // Variable to store register data

    AS7341_readReg(REG_AS7341_CPIO, &data, 1); // Reading GPIO register

    if (connect == true) // Checking if connecting GPIO pin is requested
    {
        data = data | (1 << 0); // Setting the respective bit for pin connection
    }
    else // If disconnecting GPIO pin is requested
    {
        data = data & (~(1 << 0)); // Clearing the respective bit for pin disconnection
    }

    AS7341_writeReg(REG_AS7341_CPIO, &data, 1); // Writing modified data to GPIO register
}

// Function to set GPIO pin mode (INPUT or OUTPUT)
void AS7341_setGpioMode(uint8_t mode)
{
    uint8_t data; // Variable to store register data

    AS7341_readReg(REG_AS7341_GPIO_2, &data, 1); // Reading GPIO configuration register

    if (mode == INPUT) // Checking if the mode is set to INPUT
    {
        data = data | (1 << 2); // Setting the respective bit for INPUT mode
    }

    if (mode == OUTPUT) // Checking if the mode is set to OUTPUT
    {
        data = data & (~(1 << 2)); // Clearing the respective bit for OUTPUT mode
    }

    AS7341_writeReg(REG_AS7341_GPIO_2, &data, 1); // Writing modified data to GPIO configuration register
}


// Function to enable or disable the LED
void AS7341_enableLed(bool on)
{
    uint8_t data = 0; // Variable to store register data
    uint8_t data1 = 0; // Variable to store another register data
    AS7341_setBank(1); // Setting bank to access specific registers

    // Reading configuration registers related to LED
    AS7341_readReg(REG_AS7341_CONFIG, &data, 1);
    AS7341_readReg(REG_AS7341_LED, &data1, 1);

    if (on == true) // Checking if LED should be turned on
    {
        data = data | (1 << 3); // Setting bit 3 to turn on the LED
        data1 = data1 | (1 << 7); // Setting bit 7 to turn on the LED
    }
    else // If LED should be turned off
    {
        data = data & (~(1 << 3)); // Clearing bit 3 to turn off the LED
        data1 = data1 & (~(1 << 7)); // Clearing bit 7 to turn off the LED
    }

    AS7341_writeReg(REG_AS7341_CONFIG, &data, 1); // Writing modified data to the configuration register
    AS7341_writeReg(REG_AS7341_LED, &data1, 1); // Writing modified data to the LED register
    AS7341_setBank(0); // Resetting the bank
}

// Function to set the bank for accessing specific registers
void AS7341_setBank(uint8_t addr)
{
    uint8_t data = 0; // Variable to store register data
    AS7341_readReg(REG_AS7341_CFG_0, &data, 1); // Reading configuration register

    if (addr == 1) // If address is 1, set the bank
    {
        data = data | (1 << 4); // Setting bit 4 to select the bank
    }

    if (addr == 0) // If address is 0, clear the bank
    {
        data = data & (~(1 << 4)); // Clearing bit 4 to deselect the bank
    }

    AS7341_writeReg(REG_AS7341_CFG_0, &data, 1); // Writing modified data to the configuration register
}

// Function to control the LED current
void AS7341_controlLed(uint8_t current)
{
    uint8_t data = 0; // Variable to store register data

    if (current < 1)
        current = 1;
    current--;

    if (current > 19)
        current = 19;

    AS7341_setBank(1); // Setting bank to access specific registers

    data = 0;
    // AS7341_readReg(REG_AS7341_LED,&data,1);
    data = data | (1 << 7); // Setting bit 7 to control LED current
    data = data | (current & 0x7f); // Setting the LED current value
    AS7341_writeReg(REG_AS7341_LED, &data, 1); // Writing modified data to the LED register
    busy_wait_ms(100); // Delay
    AS7341_setBank(0); // Resetting the bank
}

// Function to set interrupt
void AS7341_setInt(bool connect)
{
    uint8_t data; // Variable to store register data

    AS7341_readReg(REG_AS7341_CPIO, &data, 1); // Reading CPIO register

    if (connect == true) // If connection is requested
    {
        data = data | (1 << 1); // Setting bit 1 for interrupt
    }
    else // If disconnection is requested
    {
        data = data & (~(1 << 1)); // Clearing bit 1 for interrupt
    }

    AS7341_writeReg(REG_AS7341_CPIO, &data, 1); // Writing modified data to CPIO register
}


// Function to enable or disable system interrupt
void AS7341_enableSysInt(bool on)
{
    uint8_t data; // Variable to store register data
    AS7341_readReg(REG_AS7341_INTENAB, &data, 1); // Reading interrupt enable register

    if (on == true) // If system interrupt should be enabled
    {
        data = data | (1 << 0); // Setting bit 0 to enable system interrupt
    }
    else // If system interrupt should be disabled
    {
        data = data & (~(1 << 0)); // Clearing bit 0 to disable system interrupt
    }

    AS7341_writeReg(REG_AS7341_INTENAB, &data, 1); // Writing modified data to interrupt enable register
}

// Function to enable or disable FIFO interrupt
void AS7341_enableFIFOInt(bool on)
{
    uint8_t data; // Variable to store register data
    AS7341_readReg(REG_AS7341_INTENAB, &data, 1); // Reading interrupt enable register

    if (on == true) // If FIFO interrupt should be enabled
    {
        data = data | (1 << 2); // Setting bit 2 to enable FIFO interrupt
    }
    else // If FIFO interrupt should be disabled
    {
        data = data & (~(1 << 2)); // Clearing bit 2 to disable FIFO interrupt
    }

    AS7341_writeReg(REG_AS7341_INTENAB, &data, 1); // Writing modified data to interrupt enable register
}

// Function to enable or disable spectral interrupt
void AS7341_enableSpectralInt(bool on)
{
    uint8_t data; // Variable to store register data
    AS7341_readReg(REG_AS7341_INTENAB, &data, 1); // Reading interrupt enable register

    if (on == true) // If spectral interrupt should be enabled
    {
        data = data | (1 << 3); // Setting bit 3 to enable spectral interrupt
    }
    else // If spectral interrupt should be disabled
    {
        data = data & (~(1 << 3)); // Clearing bit 3 to disable spectral interrupt
    }

    AS7341_writeReg(REG_AS7341_INTENAB, &data, 1); // Writing modified data to interrupt enable register
}

// Function to end sleep mode
void AS7341_endSleep()
{
    uint8_t data; // Variable to store register data
    AS7341_readReg(REG_AS7341_INTENAB, &data, 1); // Reading interrupt enable register
    data = data | (1 << 3); // Setting bit 3 to end sleep mode
    AS7341_writeReg(REG_AS7341_INTENAB, &data, 1); // Writing modified data to interrupt enable register
}

// Function to clear FIFO
void AS7341_clearFIFO()
{
    uint8_t data; // Variable to store register data
    AS7341_readReg(REG_AS7341_CONTROL, &data, 1); // Reading control register
    data = data | (1 << 0); // Setting bit 0 to clear FIFO
    data = data & (~(1 << 0)); // Clearing bit 0 to finish clearing FIFO
    AS7341_writeReg(REG_AS7341_CONTROL, &data, 1); // Writing modified data to control register
}

// Function to perform spectral auto-zero
void AS7341_spectralAutozero()
{
    uint8_t data; // Variable to store register data
    AS7341_readReg(REG_AS7341_CONTROL, &data, 1); // Reading control register
    data = data | (1 << 1); // Setting bit 1 to perform spectral auto-zero
    AS7341_writeReg(REG_AS7341_CONTROL, &data, 1); // Writing modified data to control register
}

// Function to enable or disable flicker interrupt
void AS7341_enableFlickerInt(bool on)
{
    uint8_t data; // Variable to store register data
    AS7341_readReg(REG_AS7341_INTENAB, &data, 1); // Reading interrupt enable register

    if (on == true) // If flicker interrupt should be enabled
    {
        data = data | (1 << 2); // Setting bit 2 to enable flicker interrupt
    }
    else // If flicker interrupt should be disabled
    {
        data = data & (~(1 << 2)); // Clearing bit 2 to disable flicker interrupt
    }

    AS7341_writeReg(REG_AS7341_INTENAB, &data, 1); // Writing modified data to interrupt enable register
}

// Function to set the integration time
void AS7341_setAtime(uint8_t value)
{
    AS7341_writeReg(REG_AS7341_ATIME, &value, 1); // Writing integration time value to the respective register
}

// Function to set the analog gain
void AS7341_setAGAIN(uint8_t value)
{
    if (value > 10)
        value = 10; // Limiting the maximum analog gain value to 10
    AS7341_writeReg(REG_AS7341_CFG_1, &value, 1); // Writing analog gain value to the respective register
}

// Function to set the analog step
void AS7341_setAstep(uint16_t value)
{
    uint8_t highValue, lowValue;
    lowValue = value & 0x00ff; // Extracting lower 8 bits of the value
    highValue = value >> 8; // Extracting upper 8 bits of the value
    AS7341_writeReg(REG_AS7341_ASTEP_L, &lowValue, 1); // Writing lower bits of analog step
    AS7341_writeReg(REG_AS7341_ASTEP_H, &highValue, 1); // Writing higher bits of analog step
}


// Function to retrieve the integration time
float AS7341_getIntegrationTime()
{
    uint8_t data; // Variable to store register data
    uint8_t astepData[2] = {0}; // Array to store analog step data
    uint16_t astep; // Variable to store the analog step value

    AS7341_readReg(REG_AS7341_ATIME, &data, 1); // Reading integration time register
    AS7341_readReg(REG_AS7341_ASTEP_L, astepData, 1); // Reading lower bits of analog step
    AS7341_readReg(REG_AS7341_ASTEP_L, astepData + 1, 1); // Reading higher bits of analog step
    astep = astepData[1]; // Storing higher bits of analog step
    astep = (astep << 8) | astepData[0]; // Combining lower and higher bits of analog step

    return 0; // Returning default value (needs to be modified to return actual integration time)
}

// Function to set the wait time
void AS7341_setWtime(uint8_t value)
{
    AS7341_writeReg(REG_AS7341_WTIME, &value, 1); // Writing wait time value to respective register
}

// Function to retrieve the wait time
float AS7341_getWtime()
{
    float value; // Variable to store the wait time value
    uint8_t data; // Variable to store register data

    AS7341_readReg(REG_AS7341_WTIME, &data, 1); // Reading wait time register

    // Calculating the corresponding wait time based on the register value
    if (data == 0)
    {
        value = 2.78;
    }
    else if (data == 1)
    {
        value = 5.56;
    }
    else if (data > 1 && data < 255)
    {
        value = 2.78 * (data + 1);
    }
    else if (data == 255)
    {
        value = 711;
    }

    return value; // Returning the calculated wait time value
}

// Function to set the spectral threshold
void AS7341_setThreshold(uint16_t lowTh, uint16_t highTh)
{
    if (lowTh >= highTh)
        return;

    // Writing low and high spectral thresholds to respective registers
    AS7341_writeReg_direct(REG_AS7341_SP_TH_H_MSB, highTh >> 8);
    AS7341_writeReg_direct(REG_AS7341_SP_TH_H_LSB, highTh);

    AS7341_writeReg_direct(REG_AS7341_SP_TH_L_MSB, lowTh >> 8);
    AS7341_writeReg_direct(REG_AS7341_SP_TH_L_LSB, lowTh);

    busy_wait_ms(10); // Waiting for some time
}

// Function to retrieve the low spectral threshold
uint16_t AS7341_getLowThreshold()
{
    uint16_t data; // Variable to store the low spectral threshold
    data = AS7341_readReg_direct(REG_AS7341_SP_TH_L_MSB); // Reading MSB of low threshold
    data = AS7341_readReg_direct(REG_AS7341_SP_TH_L_LSB) | (data << 8); // Reading LSB and combining to get the full value
    return data; // Returning the low spectral threshold value
}

// Function to retrieve the high spectral threshold
uint16_t AS7341_getHighThreshold()
{
    uint16_t data; // Variable to store the high spectral threshold
    data = AS7341_readReg_direct(REG_AS7341_SP_TH_H_MSB); // Reading MSB of high threshold
    data = AS7341_readReg_direct(REG_AS7341_SP_TH_H_LSB) | (data << 8); // Reading LSB and combining to get the full value
    return data; // Returning the high spectral threshold value
}

// Function to clear the interrupt status
void AS7341_clearInterrupt()
{
    AS7341_writeReg_direct(REG_AS7341_STATUS_1, 0xff); // Writing to clear interrupt status
}

// Function to enable or disable spectral interrupt
void AS7341_enableSpectralInterrupt(bool on)
{
    uint8_t data; // Variable to store register data
    data = AS7341_readReg_direct(REG_AS7341_INTENAB); // Reading interrupt enable register

    if (on == true) // If spectral interrupt should be enabled
    {
        data = data | (1 << 3); // Setting bit 3 to enable spectral interrupt
        AS7341_writeReg_direct(REG_AS7341_INTENAB, data); // Writing modified data to interrupt enable register
    }
    else // If spectral interrupt should be disabled
    {
        data = data & (~(1 << 3)); // Clearing bit 3 to disable spectral interrupt
        AS7341_writeReg_direct(REG_AS7341_INTENAB, data); // Writing modified data to interrupt enable register
    }
}

// Function to set the interrupt channel
void AS7341_setIntChannel(uint8_t channel)
{
    if (channel >= 5)
        return;

    uint8_t data; // Variable to store register data
    data = AS7341_readReg_direct(REG_AS7341_CFG_12); // Reading configuration register 12
    data = data & (~(7)); // Clearing the lower 3 bits
    data = data | (channel); // Setting the interrupt channel
    AS7341_writeReg_direct(REG_AS7341_CFG_12, data); // Writing modified data to configuration register 12
}


// Function to set the number of consecutive measurements needed to trigger an interrupt
void AS7341_setAPERS(uint8_t num)
{
    uint8_t data; // Variable to store register data
    data = AS7341_readReg_direct(REG_AS7341_PERS); // Reading the persistence register
    data = data & (~(15)); // Clearing the lower 4 bits
    data = data | (num); // Setting the number of measurements
    AS7341_writeReg_direct(REG_AS7341_PERS, data); // Writing modified data to the persistence register
}

// Function to get the interrupt source
uint8_t AS7341_getIntSource()
{
    return AS7341_readReg_direct(REG_AS7341_STATUS_3); // Reading interrupt source from status register 3
}

// Function to check if an interrupt has occurred
bool AS7341_interrupt()
{
    uint8_t data = AS7341_readReg_direct(REG_AS7341_STATUS_1); // Reading status register 1
    if (data & 0x80) // Checking if the interrupt bit is set
    {
        return true; // Returning true if interrupt bit is set
    }
    else
    {
        return false; // Returning false if interrupt bit is not set
    }
}

// Function to check the validity of WTIME
bool AS7341_checkWtime()
{
    uint8_t status; // Variable to store status data
    uint8_t value; // Variable to store specific value

    AS7341_readReg(REG_AS7341_STATUS_6, &status, 1); // Reading status 6 register
    value = status & (1 << 2); // Extracting specific bit (WTIME status)

    if (value > 0) // Checking if WTIME bit is set
    {
        return false; // Returning false if WTIME bit is set
    }
    else
    {
        return true; // Returning true if WTIME bit is not set
    }
}
