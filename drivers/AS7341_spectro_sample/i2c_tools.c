/** @file i2c_tools.c
 *
 * @brief This file contains the source code for the I2C tools library.
 * Brief overview of the code:
 * An all in one i2c library for the Raspberry Pi Pico, designed to be a drop-in replacement for the Arduino Wire library.
 * Based on the arduino-pico wire library written by Earle F. Philhower, III (@earlephilhower) https://github.com/earlephilhower/arduino-pico
 *
 * Do note that only the master mode functionalities have been ported, while the slave mode functionalities have been left out.
 * As for our project's purposes, we only need the master mode functionalities.
 *
 * This library aims to provide a C version, drop-in replacement for the Arduino Wire library.
 * Designed to be as close to the original Arduino API as possible, this will allow existing I2C code written for Arduino to work on the Raspberry Pi Pico.
 * Without the need to rewrite everything from scratch (some rewriting is still required, but nothing too major)
 *
 * Do note that as the Arduino Wire library is written in C++, some of the functionalities have been changed to fit the C language.
 * And because C does not support function overloading, some of the functions have been renamed to avoid conflicts.
 *
 * To make things simpler, any code that is called in the I2C library will be prefixed with "i2c_tools_".
 * e.g. _pWire = TwoWire(0) will become i2c_tools_init(i2c0, PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN) instead.
 * _pWire->begin() will become i2c_tools_begin() instead.
 * and _pWire->beginTransmission() will become i2c_tools_beginTransmission() instead.
 * etc, etc. (see the example ported I2C libraries for more details)
 *
 * Additionally, the following functions have also been ported from the Arduino wiring API:
 * 1. digitalWrite()
 * 2. digitalRead()
 * 3. pinMode()
 */

#include <stdio.h>
#include <pico/stdlib.h>
#include <hardware/gpio.h>
#include <hardware/i2c.h>
#include <hardware/irq.h>
#include <hardware/regs/intctrl.h>
#include "i2c_tools.h"

// Timeout value for I2C operations in milliseconds.
static int _timeout = 500;

// I2C instance pointer.
static i2c_inst_t *_i2c;

// GPIO pin for the I2C data line.
static int _sda;

// GPIO pin for the I2C clock line.
static int _scl;

// Clock frequency for the I2C communication.
static int _clkHz;

// Flag indicating whether I2C is currently running/initialized.
static bool _running;

// Flag indicating whether the I2C instance is in slave mode. (Not in use, but kept for compatibility)
static bool _slave;

// 7-bit I2C address for the target device in master mode.
static uint8_t _addr;

// Flag indicating whether a transmission session is in progress.
static bool _txBegun;

// Internal buffer for storing data during I2C operations.
static uint8_t _buff[WIRE_BUFFER_SIZE];

// Length of data in the internal buffer.
static int _buffLen;

// Offset within the internal buffer.
static int _buffOff;

// Default TWI (Two-Wire Interface) clock frequency.
static const uint32_t TWI_CLOCK = 100000;

// Array to store the pin mode for each GPIO pin.
static PinMode _pm[30];

#pragma region GPIO function calls (from wiring_digital.c)

/**
 * @brief Sets the digital state of a GPIO pin.
 *
 * This function mimics the behavior of Arduino's digitalWrite function.
 * It configures the pin mode and sets the output value accordingly.
 *
 * @param pin The GPIO pin number.
 * @param val The value to set: HIGH (1) or LOW (0).
 */
void digitalWrite(int pin, int val)
{
    // Set the GPIO function to General Purpose I/O.
    gpio_set_function(pin, GPIO_FUNC_SIO);

    // Check if the pin has INPUT_PULLDOWN mode.
    if (_pm[pin] == INPUT_PULLDOWN)
    {
        // Set the pin direction based on the value.
        gpio_set_dir(pin, (val == LOW) ? false : true);
    }
    // Check if the pin has INPUT_PULLUP mode.
    else if (_pm[pin] == INPUT_PULLUP)
    {
        // Set the pin direction based on the value.
        gpio_set_dir(pin, (val == HIGH) ? false : true);
    }
    // For other modes, directly set the output value.
    else
    {
        gpio_put(pin, (val == LOW) ? 0 : 1);
    }
}

/**
 * @brief Reads the digital state of a GPIO pin.
 *
 * This function mimics the behavior of Arduino's digitalRead function.
 * It retrieves the digital state of the specified GPIO pin.
 *
 * @param pin The GPIO pin number.
 * @return The digital state of the pin: HIGH (1) or LOW (0).
 */
bool digitalRead(int pin)
{
    // Retrieve the digital state of the GPIO pin and convert it to HIGH or LOW.
    return gpio_get(pin) ? HIGH : LOW;
}
/**
 * @brief Configures the mode of a GPIO pin.
 *
 * This function sets the mode of the specified GPIO pin, such as INPUT, OUTPUT, or INPUT_PULLUP.
 *
 * @param pin The GPIO pin number.
 * @param mode The mode to set for the pin: INPUT, INPUT_PULLUP, INPUT_PULLDOWN, OUTPUT, OUTPUT_2MA, OUTPUT_4MA, OUTPUT_8MA, OUTPUT_12MA.
 */
void pinMode(int pin, int mode)
{
    switch (mode)
    {

    case INPUT:
        // Configure the pin as INPUT with no pull-up or pull-down.
        gpio_init(pin);
        gpio_set_dir(pin, false);
        gpio_disable_pulls(pin);
        break;

    case INPUT_PULLUP:
        // Configure the pin as INPUT with pull-up resistor.
        gpio_init(pin);
        gpio_set_dir(pin, false);
        gpio_pull_up(pin);
        gpio_put(pin, 0);
        break;
    case INPUT_PULLDOWN:
        // Configure the pin as INPUT with pull-down resistor.
        gpio_init(pin);
        gpio_set_dir(pin, false);
        gpio_pull_down(pin);
        gpio_put(pin, 1);
        break;
    case OUTPUT: // Configure the pin as OUTPUT with specified drive strength.
    case OUTPUT_4MA:
        gpio_init(pin);
        gpio_set_drive_strength(pin, GPIO_DRIVE_STRENGTH_4MA);
        gpio_set_dir(pin, true);
        break;
    case OUTPUT_2MA:
        gpio_init(pin);
        gpio_set_drive_strength(pin, GPIO_DRIVE_STRENGTH_2MA);
        gpio_set_dir(pin, true);
        break;
    case OUTPUT_8MA:
        gpio_init(pin);
        gpio_set_drive_strength(pin, GPIO_DRIVE_STRENGTH_8MA);
        gpio_set_dir(pin, true);
        break;
    case OUTPUT_12MA:
        gpio_init(pin);
        gpio_set_drive_strength(pin, GPIO_DRIVE_STRENGTH_12MA);
        gpio_set_dir(pin, true);
        break;
    }
    // Store the pin mode for future reference.
    _pm[pin] = mode;
}

#pragma endregion

#pragma region I2C init and deinit functions

/**
 * @brief Initializes I2C tools with the specified parameters.
 *
 * This function initializes the I2C tools with the provided I2C instance, SDA and SCL pins, and default clock frequency.
 *
 * @param i2c Pointer to the I2C instance.
 * @param sda The pin number for the I2C SDA (data) line.
 * @param scl The pin number for the I2C SCL (clock) line.
 */
void i2c_tools_init(i2c_inst_t *i2c, int sda, int scl)
{
    // Assign parameters to internal variables.
    _sda = sda;
    _scl = scl;
    _i2c = i2c;
    _clkHz = TWI_CLOCK;
    _running = false;
    _txBegun = false;
    _buffLen = 0;
}

/**
 * @brief Sets the I2C SDA (data) pin.
 *
 * This function sets the I2C SDA pin to the specified pin number if not currently running.
 *
 * @param pin The pin number to set as the I2C SDA pin.
 * @return True if the pin is successfully set or is already set; False otherwise.
 */
bool i2c_tools_setSDA(int pin)
{
    // Check if the provided pin is the same as the current SDA pin.
    if (_sda == pin)
    {
        // Return true as the pin is already set.
        return true;
    }
    // Check if I2C is not currently running.
    if (!_running)
    {
        // Set the SDA pin and return true.
        _sda = pin;
        return true;
    }
    else
    {
        panic("FATAL: Attempting to set Wire%s.SDA while running", i2c_hw_index(_i2c) ? "1" : "");
    }

    // Return false if setting the pin fails.
    return false;
}

/**
 * @brief Sets the I2C SCL (clock) pin.
 *
 * This function sets the I2C SCL pin to the specified pin number if not currently running.
 *
 * @param pin The pin number to set as the I2C SCL pin.
 * @return True if the pin is successfully set or is already set; False otherwise.
 */
bool i2c_tools_setSCL(int pin)
{
    // Check if the provided pin is the same as the current SCL pin.
    if (_scl == pin)
    {
        // Return true as the pin is already set.
        return true;
    }
    // Check if I2C is not currently running.
    if (!_running)
    {
        // Set the SCL pin and return true.
        _scl = pin;
        return true;
    }
    else
    {
        panic("FATAL: Attempting to set Wire%s.SCL while running", i2c_hw_index(_i2c) ? "1" : "");
    }

    // Return false if setting the pin fails.
    return false;
}

/**
 * @brief Sets the clock frequency for I2C communication.
 *
 * This function sets the clock frequency for I2C communication.
 * If I2C is currently running, it updates the baud rate accordingly.
 *
 * @param hz The desired clock frequency in Hertz.
 */
void i2c_tools_setClock(uint32_t hz)
{
    // Set the internal clock frequency variable.
    _clkHz = hz;

    // Check if I2C is currently running.
    if (_running)
    {
        // Update the I2C baud rate with the new clock frequency.
        i2c_set_baudrate(_i2c, hz);
    }
}

/**
 * @brief Initializes I2C communication.
 *
 * This function initializes I2C communication, configuring the I2C instance, pins, and related settings.
 * If I2C is already running, it returns without doing anything further.
 */
void i2c_tools_begin()
{
    // Check if I2C is already running.
    if (_running)
    {
        // Returns if I2C has already been initialized.
        return;
    }

    // Set the I2C mode to master.
    _slave = false;

    // Initialize the I2C instance with the specified clock frequency.
    i2c_init(_i2c, _clkHz);

    // Set I2C to master mode with no address.
    i2c_set_slave_mode(_i2c, false, 0);

    // Configure SDA pin for I2C and enable pull-up resistor.
    gpio_set_function(_sda, GPIO_FUNC_I2C);
    gpio_pull_up(_sda);

    // Configure SCL pin for I2C and enable pull-up resistor.
    gpio_set_function(_scl, GPIO_FUNC_I2C);
    gpio_pull_up(_scl);

    // Set internal flags to indicate that I2C is now running.
    _running = true;
    _txBegun = false;
    _buffLen = 0;
}

/**
 * @brief Ends I2C communication.
 *
 * This function ends I2C communication, deinitializing the I2C instance and setting pins to INPUT mode.
 * If I2C is not currently running, it returns without doing anything further.
 */
void i2c_tools_end()
{
    // Check if I2C is not currently running.
    if (!_running)
    {
        // Returns if I2C is not running.
        return;
    }

    // Deinitialize the I2C instance.
    i2c_deinit(_i2c);

    // Set SDA pin to INPUT mode.
    pinMode(_sda, INPUT);

    // Set SCL pin to INPUT mode.
    pinMode(_scl, INPUT);

    // Set internal flags to indicate that I2C is no longer running.
    _running = false;
    _txBegun = false;
}

#pragma endregion

#pragma region I2C transmission functions
/**
 * @brief Starts a new transmission session to the specified address.
 *
 * This function prepares the library to start a transmission session to the specified address.
 * If I2C is not currently running or transmission session is already in progress, it returns without doing anything further.
 *
 * @param addr The I2C address of the target device.
 */
void i2c_tools_beginTransmission(uint8_t addr)
{
    // Check if I2C is not currently running or transmission has already begun.
    if (!_running || _txBegun)
    {
        // Returns without doing anything further if I2C is not running or a transmission is already in progress.
        return;
    }

    // Set the target device address for transmission.
    _addr = addr;

    // Reset buffer length and offset.
    _buffLen = 0;
    _buffOff = 0;

    // Set internal flag to indicate that a transmission session is in progress.
    _txBegun = true;
}

/**
 * @brief Requests data from an I2C device with an optional stop bit.
 *
 * This function requests data from an I2C device with the specified address.
 * It reads the specified quantity of bytes into the internal buffer, blocking until the specified absolute time is reached.
 *
 * @param address The I2C address of the target device.
 * @param quantity The number of bytes to request and read.
 * @param stopBit Indicates whether to send a stop bit after the request.
 * @return The number of bytes read, or 0 if there was an error or invalid parameters.
 */
size_t i2c_tools_requestFrom_w_stopbit(uint8_t address, size_t quantity, bool stopBit)
{
    // Check if I2C is not currently running, a transmission has already begun, quantity is zero, or quantity exceeds buffer size.
    if (!_running || _txBegun || !quantity || (quantity > sizeof(_buff)))
    {
        // Return 0 if there is an error or invalid parameters.
        return 0;
    }

    // Perform I2C read with optional stop bit, blocking until specified absolute time is reached.
    _buffLen = i2c_read_blocking_until(_i2c, address, _buff, quantity, !stopBit, make_timeout_time_ms(_timeout));

    // Check for read errors or timeout.
    if ((_buffLen == PICO_ERROR_GENERIC) || (_buffLen == PICO_ERROR_TIMEOUT))
    {
        // Reset buffer length to 0 in case of errors.
        _buffLen = 0;
    }

    // Reset buffer offset for subsequent reads.
    _buffOff = 0;

    // Return the number of bytes read.
    return _buffLen;
}

/**
 * @brief Requests data from an I2C device with a stop bit.
 *
 * This function is a wrapper for i2c_tools_requestFrom_w_stopbit with the stopBit parameter set to true.
 *
 * @param address The I2C address of the target device.
 * @param quantity The number of bytes to request and read.
 * @return The number of bytes read, or 0 if there was an error or invalid parameters.
 */
size_t i2c_tools_requestFrom(uint8_t address, size_t quantity)
{
    // Call the i2c_tools_requestFrom_w_stopbit function with stopBit parameter set to true.
    return i2c_tools_requestFrom_w_stopbit(address, quantity, true);
}

/**
 * @brief Implements clock stretching for I2C communication.
 *
 * This function performs clock stretching by waiting for the specified pin to go HIGH within a time limit.
 *
 * @param pin The GPIO pin used for clock stretching.
 * @return True if the pin goes HIGH within the time limit; False otherwise.
 */
static bool _clockStretch(int pin)
{
    // Calculate the absolute end time for the clock stretching period (100 microseconds).
    uint64_t end = time_us_64() + 100;

    // Wait until either the specified pin goes HIGH or the time limit is reached.
    while ((time_us_64() < end) && (!digitalRead(pin)))
    {
        // No operation (noop) during the wait.
    }

    // Return the digital state of the pin after the clock stretching period.
    return digitalRead(pin);
}
/**
 * @brief Probes an I2C device at the specified address using software I2C.
 *
 * This function probes an I2C device at the specified address using software I2C.
 * It checks for the presence of the device and returns true if an acknowledgment is received.
 *
 * @param addr The I2C address of the device to probe.
 * @param sda The GPIO pin used for the I2C data line.
 * @param scl The GPIO pin used for the I2C clock line.
 * @param freq The desired frequency for the software I2C communication.
 * @return True if the device acknowledges the address; False otherwise.
 */
static bool _probe(int addr, int sda, int scl, int freq)
{
    // Calculate half of the delay based on the desired frequency.
    int delay = (1000000 / freq) / 2;
    bool ack = false;

    // Set up GPIO pins and functions
    pinMode(sda, INPUT_PULLUP);
    pinMode(scl, INPUT_PULLUP);
    gpio_set_function(scl, GPIO_FUNC_SIO);
    gpio_set_function(sda, GPIO_FUNC_SIO);

    // Initialize the start condition.
    digitalWrite(sda, HIGH);
    sleep_us(delay);
    digitalWrite(scl, HIGH);

    // Check for clock stretching during the start condition.
    if (!_clockStretch(scl))
    {
        goto stop;
    }

    digitalWrite(sda, LOW);
    sleep_us(delay);
    digitalWrite(scl, LOW);
    sleep_us(delay);

    // Send the 7-bit I2C address with MSB first.
    for (int i = 0; i < 8; i++)
    {
        addr <<= 1;
        digitalWrite(sda, (addr & (1 << 7)) ? HIGH : LOW);
        sleep_us(delay);
        digitalWrite(scl, HIGH);
        sleep_us(delay);

        // Check for clock stretching during data transmission.
        if (!_clockStretch(scl))
        {
            goto stop;
        }

        digitalWrite(scl, LOW);
        sleep_us(5); // Ensure we don't change too close to the clock edge.
    }

    // Release the data line and check for acknowledgment.
    digitalWrite(sda, HIGH);
    sleep_us(delay);
    digitalWrite(scl, HIGH);

    // Check for clock stretching during acknowledgment.
    if (!_clockStretch(scl))
    {
        goto stop;
    }

    // Check if the device acknowledges the address.
    ack = digitalRead(sda) == LOW;
    sleep_us(delay);
    digitalWrite(scl, LOW);

stop:
    // Perform stop condition.
    sleep_us(delay);
    digitalWrite(sda, LOW);
    sleep_us(delay);
    digitalWrite(scl, HIGH);
    sleep_us(delay);
    digitalWrite(sda, HIGH);
    sleep_us(delay);

    // Restore GPIO functions to I2C mode.
    gpio_set_function(scl, GPIO_FUNC_I2C);
    gpio_set_function(sda, GPIO_FUNC_I2C);

    // Return true if acknowledgment is received; False otherwise.
    return ack;
}

/**
 * @brief Sends the data in the internal buffer to the target device.
 *
 * This function sends the data in the internal buffer to the target device.
 * And then sets the internal session flag to indicate that the transmission has ended.
 *
 * @param stopBit If false, master retains control of the bus at the end of the transfer (no Stop is issued),
 *           and the next transfer will begin with a Restart rather than a Start.
 * @return Error code:
 *         - 0: Success
 *         - 1: Data too long
 *         - 2: NACK on transmit of address
 *         - 3: NACK on transmit of data
 *         - 4: Other error
 */
uint8_t i2c_tools_endTransmission_w_stopbit(bool stopBit)
{
    // Check if I2C is not currently running or transmission has not begun.
    if (!_running || !_txBegun)
    {
        // Return 4 for "Other error" if I2C is not running or transmission has not begun.
        return 4;
    }

    // Reset the transmission flag.
    _txBegun = false;

    // Check for special case of 0-length writes used for I2C probing.
    if (!_buffLen)
    {
        // Probe the I2C device at the specified address.
        return _probe(_addr, _sda, _scl, _clkHz) ? 0 : 2;
    }
    else
    {
        // Save the length of the buffer.
        int len = _buffLen;

        // Perform I2C write with optional stop bit, blocking until specified timeout.
        int ret = i2c_write_blocking_until(_i2c, _addr, _buff, _buffLen, !stopBit, make_timeout_time_ms(_timeout));

        // Reset the buffer length.
        _buffLen = 0;

        // Return 0 for success, 4 for other errors.
        return (ret == len) ? 0 : 4;
    }
}

/**
 * @brief Sends the data in the internal buffer to the target device.
 *
 * This function sends the data in the internal buffer to the target device.
 * And then sets the internal session flag to indicate that the transmission has ended.
 * This function is a wrapper for i2c_tools_endTransmission_w_stopbit with the stopBit parameter set to true.
 *
 * @return Error code:
 *         - 0: Success
 *         - 1: Data too long
 *         - 2: NACK on transmit of address
 *         - 3: NACK on transmit of data
 *         - 4: Other error
 */
uint8_t i2c_tools_endTransmission()
{
    // Call i2c_tools_endTransmission_w_stopbit with stopBit parameter set to true.
    return i2c_tools_endTransmission_w_stopbit(true);
}

/**
 * @brief Writes a byte to the I2C device.
 *
 * This function writes a byte to the I2C device.
 * It adds the byte to the internal buffer if transmission has begun and the buffer is not full.
 *
 * @param ucData The byte to be written to the I2C device.
 * @return Number of bytes written (1 if successful, 0 otherwise).
 */
size_t i2c_tools_write(uint8_t ucData)
{
    // Check if I2C is not currently running.
    if (!_running)
    {
        // Return 0 if I2C is not running.
        return 0;
    }

    // Check if I2C is in slave mode.
    if (_slave)
    {
        // In slave mode, wait for a spot in the TX FIFO.
        while (0 == (_i2c->hw->status & (1 << 1)))
        {
            // No operation (noop) while waiting for a spot in the TX FIFO.
        }
        // Send the byte to the I2C device.
        _i2c->hw->data_cmd = ucData;
        return 1;
    }
    else
    {
        // In master mode, check if transmission has begun or the buffer is full.
        if (!_txBegun || (_buffLen == sizeof(_buff)))
        {
            // Return 0 if transmission has not begun or the buffer is full.
            return 0;
        }
        // Add the byte to the internal buffer.
        _buff[_buffLen++] = ucData;
        return 1;
    }
}

/**
 * @brief Writes multiple bytes to the I2C device.
 *
 * This function writes an array of bytes to the I2C device using i2c_tools_write for each byte.
 * It returns the number of bytes successfully written before encountering an error or reaching the end of the array.
 *
 * @param data Pointer to the array of bytes to be written.
 * @param quantity The number of bytes to write.
 * @return Number of bytes successfully written.
 */
size_t i2c_tools_write_w_quantity(const uint8_t *data, size_t quantity)
{
    // Iterate through the array of bytes.
    for (size_t i = 0; i < quantity; ++i)
    {
        // Call i2c_tools_write for each byte.
        if (!i2c_tools_write(data[i]))
        {
            // Return the number of bytes successfully written before encountering an error.
            return i;
        }
    }

    // Return the total number of bytes successfully written.
    return quantity;
}

/**
 * @brief Gets the number of bytes available for reading from the I2C device.
 *
 * This function returns the number of bytes available for reading from the I2C device.
 *
 * @return The number of bytes available for reading.
 */
int i2c_tools_available(void)
{
    // Check if I2C is currently running.
    return _running ? _buffLen - _buffOff : 0;
}

/**
 * @brief Reads a byte from the I2C device.
 *
 * This function reads a byte from the internal buffer of the I2C device.
 * It returns the read byte or -1 if no bytes are available (EOF).
 *
 * @return The read byte or -1 if no bytes are available (EOF).
 */
int i2c_tools_read(void)
{
    // Check if bytes are available for reading.
    if (i2c_tools_available())
    {
        // Return the next byte from the internal buffer.
        return _buff[_buffOff++];
    }
    // Return -1 if no bytes are available (EOF).
    return -1;
}

/**
 * @brief Peeks at the next byte in the I2C device without consuming it.
 *
 * This function returns the next byte in the internal buffer of the I2C device without consuming it.
 * It returns the peeked byte or -1 if no bytes are available (EOF).
 *
 * @return The peeked byte or -1 if no bytes are available (EOF).
 */
int i2c_tools_peek(void)
{
    // Check if bytes are available for peeking.
    if (i2c_tools_available())
    {
        // Return the next byte in the internal buffer without consuming it.
        return _buff[_buffOff];
    }
    // Return -1 if no bytes are available (EOF).
    return -1;
}

/**
 * @brief Flushes the internal buffer of the I2C device.
 *
 * This function does nothing, and it is recommended to use endTransmission to force data transfer.
 */
void i2c_tools_flush(void)
{
    // Do nothing, use endTransmission(..) to force data transfer.
}

#pragma endregion

#pragma region miscllaneous functions

/**
 * @brief Converts a 32-bit hexadecimal number to ASCII representation and prints it.
 *
 * This function takes a 32-bit hexadecimal number, converts it to its ASCII representation,
 * and prints the result using printf.
 *
 * @param n The 32-bit hexadecimal number to be converted and printed.
 */
void hexToAscii(uint32_t n)
{
    // Print each nibble of the hexadecimal number as a character.
    printf("%c%c%c%c%c%c%c%c\n",
           '0' + ((n) >> 28 & 0xF),
           '0' + ((n) >> 24 & 0xF),
           '0' + ((n) >> 20 & 0xF),
           '0' + ((n) >> 16 & 0xF),
           '0' + ((n) >> 12 & 0xF),
           '0' + ((n) >> 8 & 0xF),
           '0' + ((n) >> 4 & 0xF),
           '0' + ((n) & 0xF));
}
#pragma endregion

#ifndef __WIRE0_DEVICE
#define __WIRE0_DEVICE i2c0
#endif
#ifndef __WIRE1_DEVICE
#define __WIRE1_DEVICE i2c1
#endif