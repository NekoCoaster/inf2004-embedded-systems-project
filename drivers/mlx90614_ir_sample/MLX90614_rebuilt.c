#include <stdio.h>
#include "pico/stdlib.h"
#include "i2c_tools.h"
#include "MLX90614_rebuilt.h"
#include <math.h>

static uint8_t _deviceAddr; // I2C communication device address
#define MLX90614_SDA PICO_DEFAULT_I2C_SDA_PIN
#define MLX90614_SCL PICO_DEFAULT_I2C_SCL_PIN
#define ENABLE_DBG
#ifdef ENABLE_DBG
#define DBG printf
#else
#define DBG(...)

#endif

/***** The imporant stuffs first, otherwise, the C compiler won't stop B***hing ******/

unsigned char MLX90614_crc8Polyomial107(unsigned char *ptr, size_t len)
{
    unsigned char i;
    unsigned char crc = 0x00; // calculated initial crc value

    while (len--)
    {
        // DBG("*ptr, HEX");
        // DBG(*ptr, HEX);

        // first xor with the data to be calculated every time, point to the next data after completing the calculation
        crc ^= *ptr++;

        for (i = 8; i > 0; i--)
        {
            // the following calculation is the same as calculating a byte crc
            if (crc & 0x80)
            {
                crc = (crc << 1) ^ 0x07;
            }
            else
            {
                crc = (crc << 1);
            }
        }
    }
    // DBG(crc, HEX);

    return (crc);
}

void MLX90614_I2C_writeReg(uint8_t reg, const void *pBuf)
{
    if (pBuf == NULL)
    {
        DBG("pBuf ERROR!! : null pointer");
    }
    uint8_t *_pBuf = (uint8_t *)pBuf;
    unsigned char crc_write[5] = {(uint8_t)(_deviceAddr << 1), reg, _pBuf[0], _pBuf[1], '\0'}; // the array prepared for calculating the check code
    beginTransmission(_deviceAddr);
    write(reg);

    for (size_t i = 0; i < 2; i++)
    {
        write(_pBuf[i]);
    }
    write(MLX90614_crc8Polyomial107(crc_write, 4));
    endTransmission();
}

size_t MLX90614_I2C_readReg(uint8_t reg, void *pBuf)
{
    size_t count = 0;
    if (NULL == pBuf)
    {
        DBG("pBuf ERROR!! : null pointer");
    }
    uint8_t *_pBuf = (uint8_t *)pBuf;
    beginTransmission(_deviceAddr);
    write(reg);
    if (0 != endTransmission_w_stopbit(false))
    {
        DBG("endTransmission ERROR!!");
    }
    else
    {
        // Master device requests size bytes from slave device, which can be accepted by master device with read() or available()
        requestFrom(_deviceAddr, (size_t)3);
        while (available())
        {
            _pBuf[count++] = (uint8_t)read(); // Use read() to receive and put into buf
        }
        endTransmission();
        // the array prepared for calculating the check code
        unsigned char crc_read[6] = {(uint8_t)(_deviceAddr << 1), reg, (uint8_t)((_deviceAddr << 1) | 1), _pBuf[0], _pBuf[1], '\0'};

        if (_pBuf[2] != MLX90614_crc8Polyomial107(crc_read, 5))
        {
            count = 0;
            DBG("crc8Polyomial107 ERROR!!");
            hexToAscii(_pBuf[2]);
        }
    }
    return count;
}

/******MLX90614 SENSOR COMPONENT SECTION******/

int MLX90614_begin()
{
    uint8_t idBuf[3];
    if (0 == MLX90614_I2C_readReg(MLX90614_ID_NUMBER, idBuf))
    { // Judge whether the data bus is successful
        DBG("ERR_DATA_BUS");
        return ERR_DATA_BUS;
    }

    DBG("real sensor id=");
    hexToAscii((uint16_t)idBuf[0] | (uint16_t)(idBuf[1] << 8));
    hexToAscii(idBuf[2]);
    if (0 == ((uint16_t)idBuf[0] | (uint16_t)(idBuf[1] << 8)))
    { // Judge whether the chip version matches
        DBG("ERR_IC_VERSION");
        return ERR_IC_VERSION;
    }

    sleep_ms(200);
    DBG("begin ok!");
    return NO_ERR;
}

void MLX90614_setEmissivityCorrectionCoefficient(float calibrationValue)
{
    uint16_t emissivity = round(65535 * calibrationValue);
    hexToAscii(emissivity);
    uint8_t buf[2] = {0};
    MLX90614_I2C_writeReg(MLX90614_EMISSIVITY, buf);
    sleep_ms(10);

    buf[0] = (emissivity & 0x00FF);
    buf[1] = ((emissivity & 0xFF00) >> 8);
    MLX90614_I2C_writeReg(MLX90614_EMISSIVITY, buf);
    sleep_ms(10);
}

void MLX90614_setMeasuredParameters(eIIRMode_t IIRMode, eFIRMode_t FIRMode)
{
    uint8_t buf[2] = {0};
    MLX90614_I2C_readReg(MLX90614_CONFIG_REG1, buf);
    sleep_ms(10);

    buf[0] &= 0xF8;
    buf[1] &= 0xF8;
    MLX90614_I2C_writeReg(MLX90614_CONFIG_REG1, buf);
    sleep_ms(10);

    buf[0] |= IIRMode;
    buf[1] |= FIRMode;
    MLX90614_I2C_writeReg(MLX90614_CONFIG_REG1, buf);
    sleep_ms(10);
}
float MLX90614_getAmbientTempCelsius(void)
{
    uint8_t buf[2];
    MLX90614_I2C_readReg(MLX90614_TA, buf);
    float temp = ((uint16_t)buf[0] | (uint16_t)(buf[1] << 8)) * 0.02 - 273.15;

    return temp; // Get celsius temperature of the ambient
}

float MLX90614_getObjectTempCelsius(void)
{
    uint8_t buf[2];
    MLX90614_I2C_readReg(MLX90614_TOBJ1, buf);
    // DBG((buf[0] | buf[1] << 8), HEX);
    float temp = ((uint16_t)buf[0] | (uint16_t)(buf[1] << 8)) * 0.02 - 273.15;

    return temp; // Get celsius temperature of the object
}

float MLX90614_getObject2TempCelsius(void)
{
    uint8_t buf[2];
    MLX90614_I2C_readReg(MLX90614_TOBJ2, buf);
    // DBG((buf[0] | buf[1] << 8), HEX);
    float temp = ((uint16_t)buf[0] | (uint16_t)(buf[1] << 8)) * 0.02 - 273.15;

    return temp; // Get celsius temperature of the object
}
uint8_t MLX90614_readModuleFlags(void)
{
    uint8_t flagBuf[3], ret = 0;
    MLX90614_I2C_readReg(MLX90614_FLAGS, flagBuf);

    if (flagBuf[0] & (1 << 3))
    {
        ret |= 1;
        DBG("Not implemented.");
    }

    if (!(flagBuf[0] & (1 << 4)))
    {
        ret |= (1 << 1);
        DBG("INIT - POR initialization routine is still ongoing. Low active.");
    }

    if (flagBuf[0] & (1 << 5))
    {
        ret |= (1 << 2);
        DBG("EE_DEAD - EEPROM double error has occurred. High active.");
    }

    if (flagBuf[0] & (1 << 7))
    {
        ret |= (1 << 3);
        DBG("EEBUSY - the previous write/erase EEPROM access is still in progress. High active.");
    }

    return ret;
}

/******MLX90614 I2C COMPONENT SECTION******/
void MLX90614_I2C_init(uint8_t i2cAddr)
{
    _deviceAddr = i2cAddr;
}

int MLX90614_I2C_begin()
{
    MLX90614_enterSleepMode(false);
    sleep_ms(50);

    return MLX90614_begin();
}

void MLX90614_enterSleepMode(bool mode)
{
    if (mode)
    {
        // sleep command, refer to the chip datasheet
        beginTransmission(_deviceAddr);
        write(MLX90614_SLEEP_MODE);
        write(MLX90614_SLEEP_MODE_PEC);
        endTransmission();
        DBG("enter sleep mode");
    }
    else
    {
        // end();

        // wake up command, refer to the chip datasheet
        pinMode(MLX90614_SDA, OUTPUT);
        pinMode(MLX90614_SCL, OUTPUT);
        digitalWrite(MLX90614_SCL, LOW);
        digitalWrite(MLX90614_SDA, HIGH);
        sleep_ms(50);
        digitalWrite(MLX90614_SCL, HIGH);
        digitalWrite(MLX90614_SDA, LOW);
        sleep_ms(50);

        begin(); // Wire.h（I2C）library function initialize wire library

        beginTransmission(_deviceAddr);
        endTransmission();
        DBG("exit sleep mode");
    }
    sleep_ms(200);
}

void MLX90614_setI2CAddress(uint8_t addr)
{
    uint8_t buf[2] = {0};
    MLX90614_I2C_writeReg(MLX90614_SMBUS_ADDR, buf);
    sleep_ms(10);
    buf[0] = addr;
    MLX90614_I2C_writeReg(MLX90614_SMBUS_ADDR, buf);
    sleep_ms(10);
}
