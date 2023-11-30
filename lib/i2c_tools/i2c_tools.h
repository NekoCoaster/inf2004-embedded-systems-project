/** @file i2c_tools.h
 *
 * @brief This file contains the header file for the I2C tools library.
 *
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

#pragma once
#ifndef _I2C_TOOLS_H_
#define _I2C_TOOLS_H_

#include <hardware/i2c.h>

// WIRE_HAS_END means Wire has end()
#define WIRE_HAS_END 1

#ifndef WIRE_BUFFER_SIZE
#define WIRE_BUFFER_SIZE 256
#endif
#pragma region custom enums to mimic Arduino wiring API
typedef enum
{
    LOW = 0,
    HIGH = 1,
    CHANGE = 2,
    FALLING = 3,
    RISING = 4,
} PinStatus;

typedef enum
{
    INPUT = 0x0,
    OUTPUT = 0x1,
    INPUT_PULLUP = 0x2,
    INPUT_PULLDOWN = 0x3,
    OUTPUT_2MA = 0x4,
    OUTPUT_4MA = 0x5,
    OUTPUT_8MA = 0x6,
    OUTPUT_12MA = 0x7,
} PinMode;
#pragma endregion

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
void digitalWrite(int pin, int val);

/**
 * @brief Reads the digital state of a GPIO pin.
 *
 * This function mimics the behavior of Arduino's digitalRead function.
 * It retrieves the digital state of the specified GPIO pin.
 *
 * @param pin The GPIO pin number.
 * @return The digital state of the pin: HIGH (1) or LOW (0).
 */
bool digitalRead(int pin);

/**
 * @brief Configures the mode of a GPIO pin.
 *
 * This function sets the mode of the specified GPIO pin, such as INPUT, OUTPUT, or INPUT_PULLUP.
 *
 * @param pin The GPIO pin number.
 * @param mode The mode to set for the pin: INPUT, INPUT_PULLUP, INPUT_PULLDOWN, OUTPUT, OUTPUT_2MA, OUTPUT_4MA, OUTPUT_8MA, OUTPUT_12MA.
 */
void pinMode(int pin, int mode);
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
void i2c_tools_init(i2c_inst_t *i2c, int sda, int scl);

/**
 * @brief Initializes I2C communication.
 *
 * This function initializes I2C communication, configuring the I2C instance, pins, and related settings.
 * If I2C is already running, it returns without doing anything further.
 */
void i2c_tools_begin();

/**
 * @brief Ends I2C communication.
 *
 * This function ends I2C communication, deinitializing the I2C instance and setting pins to INPUT mode.
 * If I2C is not currently running, it returns without doing anything further.
 */
void i2c_tools_end();

/**
 * @brief Sets the I2C SDA (data) pin.
 *
 * This function sets the I2C SDA pin to the specified pin number if not currently running.
 *
 * @param pin The pin number to set as the I2C SDA pin.
 * @return True if the pin is successfully set or is already set; False otherwise.
 */
bool i2c_tools_setSDA(int sda);

/**
 * @brief Sets the I2C SCL (clock) pin.
 *
 * This function sets the I2C SCL pin to the specified pin number if not currently running.
 *
 * @param pin The pin number to set as the I2C SCL pin.
 * @return True if the pin is successfully set or is already set; False otherwise.
 */
bool i2c_tools_setSCL(int scl);

/**
 * @brief Sets the clock frequency for I2C communication.
 *
 * This function sets the clock frequency for I2C communication.
 * If I2C is currently running, it updates the baud rate accordingly.
 *
 * @param hz The desired clock frequency in Hertz.
 */
void i2c_tools_setClock(uint32_t freqHz);

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
void i2c_tools_beginTransmission(uint8_t);

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
uint8_t i2c_tools_endTransmission_w_stopbit(bool stopBit);

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
uint8_t i2c_tools_endTransmission(void);

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
size_t i2c_tools_requestFrom_w_stopbit(uint8_t address, size_t quantity, bool stopBit);

/**
 * @brief Requests data from an I2C device with a stop bit.
 *
 * This function is a wrapper for i2c_tools_requestFrom_w_stopbit with the stopBit parameter set to true.
 *
 * @param address The I2C address of the target device.
 * @param quantity The number of bytes to request and read.
 * @return The number of bytes read, or 0 if there was an error or invalid parameters.
 */
size_t i2c_tools_requestFrom(uint8_t address, size_t quantity);

/**
 * @brief Writes a byte to the I2C device.
 *
 * This function writes a byte to the I2C device.
 * It adds the byte to the internal buffer if transmission has begun and the buffer is not full.
 *
 * @param ucData The byte to be written to the I2C device.
 * @return Number of bytes written (1 if successful, 0 otherwise).
 */
size_t i2c_tools_write(uint8_t data);

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
size_t i2c_tools_write_w_quantity(const uint8_t *data, size_t quantity);

/**
 * @brief Gets the number of bytes available for reading from the I2C device.
 *
 * This function returns the number of bytes available for reading from the I2C device.
 *
 * @return The number of bytes available for reading.
 */
int i2c_tools_available(void);

/**
 * @brief Reads a byte from the I2C device.
 *
 * This function reads a byte from the internal buffer of the I2C device.
 * It returns the read byte or -1 if no bytes are available (EOF).
 *
 * @return The read byte or -1 if no bytes are available (EOF).
 */
int i2c_tools_read(void);

/**
 * @brief Peeks at the next byte in the I2C device without consuming it.
 *
 * This function returns the next byte in the internal buffer of the I2C device without consuming it.
 * It returns the peeked byte or -1 if no bytes are available (EOF).
 *
 * @return The peeked byte or -1 if no bytes are available (EOF).
 */
int i2c_tools_peek(void);

/**
 * @brief Flushes the internal buffer of the I2C device.
 *
 * This function does nothing, and it is recommended to use endTransmission to force data transfer.
 */
void i2c_tools_flush(void);

#pragma endregion
#pragma region Miscellaneous functions

/**
 * @brief Converts a 32-bit hexadecimal number to ASCII representation and prints it.
 *
 * This function takes a 32-bit hexadecimal number, converts it to its ASCII representation,
 * and prints the result using printf.
 *
 * @param n The 32-bit hexadecimal number to be converted and printed.
 */
void hexToAscii(uint32_t n);

#pragma endregion

#pragma region Inline functions

/**
 * @brief Writes an unsigned long value to the I2C device.
 *
 * This inline function takes an unsigned long value, converts it to an 8-bit value,
 * and writes it to the I2C device using i2c_tools_write.
 *
 * @param n The unsigned long value to be written to the I2C device.
 * @return The number of bytes written.
 */
inline size_t i2c_tools_write_ulong(unsigned long n)
{
    // Convert the unsigned long value to an 8-bit value and write it to the I2C device.
    return i2c_tools_write((uint8_t)n);
}

/**
 * @brief Writes a long value to the I2C device.
 *
 * This inline function takes a long value, converts it to an 8-bit value,
 * and writes it to the I2C device using i2c_tools_write.
 *
 * @param n The long value to be written to the I2C device.
 * @return The number of bytes written.
 */
inline size_t i2c_tools_write_long(long n)
{
    // Convert the long value to an 8-bit value and write it to the I2C device.
    return i2c_tools_write((uint8_t)n);
}

/**
 * @brief Writes an unsigned int value to the I2C device.
 *
 * This inline function takes an unsigned int value, converts it to an 8-bit value,
 * and writes it to the I2C device using i2c_tools_write.
 *
 * @param n The unsigned int value to be written to the I2C device.
 * @return The number of bytes written.
 */
inline size_t i2c_tools_write_uint(unsigned int n)
{
    // Convert the unsigned int value to an 8-bit value and write it to the I2C device.
    return i2c_tools_write((uint8_t)n);
}

/**
 * @brief Writes an int value to the I2C device.
 *
 * This inline function takes an int value, converts it to an 8-bit value,
 * and writes it to the I2C device using i2c_tools_write.
 *
 * @param n The int value to be written to the I2C device.
 * @return The number of bytes written.
 */
inline size_t i2c_tools_write_int(int n)
{
    // Convert the int value to an 8-bit value and write it to the I2C device.
    return i2c_tools_write((uint8_t)n);
}

#pragma endregion

#endif // _I2C_TOOLS_H_