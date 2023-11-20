
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
void begin();
// Shut down the I2C interface
void end();

// Select IO pins to use.  Call before ::begin()
bool setSDA(int sda);
bool setSCL(int scl);

void setClock(uint32_t freqHz);

void beginTransmission(uint8_t);
uint8_t endTransmission_w_stopbit(bool stopBit);
uint8_t endTransmission(void);

size_t requestFrom_w_stopbit(uint8_t address, size_t quantity, bool stopBit);
size_t requestFrom(uint8_t address, size_t quantity);

size_t write(uint8_t data);
size_t write_w_quantity(const uint8_t *data, size_t quantity);

int available(void);
int read(void);
int peek(void);
void flush(void);
void hexToAscii(uint32_t n);

inline size_t write_ulong(unsigned long n)
{
    return write((uint8_t)n);
}
inline size_t write_long(long n)
{
    return write((uint8_t)n);
}
inline size_t write_uint(unsigned int n)
{
    return write((uint8_t)n);
}
inline size_t write_int(int n)
{
    return write((uint8_t)n);
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