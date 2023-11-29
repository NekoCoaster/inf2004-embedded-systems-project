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

#include <stdio.h>
#include <pico/stdlib.h>
#include "AS7341_Rebuilt.h"
#include "i2c_tools.h"
#include <stdint.h>

static uint8_t _address;
static uint8_t _mode;
static AS7341_eMode_t measureMode;

#define AS7341_DEBUG

#ifdef AS7341_DEBUG
void AS7341_debugPrint(const char *str)
{
    printf("%s\n", str);
}
#else
void AS7341_debugPrint(const char *str)
{
    // Do nothing
}
#endif
void AS7341_init()
{
}

void AS7341_writeReg(uint8_t reg, void *pBuf, size_t size)
{
    if (pBuf == NULL)
    {
        AS7341_debugPrint("pBuf ERROR!! : null pointer");
    }
    uint8_t *_pBuf = (uint8_t *)pBuf;
    i2c_tools_beginTransmission((uint8_t)_address);
    i2c_tools_write(reg);
    for (uint16_t i = 0; i < size; i++)
    {
        i2c_tools_write(_pBuf[i]);
    }
    i2c_tools_endTransmission();
}
void AS7341_writeReg_direct(uint8_t reg, uint8_t data)
{
    AS7341_writeReg(reg, &data, 1);
}

uint8_t AS7341_readReg(uint8_t reg, void *pBuf, size_t size)
{
    if (pBuf == NULL)
    {
        AS7341_debugPrint("pBuf ERROR!! : null pointer");
    }
    uint8_t *_pBuf = (uint8_t *)pBuf;
    i2c_tools_beginTransmission(_address);
    i2c_tools_write(reg);
    if (i2c_tools_endTransmission() != 0)
    {
        return 0;
    }
    sleep_ms(10);
    i2c_tools_requestFrom(_address, size);
    for (uint16_t i = 0; i < size; i++)
    {
        _pBuf[i] = i2c_tools_read();
    }
    return size;
}

uint8_t AS7341_readReg_direct(uint8_t reg)
{

    uint8_t data;
    AS7341_readReg(reg, &data, 1);
    return data;
}

int AS7341_begin(AS7341_eMode_t mode)
{
    _address = 0x39;
    i2c_tools_begin();
    i2c_tools_beginTransmission(_address);
    i2c_tools_endTransmission();

    /*  Skip this section because it's all bugged all to high hell.
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
    AS7341_enableAS7341(true);
    measureMode = mode;
    return ERR_OK;
}

uint8_t AS7341_readID()
{
    uint8_t id;
    if (AS7341_readReg(REG_AS7341_ID, &id, 1) == 0)
    {
        AS7341_debugPrint("id read error");
        return 0;
    }
    else
    {
        return id;
    }
}

void AS7341_enableAS7341(bool on)
{
    uint8_t data;
    AS7341_readReg(REG_AS7341_ENABLE, &data, 1);
    if (on == true)
    {
        data = data | (1 << 0);
    }
    else
    {
        data = data & (~1);
    }
    AS7341_writeReg(REG_AS7341_ENABLE, &data, 1);
}

void AS7341_enableSpectralMeasure(bool on)
{
    uint8_t data;
    AS7341_readReg(REG_AS7341_ENABLE, &data, 1);
    if (on == true)
    {
        data = data | (1 << 1);
    }
    else
    {
        data = data & (~(1 << 1));
    }
    AS7341_writeReg(REG_AS7341_ENABLE, &data, 1);
}

void AS7341_enableWait(bool on)
{

    uint8_t data;
    AS7341_readReg(REG_AS7341_ENABLE, &data, 1);
    if (on == true)
    {
        data = data | (1 << 3);
    }
    else
    {
        data = data & (~(1 << 3));
    }
    AS7341_writeReg(REG_AS7341_ENABLE, &data, 1);
}

void AS7341_enableSMUX(bool on)
{
    uint8_t data;
    AS7341_readReg(REG_AS7341_ENABLE, &data, 1);
    if (on == true)
    {
        data = data | (1 << 4);
    }
    else
    {
        data = data & (~(1 << 4));
    }
    AS7341_writeReg(REG_AS7341_ENABLE, &data, 1);
}

void AS7341_enableFlickerDetection(bool on)
{

    uint8_t data;
    AS7341_readReg(REG_AS7341_ENABLE, &data, 1);
    if (on == true)
    {
        data = data | (1 << 6);
    }
    else
    {
        data = data & (~(1 << 6));
    }
    AS7341_writeReg(REG_AS7341_ENABLE, &data, 1);
}

void AS7341_config(AS7341_eMode_t mode)
{
    uint8_t data;
    AS7341_setBank(1);
    AS7341_readReg(REG_AS7341_CONFIG, &data, 1);
    switch (mode)
    {
    case eSpm:
    {
        data = (data & (~3)) | eSpm;
    };
    break;
    case eSyns:
    {
        data = (data & (~3)) | eSyns;
    };
    break;
    case eSynd:
    {
        data = (data & (~3)) | eSynd;
    };
    break;
    default:
        break;
    }
    AS7341_writeReg(REG_AS7341_CONFIG, &data, 1);
    AS7341_setBank(0);
}

void AS7341_F1F4_Clear_NIR()
{
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

void AS7341_F5F8_Clear_NIR()
{
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

void AS7341_FDConfig()
{

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

void AS7341_startMeasure(AS7341_eChChoose_t mode)
{
    uint8_t data = 0;

    AS7341_readReg(REG_AS7341_CFG_0, &data, 1);
    data = data & (~(1 << 4));
    AS7341_writeReg(REG_AS7341_CFG_0, &data, 1);

    AS7341_enableSpectralMeasure(false);
    AS7341_writeReg_direct(0xAF, 0x10);
    if (mode == eF1F4ClearNIR)
        AS7341_F1F4_Clear_NIR();
    else if (mode == eF5F8ClearNIR)
        AS7341_F5F8_Clear_NIR();
    AS7341_enableSMUX(true);
    if (measureMode == eSyns)
    {
        AS7341_setGpioMode(INPUT);
        AS7341_config(eSyns);
        // AS7341_writeReg(0xA9), 0));
    }
    else if (measureMode == eSpm)
    {
        AS7341_config(eSpm);
    }
    AS7341_enableSpectralMeasure(true);
    if (measureMode == eSpm)
    {
        while (!AS7341_measureComplete())
        {
            sleep_ms(1);
        }
    }
}
uint8_t AS7341_readFlickerData()
{
    uint8_t flicker;
    uint8_t data = 0;
    AS7341_readReg(REG_AS7341_CFG_0, &data, 1);
    data = data & (~(1 << 4));
    AS7341_writeReg(REG_AS7341_CFG_0, &data, 1);
    AS7341_enableSpectralMeasure(false);
    AS7341_writeReg_direct(0xAF, 0x10);
    AS7341_FDConfig();
    AS7341_enableSMUX(true);
    AS7341_enableSpectralMeasure(true);
    uint8_t retry = 100;

    if (retry == 0)
        AS7341_debugPrint(" data access error");
    AS7341_enableFlickerDetection(true);
    sleep_ms(600);
    AS7341_readReg(REG_AS7341_STATUS, &flicker, 1);
    AS7341_enableFlickerDetection(false);
    switch (flicker)
    {
    case 44:
        flicker = 1;
        break;
    case 45:
        flicker = 50;
        break;
    case 46:
        flicker = 60;
        break;
    default:
        flicker = 0;
    }
    return flicker;
}

bool AS7341_measureComplete()
{
    uint8_t status;
    AS7341_readReg(REG_AS7341_STATUS_2, &status, 1);
    if ((status & (1 << 6)))
    {
        return true;
    }
    else
    {
        return false;
    }
}

uint16_t AS7341_getChannelData(uint8_t channel)
{
    uint8_t data[2];
    uint16_t channelData = 0x0000;
    AS7341_readReg(REG_AS7341_CH0_DATA_L + channel * 2, data, 1);
    AS7341_readReg(REG_AS7341_CH0_DATA_H + channel * 2, data + 1, 1);
    channelData = data[1];
    channelData = (channelData << 8) | data[0];
    sleep_ms(50);
    return channelData;
}

AS7341_sModeOneData_t AS7341_readSpectralDataOne()
{
    AS7341_sModeOneData_t data;
    data.ADF1 = AS7341_getChannelData(0);
    data.ADF2 = AS7341_getChannelData(1);
    data.ADF3 = AS7341_getChannelData(2);
    data.ADF4 = AS7341_getChannelData(3);
    data.ADCLEAR = AS7341_getChannelData(4);
    data.ADNIR = AS7341_getChannelData(5);
    return data;
}

AS7341_sModeTwoData_t AS7341_readSpectralDataTwo()
{
    AS7341_sModeTwoData_t data;
    data.ADF5 = AS7341_getChannelData(0);
    data.ADF6 = AS7341_getChannelData(1);
    data.ADF7 = AS7341_getChannelData(2);
    data.ADF8 = AS7341_getChannelData(3);
    data.ADCLEAR = AS7341_getChannelData(4);
    data.ADNIR = AS7341_getChannelData(5);
    return data;
}

void AS7341_setGpio(bool connect)
{
    uint8_t data;
    AS7341_readReg(REG_AS7341_CPIO, &data, 1);
    if (connect == true)
    {
        data = data | (1 << 0);
    }
    else
    {
        data = data & (~(1 << 0));
    }
    AS7341_writeReg(REG_AS7341_CPIO, &data, 1);
}

void AS7341_setGpioMode(uint8_t mode)
{
    uint8_t data;

    AS7341_readReg(REG_AS7341_GPIO_2, &data, 1);

    if (mode == INPUT)
    {
        data = data | (1 << 2);
    }

    if (mode == OUTPUT)
    {
        data = data & (~(1 << 2));
    }
    AS7341_writeReg(REG_AS7341_GPIO_2, &data, 1);
}

void AS7341_enableLed(bool on)
{
    uint8_t data = 0;
    uint8_t data1 = 0;
    AS7341_setBank(1);

    AS7341_readReg(REG_AS7341_CONFIG, &data, 1);
    AS7341_readReg(REG_AS7341_LED, &data1, 1);
    if (on == true)
    {
        data = data | (1 << 3);
        data1 = data1 | (1 << 7);
    }
    else
    {
        data = data & (~(1 << 3));
        data1 = data1 & (~(1 << 7));
    }
    AS7341_writeReg(REG_AS7341_CONFIG, &data, 1);
    AS7341_writeReg(REG_AS7341_LED, &data1, 1);
    AS7341_setBank(0);
}

void AS7341_setBank(uint8_t addr)
{
    uint8_t data = 0;
    AS7341_readReg(REG_AS7341_CFG_0, &data, 1);
    if (addr == 1)
    {

        data = data | (1 << 4);
    }

    if (addr == 0)
    {

        data = data & (~(1 << 4));
    }
    AS7341_writeReg(REG_AS7341_CFG_0, &data, 1);
}
void AS7341_controlLed(uint8_t current)
{
    uint8_t data = 0;
    if (current < 1)
        current = 1;
    current--;
    if (current > 19)
        current = 19;

    AS7341_setBank(1);

    data = 0;
    // AS7341_readReg(REG_AS7341_LED,&data,1);
    data = data | (1 << 7);
    data = data | (current & 0x7f);
    AS7341_writeReg(REG_AS7341_LED, &data, 1);
    sleep_ms(100);
    // AS7341_readReg(REG_AS7341_CFG_0,&data,1);
    // data = data & (~(1<<4));
    // AS7341_writeReg(REG_AS7341_CFG_0,&data,1);
    AS7341_setBank(0);
}

void AS7341_setInt(bool connect)
{
    uint8_t data;
    AS7341_readReg(REG_AS7341_CPIO, &data, 1);
    if (connect == true)
    {
        data = data | (1 << 1);
    }
    else
    {
        data = data & (~(1 << 1));
    }
    AS7341_writeReg(REG_AS7341_CPIO, &data, 1);
}

void AS7341_enableSysInt(bool on)
{
    uint8_t data;
    AS7341_readReg(REG_AS7341_INTENAB, &data, 1);
    if (on == true)
    {
        data = data | (1 << 0);
    }
    else
    {
        data = data & (~(1 << 0));
    }
    AS7341_writeReg(REG_AS7341_INTENAB, &data, 1);
}

void AS7341_enableFIFOInt(bool on)
{
    uint8_t data;
    AS7341_readReg(REG_AS7341_INTENAB, &data, 1);
    if (on == true)
    {
        data = data | (1 << 2);
    }
    else
    {
        data = data & (~(1 << 2));
    }
    AS7341_writeReg(REG_AS7341_INTENAB, &data, 1);
}

void AS7341_enableSpectralInt(bool on)
{
    uint8_t data;
    AS7341_readReg(REG_AS7341_INTENAB, &data, 1);
    if (on == true)
    {
        data = data | (1 << 3);
    }
    else
    {
        data = data & (~(1 << 3));
    }
    AS7341_writeReg(REG_AS7341_INTENAB, &data, 1);
}

void AS7341_endSleep()
{
    uint8_t data;
    AS7341_readReg(REG_AS7341_INTENAB, &data, 1);
    data = data | (1 << 3);
    AS7341_writeReg(REG_AS7341_INTENAB, &data, 1);
}

void AS7341_clearFIFO()
{
    uint8_t data;
    AS7341_readReg(REG_AS7341_CONTROL, &data, 1);
    data = data | (1 << 0);
    data = data & (~(1 << 0));
    AS7341_writeReg(REG_AS7341_CONTROL, &data, 1);
}

void AS7341_spectralAutozero()
{
    uint8_t data;
    AS7341_readReg(REG_AS7341_CONTROL, &data, 1);
    data = data | (1 << 1);
    AS7341_writeReg(REG_AS7341_CONTROL, &data, 1);
}

void AS7341_enableFlickerInt(bool on)
{
    uint8_t data;
    AS7341_readReg(REG_AS7341_INTENAB, &data, 1);
    if (on == true)
    {
        data = data | (1 << 2);
    }
    else
    {
        data = data & (~(1 << 2));
    }
    AS7341_writeReg(REG_AS7341_INTENAB, &data, 1);
}

void AS7341_setAtime(uint8_t value)
{
    AS7341_writeReg(REG_AS7341_ATIME, &value, 1);
}

void AS7341_setAGAIN(uint8_t value)
{
    if (value > 10)
        value = 10;
    AS7341_writeReg(REG_AS7341_CFG_1, &value, 1);
}

void AS7341_setAstep(uint16_t value)
{
    uint8_t highValue, lowValue;
    lowValue = value & 0x00ff;
    highValue = value >> 8;
    AS7341_writeReg(REG_AS7341_ASTEP_L, &lowValue, 1);

    AS7341_writeReg(REG_AS7341_ASTEP_H, &highValue, 1);
}

float AS7341_getIntegrationTime()
{

    uint8_t data;
    uint8_t astepData[2] = {0};
    uint16_t astep;
    AS7341_readReg(REG_AS7341_ATIME, &data, 1);
    AS7341_readReg(REG_AS7341_ASTEP_L, astepData, 1);
    AS7341_readReg(REG_AS7341_ASTEP_L, astepData + 1, 1);
    astep = astepData[1];
    astep = (astep << 8) | astepData[0];
    return 0;
}

void AS7341_setWtime(uint8_t value)
{
    AS7341_writeReg(REG_AS7341_WTIME, &value, 1);
}
float AS7341_getWtime()
{
    float value;
    uint8_t data;
    AS7341_readReg(REG_AS7341_WTIME, &data, 1);
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
    return value;
}

void AS7341_setThreshold(uint16_t lowTh, uint16_t highTh)
{
    if (lowTh >= highTh)
        return;

    AS7341_writeReg_direct(REG_AS7341_SP_TH_H_MSB, highTh >> 8);
    AS7341_writeReg_direct(REG_AS7341_SP_TH_H_LSB, highTh);

    AS7341_writeReg_direct(REG_AS7341_SP_TH_L_MSB, lowTh >> 8);
    AS7341_writeReg_direct(REG_AS7341_SP_TH_L_LSB, lowTh);

    sleep_ms(10);

    sleep_ms(10);
}
uint16_t AS7341_getLowThreshold()
{
    uint16_t data;
    data = AS7341_readReg_direct(REG_AS7341_SP_TH_L_MSB);
    // Serial.println(data);
    data = AS7341_readReg_direct(REG_AS7341_SP_TH_L_LSB) | (data << 8);
    // Serial.println(AS7341_readReg(REG_AS7341_SP_TH_L_LSB));
    return data;
}
uint16_t AS7341_getHighThreshold()
{

    uint16_t data;
    data = AS7341_readReg_direct(REG_AS7341_SP_TH_H_MSB);
    data = AS7341_readReg_direct(REG_AS7341_SP_TH_H_LSB) | (data << 8);
    return data;
}

void AS7341_clearInterrupt()
{
    AS7341_writeReg_direct(REG_AS7341_STATUS_1, 0xff);
}
void AS7341_enableSpectralInterrupt(bool on)
{
    uint8_t data;
    data = AS7341_readReg_direct(REG_AS7341_INTENAB);
    if (on == true)
    {
        data = data | (1 << 3);
        AS7341_writeReg_direct(REG_AS7341_INTENAB, data);
    }
    else
    {
        data = data & (~(1 << 3));
        AS7341_writeReg_direct(REG_AS7341_INTENAB, data);
    }
}
void AS7341_setIntChannel(uint8_t channel)
{
    if (channel >= 5)
        return;

    uint8_t data;
    data = AS7341_readReg_direct(REG_AS7341_CFG_12);
    data = data & (~(7));
    data = data | (channel);
    AS7341_writeReg_direct(REG_AS7341_CFG_12, data);
}

void AS7341_setAPERS(uint8_t num)
{

    uint8_t data;
    data = AS7341_readReg_direct(REG_AS7341_PERS);
    data = data & (~(15));
    data = data | (num);
    AS7341_writeReg_direct(REG_AS7341_PERS, data);
}
uint8_t AS7341_getIntSource()
{
    return AS7341_readReg_direct(REG_AS7341_STATUS_3);
}

bool AS7341_interrupt()
{

    uint8_t data = AS7341_readReg_direct(REG_AS7341_STATUS_1);
    if (data & 0x80)
    {
        return true;
    }
    else
    {
        return false;
    }
}
bool AS7341_checkWtime()
{
    uint8_t status;
    uint8_t value;
    AS7341_readReg(REG_AS7341_STATUS_6, &status, 1);
    value = status & (1 << 2);
    if (value > 0)
    {
        return false;
    }
    else
    {
        return true;
    }
}
