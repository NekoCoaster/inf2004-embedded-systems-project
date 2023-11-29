
#pragma once
#ifndef _I2C_TOOLS_H_
#define _I2C_TOOLS_H_

#include <hardware/i2c.h>

// WIRE_HAS_END means Wire has end()
#define WIRE_HAS_END 1

#ifndef WIRE_BUFFER_SIZE
#define WIRE_BUFFER_SIZE 256
#endif

void digitalWrite(int pin, int val);

bool digitalRead(int pin);

void pinMode(int pin, int mode);

void i2c_tools_init(i2c_inst_t *i2c, int sda, int scl);

// Start as Master
void i2c_tools_begin();
// Shut down the I2C interface
void i2c_tools_end();

// Select IO pins to use.  Call before ::begin()
bool i2c_tools_setSDA(int sda);
bool i2c_tools_setSCL(int scl);

void i2c_tools_etClock(uint32_t freqHz);

void i2c_tools_beginTransmission(uint8_t);
uint8_t i2c_tools_endTransmission_w_stopbit(bool stopBit);
uint8_t i2c_tools_endTransmission(void);

size_t i2c_tools_requestFrom_w_stopbit(uint8_t address, size_t quantity, bool stopBit);
size_t i2c_tools_requestFrom(uint8_t address, size_t quantity);

size_t i2c_tools_write(uint8_t data);
size_t i2c_tools_write_w_quantity(const uint8_t *data, size_t quantity);

int i2c_tools_available(void);
int i2c_tools_read(void);
int i2c_tools_peek(void);
void i2c_tools_flush(void);
void hexToAscii(uint32_t n);

inline size_t i2c_tools_write_ulong(unsigned long n)
{
    return i2c_tools_write((uint8_t)n);
}
inline size_t i2c_tools_write_long(long n)
{
    return i2c_tools_write((uint8_t)n);
}
inline size_t i2c_tools_write_uint(unsigned int n)
{
    return i2c_tools_write((uint8_t)n);
}
inline size_t i2c_tools_write_int(int n)
{
    return i2c_tools_write((uint8_t)n);
}

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

#endif