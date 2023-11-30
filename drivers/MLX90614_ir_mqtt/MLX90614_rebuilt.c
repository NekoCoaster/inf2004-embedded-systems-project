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

/**
 * @brief Calculates the CRC-8 checksum using the Polynomial 107 algorithm.
 *
 * This function calculates the CRC-8 checksum of the given data array using
 * the Polynomial 107 algorithm. The calculated checksum is returned.
 *
 * @param ptr Pointer to the data array for which CRC-8 is to be calculated.
 * @param len The length of the data array.
 *
 * @return The calculated CRC-8 checksum value.
 */
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

/**
 * @brief Writes data to a register on the MLX90614 sensor over I2C.
 *
 * This function writes data to the specified register on the MLX90614 sensor
 * using the I2C communication protocol. It checks for a null pointer in the
 * provided data buffer, prepares a packet for calculating the check code,
 * and performs the necessary I2C transactions to write data to the register.
 * The function uses a CRC-8 checksum for error checking during the write process.
 *
 * @param reg The register address on the MLX90614 sensor to write data to.
 * 
 * @param pBuf A pointer to the data buffer containing the data to be written.
 * 
 * @return void
 */
void MLX90614_I2C_writeReg(uint8_t reg, const void *pBuf)
{
    if (pBuf == NULL)
    {
        DBG("pBuf ERROR!! : null pointer");
    }
    uint8_t *_pBuf = (uint8_t *)pBuf;
    unsigned char crc_write[5] = {(uint8_t)(_deviceAddr << 1), reg, _pBuf[0], _pBuf[1], '\0'}; // the array prepared for calculating the check code
    i2c_tools_beginTransmission(_deviceAddr);
    i2c_tools_write(reg);

    for (size_t i = 0; i < 2; i++)
    {
        i2c_tools_write(_pBuf[i]);
    }
    i2c_tools_write(MLX90614_crc8Polyomial107(crc_write, 4));
    i2c_tools_endTransmission();
}

/**
 * @brief Reads data from a register on the MLX90614 sensor over I2C.
 *
 * This function reads data from the specified register on the MLX90614 sensor
 * using the I2C communication protocol. It checks for a null pointer in the
 * provided data buffer, performs I2C transactions to read data from the register,
 * and verifies the received data using a CRC-8 checksum. The function returns
 * the number of bytes successfully read and updates the provided data buffer.
 *
 * @param reg The register address on the MLX90614 sensor to read data from.
 * 
 * @param pBuf A pointer to the data buffer to store the read data.
 * 
 * @return The number of bytes successfully read from the register.
 */
size_t MLX90614_I2C_readReg(uint8_t reg, void *pBuf)
{
    size_t count = 0;
    if (NULL == pBuf)
    {
        DBG("pBuf ERROR!! : null pointer");
    }
    uint8_t *_pBuf = (uint8_t *)pBuf;
    i2c_tools_beginTransmission(_deviceAddr);
    i2c_tools_write(reg);
    if (0 != i2c_tools_endTransmission_w_stopbit(false))
    {
        DBG("endTransmission ERROR!!");
    }
    else
    {
        // Master device requests size bytes from slave device, which can be accepted by master device with read() or available()
        i2c_tools_requestFrom(_deviceAddr, (size_t)3);
        while (i2c_tools_available())
        {
            _pBuf[count++] = (uint8_t)i2c_tools_read(); // Use read() to receive and put into buf
        }
        i2c_tools_endTransmission();
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

/**
 * @brief Initializes communication with the MLX90614 sensor.
 *
 * This function initiates communication with the MLX90614 sensor by reading
 * the sensor's ID number through the I2C interface. It checks for successful
 * data bus communication and verifies the sensor's ID and chip version.
 * If the communication is successful and the chip version matches, the function
 * returns without errors. Otherwise, it prints debug messages and returns an
 * error code.
 *
 * @return An integer indicating the success or failure of the initialization.
 *         - NO_ERR: Initialization successful.
 *         - ERR_DATA_BUS: Failed data bus communication.
 *         - ERR_IC_VERSION: Chip version mismatch.
 */
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

/**
 * @brief Sets the emissivity correction coefficient for the MLX90614 sensor.
 *
 * This function calculates the emissivity value based on the provided calibration
 * value and writes it to the MLX90614 sensor's emissivity register using I2C communication.
 * The emissivity value is a 16-bit unsigned integer, and the function splits it into
 * two bytes before writing it to the register. The calibration value is a floating-point
 * number between 0 and 1, representing the emissivity correction coefficient.
 *
 * @param calibrationValue The calibration value used to set the emissivity correction coefficient.
 *                         Should be a floating-point number between 0 and 1.
 *
 * @return void
 */
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

/**
 * @brief Sets the measured parameters for the MLX90614 sensor.
 *
 * This function reads the current configuration from the MLX90614 sensor's configuration
 * register, clears the bits related to IIR and FIR modes, and then sets the desired IIR
 * and FIR modes. The IIR and FIR modes represent the filtering modes used for temperature
 * measurements. The function uses I2C communication to read and write the configuration register.
 *
 * @param IIRMode The desired IIR (Infinite Impulse Response) filtering mode.
 *                Should be a value from the eIIRMode_t enumeration.
 *
 * @param FIRMode The desired FIR (Finite Impulse Response) filtering mode.
 *                Should be a value from the eFIRMode_t enumeration.
 *
 * @return void
 */
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

/**
 * @brief Gets the ambient temperature in degrees Celsius from the MLX90614 sensor.
 *
 * This function reads the raw temperature data from the ambient temperature register
 * (MLX90614_TA), converts it to degrees Celsius using the sensor's scaling factor,
 * and returns the temperature as a floating-point value.
 *
 * @return The ambient temperature in degrees Celsius.
 */
float MLX90614_getAmbientTempCelsius(void)
{
    uint8_t buf[2];
    MLX90614_I2C_readReg(MLX90614_TA, buf);
    float temp = ((uint16_t)buf[0] | (uint16_t)(buf[1] << 8)) * 0.02 - 273.15;

    return temp; // Get celsius temperature of the ambient
}

/**
 * @brief Gets the object temperature in degrees Celsius from the MLX90614 sensor.
 *
 * This function reads the raw temperature data from the object temperature register
 * (MLX90614_TOBJ1), converts it to degrees Celsius using the sensor's scaling factor,
 * and returns the temperature as a floating-point value.
 *
 * @return The object temperature in degrees Celsius.
 */
float MLX90614_getObjectTempCelsius(void)
{
    uint8_t buf[2];
    MLX90614_I2C_readReg(MLX90614_TOBJ1, buf);
    // DBG((buf[0] | buf[1] << 8), HEX);
    float temp = ((uint16_t)buf[0] | (uint16_t)(buf[1] << 8)) * 0.02 - 273.15;

    return temp; // Get celsius temperature of the object
}

/**
 * @brief Gets the second object temperature in degrees Celsius from the MLX90614 sensor.
 *
 * This function reads the raw temperature data from the second object temperature register
 * (MLX90614_TOBJ2), converts it to degrees Celsius using the sensor's scaling factor,
 * and returns the temperature as a floating-point value.
 *
 * @return The second object temperature in degrees Celsius.
 */
float MLX90614_getObject2TempCelsius(void)
{
    uint8_t buf[2];
    MLX90614_I2C_readReg(MLX90614_TOBJ2, buf);
    // DBG((buf[0] | buf[1] << 8), HEX);
    float temp = ((uint16_t)buf[0] | (uint16_t)(buf[1] << 8)) * 0.02 - 273.15;

    return temp; // Get celsius temperature of the object
}

/**
 * @brief Reads and interprets the module flags from the MLX90614 sensor.
 *
 * This function reads the module flags register (MLX90614_FLAGS) from the MLX90614 sensor
 * and interprets the individual bits to determine various status conditions. The function
 * returns a uint8_t value where each bit represents a specific module flag.
 *
 * @return A uint8_t value where each bit represents a specific module flag:
 *         - Bit 0: Not implemented.
 *         - Bit 1: INIT - POR initialization routine is still ongoing (Low active).
 *         - Bit 2: EE_DEAD - EEPROM double error has occurred (High active).
 *         - Bit 3: EEBUSY - The previous write/erase EEPROM access is still in progress (High active).
 */
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

/**
 * @brief Initializes the I2C communication for the MLX90614 sensor with the specified address.
 *
 * This function initializes the I2C communication for the MLX90614 sensor by setting the
 * I2C device address to the provided address.
 *
 * @param i2cAddr The I2C address of the MLX90614 sensor.
 */
void MLX90614_I2C_init(uint8_t i2cAddr)
{
    _deviceAddr = i2cAddr;
}

/**
 * @brief Initializes the MLX90614 sensor and wakes it from sleep mode.
 *
 * This function initiates the initialization process for the MLX90614 sensor by waking it
 * from sleep mode and then calls the MLX90614_begin function to complete the sensor setup.
 *
 * @return An integer representing the initialization status:
 *         - NO_ERR: Initialization successful.
 *         - ERR_DATA_BUS: Error on the data bus during initialization.
 *         - ERR_IC_VERSION: Error indicating an incompatible chip version.
 */
int MLX90614_I2C_begin()
{
    MLX90614_enterSleepMode(false);
    sleep_ms(50);

    return MLX90614_begin();
}

/**
 * @brief Enters or exits sleep mode for the MLX90614 sensor.
 *
 * This function allows the MLX90614 sensor to enter or exit sleep mode based on the
 * specified mode parameter. In sleep mode, the sensor consumes less power.
 *
 * @param mode Boolean value:
 *             - true: Enter sleep mode.
 *             - false: Exit sleep mode.
 */
void MLX90614_enterSleepMode(bool mode)
{
    if (mode)
    {
        // sleep command, refer to the chip datasheet
        i2c_tools_beginTransmission(_deviceAddr);
        i2c_tools_write(MLX90614_SLEEP_MODE);
        i2c_tools_write(MLX90614_SLEEP_MODE_PEC);
        i2c_tools_endTransmission();
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

        i2c_tools_begin(); // Wire.h（I2C）library function initialize wire library

        i2c_tools_beginTransmission(_deviceAddr);
        i2c_tools_endTransmission();
        DBG("exit sleep mode");
    }
    sleep_ms(200);
}

/**
 * @brief Sets the I2C address for the MLX90614 sensor.
 *
 * This function allows the user to set a new I2C address for the MLX90614 sensor
 * by updating the SMBus address register with the specified address.
 *
 * @param addr The new I2C address to be set for the MLX90614 sensor.
 */
void MLX90614_setI2CAddress(uint8_t addr)
{
    uint8_t buf[2] = {0};
    MLX90614_I2C_writeReg(MLX90614_SMBUS_ADDR, buf);
    sleep_ms(10);
    buf[0] = addr;
    MLX90614_I2C_writeReg(MLX90614_SMBUS_ADDR, buf);
    sleep_ms(10);
}
