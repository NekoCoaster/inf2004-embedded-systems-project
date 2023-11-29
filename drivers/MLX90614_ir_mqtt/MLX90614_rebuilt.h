#ifndef __DFROBOT_MLX90614_H__
#define __DFROBOT_MLX90614_H__
#include "i2c_tools.h"

#define MLX90614_ID_NUMBER_1C 0x3802 ///< mlx9614 ID number(reg: MLX90614_ID_NUMBER)

#define MLX90614_TA 0x06    ///< the register address stores the measured ambient temperature
#define MLX90614_TOBJ1 0x07 ///< the register address stores the measured temperature of object 1
#define MLX90614_TOBJ2 0x08 ///< the register address stores the measured temperature of object 2

/* these addresses are in EEPROM, read requires to "| 0x20" */
#define MLX90614_TO_MAX (0x00 | 0x20) ///< The addresses Tomax, Tomin and Ta range are for customer dependent object and ambient temperature ranges
#define MLX90614_TO_MIN (0x01 | 0x20)
#define MLX90614_PWMCTRL (0x02 | 0x20) ///< The address PWMCTRL consists of control bits for configuring the PWM/SDA pin as follows
#define MLX90614_TA_RANGE (0x03 | 0x20)
#define MLX90614_EMISSIVITY (0x04 | 0x20)  ///< The address Emissivity contains the object emissivity (factory default 1.0 = 0xFFFF)
#define MLX90614_CONFIG_REG1 (0x05 | 0x20) ///< The address ConfigRegister1 consists of control bits for configuring the analog and digital parts
#define MLX90614_SMBUS_ADDR (0x0E | 0x20)  ///< SMBus address (LSByte only)
#define MLX90614_ID_NUMBER (0x1C | 0x20)   ///< ID number

#define MLX90614_FLAGS 0xF0          ///< Read Flags
#define MLX90614_SLEEP_MODE 0xFF     ///< Enter SLEEP mode
#define MLX90614_SLEEP_MODE_PEC 0xE8 ///< Enter SLEEP mode PEC

#define NO_ERR 0            ///< No error
#define ERR_DATA_BUS (-1)   ///< data bus error
#define ERR_IC_VERSION (-2) ///< the chip version not match

typedef enum
{
    eIIR50 = 0,
    eIIR25,
    eIIR17,
    eIIR13,
    eIIR100,
    eIIR80,
    eIIR67,
    eIIR57,
} eIIRMode_t;

typedef enum
{
    eFIR8 = 0,
    eFIR16,
    eFIR32,
    eFIR64,
    eFIR128,
    eFIR256,
    eFIR512,
    eFIR1024,
} eFIRMode_t;

/*****MLX90614 Sensor Component Section *****/

/**
 * @fn begin
 * @brief initialization function
 * @return int type, indicates returning initialization status
 * @retval 0 NO_ERROR
 * @retval -1 ERR_DATA_BUS
 * @retval -2 ERR_IC_VERSION
 */
int MLX90614_begin();

/**
 * @fn setEmissivityCorrectionCoefficient
 * @brief set the emissivity calibration coefficient, users need to calculate the ratio of the temperature measured before the sensor changes emissivity to the true temperature of the object,
 * @n     upload the ratio to the api as a parameter, and the deviation of the object absolute temperature measured by the sensor will be lower
 * @param calibrationValue new calibration coefficient, the ratio of the temperature measured before the sensor changes emissivity to the true temperature of the object, range: (0~1)
 * @return None
 */
void MLX90614_setEmissivityCorrectionCoefficient(float calibrationValue);
/**
 * @fn setMeasuredParameters
 * @brief set the measurement parameters, including IIR (Infinite Impulse Response Digital Filter) and FIR (Finite Impulse Response Digital Filter)
 * @param IIRMode: eIIR100, eIIR80, eIIR67, eIIR57; (Default: eIIR100)
 * @param FIRMode: eFIR128, eFIR256, eFIR512, eFIR1024; (Default: eFIR1024)
 * @return None
 */
void MLX90614_setMeasuredParameters(eIIRMode_t IIRMode, eFIRMode_t FIRMode);

/**
 * @fn getAmbientTempCelsius
 * @brief get ambient temperature, unit is Celsius
 * @return return value range： -40.01 °C ~ 85 °C
 */
float MLX90614_getAmbientTempCelsius(void);

/**
 * @fn getObjectTempCelsius
 * @brief get temperature of object 1, unit is Celsius
 * @return return value range：
 * @n  -70.01 °C ~ 270 °C(MLX90614ESF-DCI)
 * @n  -70.01 °C ~ 380 °C(MLX90614ESF-DCC)
 */
float MLX90614_getObjectTempCelsius(void);

/**
 * @fn getObject2TempCelsius
 * @brief get temperature of object 2, unit is Celsius
 * @return return value range： -40 C ~ 85 C
 */
float MLX90614_getObject2TempCelsius(void);

/**
 * @fn readModuleFlags
 * @brief read the sensor flags
 * @return four flags
 */
uint8_t MLX90614_readModuleFlags(void);

/**
 * @fn writeReg
 * @brief write register function, design it as a pure virtual function, implement the function body through a derived class
 * @param reg  register address 8bits
 * @param pBuf to write data storage and buffer
 * @return None
 */

void MLX90614_writeReg(uint8_t reg, const void *pBuf);

/**
 * @fn readReg
 * @brief read register function, design it as a pure virtual function, implement the function body through a derived class
 * @param reg  register address 8bits
 * @param pBuf to read data storage and buffer
 * @return return read length, returning 0 means read failure
 */
size_t MLX90614_readReg(uint8_t reg, void *pBuf);

/*****MLX90614 I2C Component Section *****/

/**
 * @fn DFRobot_MLX90614_I2C
 * @brief constructor, set sensor I2C communication address according to SDO pin wiring
 * @param i2cAddr mlx9614 I2C communication address (Default: 0x5A)
 * @param pWire Wire object is defined in Wire.h, so just use &Wire and the methods in Wire can be pointed to and used
 * @return None
 */
void MLX90614_I2C_init(uint8_t i2cAddr);

/**
 * @fn begin
 * @brief subclass initialization function
 * @return returning 0 indicates initialization succeeds, returning other values indicates initialization fails
 */
int MLX90614_I2C_begin();

/**
 * @fn enterSleepMode
 * @brief control the sensor sleep mode, must enter and exit the sleep mode once after the sensor is configured (equivalent to soft reset) to ensure the normal reading of the measured data
 * @param mode select to enter or exit sleep mode, it’s enter sleep mode by default (Default: true)
 * @n            true put the sensor to sleep
 * @n            false wake up the sensor (automatically exit sleep mode after power down and restart)
 * @return None
 */
void MLX90614_enterSleepMode(bool mode);

/**
 * @fn setI2CAddress
 * @brief set I2C communication address, the setting takes effect after power down and restart
 * @param addr new I2C communication address 7bit, range: (0~127)
 * @return None
 */
void MLX90614_setI2CAddress(uint8_t addr);

/**
 * @fn crc8Polyomial107
 * @brief calculate crc8 (polynomial corresponds to 0x107) check code (PEC)
 * @param ptr correspondingly calculate the data pointer of check code
 * @param len correspondingly calculate the data length of check code
 * @return char type, return the calculated crc check code
 */
unsigned char MLX90614_crc8Polyomial107(unsigned char *ptr, size_t len);

/**
 * @fn writeReg
 * @brief write register values through I2C bus
 * @param reg  register address 8bits
 * @param pBuf to write data storage and buffer
 * @return None
 */
void MLX90614_writeReg(uint8_t reg, const void *pBuf);

/**
 * @fn readReg
 * @brief read register values through I2C bus
 * @param reg  register address 8bits
 * @param pBuf to read data storage and buffer
 * @return return read length, returning 0 means read failure
 */
size_t MLX90614_readReg(uint8_t reg, void *pBuf);

#endif
