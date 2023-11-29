/*!
 * @file DFRobot_AS7341.h
 * @brief Define the infrastructure of the DFRobot_AS7341 class
 * @details Drive AS7341-IC-integrated spectrum sensor
 * @n AS7341 is a 11 channel visible light sensor, which can measure
 * @n 8 wavelengths of visible light, suitable for color detection,
 * @n  light color temperature detection and other scenes
 * @copyright   Copyright (c) 2010 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @License     The MIT License (MIT)
 * @author [fengli](li.feng@dfrobot.com)
 * @version  V1.0
 * @date  2020-07-16
 * @url https://github.com/DFRobot/DFRobot_AS7341
 */

#ifndef DFROBOT_AS7341_H
#define DFROBOT_AS7341_H

#include <i2c_tools.h>

#define REG_AS7341_ASTATUS 0X60
/*
#define REG_AS7341_CH0_DATA_L  0X61
#define REG_AS7341_CH0_DATA_H  0X62
#define REG_AS7341_ITIME_L  0X63
#define REG_AS7341_ITIME_M  0X64
#define REG_AS7341_ITIME_H  0X65
#define REG_AS7341_CH1_DATA_L  0X66
#define REG_AS7341_CH1_DATA_H  0X67

#define REG_AS7341_CH2_DATA_L  0X68
#define REG_AS7341_CH2_DATA_H  0X69
#define REG_AS7341_CH3_DATA_L  0X6A
#define REG_AS7341_CH3_DATA_H  0X6B
#define REG_AS7341_CH4_DATA_L  0X6C
#define REG_AS7341_CH4_DATA_H  0X6D
#define REG_AS7341_CH5_DATA_L  0X6E
#define REG_AS7341_CH5_DATA_H  0X6F
*/
#define REG_AS7341_CONFIG 0X70
#define REG_AS7341_STAT 0X71
#define REG_AS7341_EDGE 0X72
#define REG_AS7341_CPIO 0X73
#define REG_AS7341_LED 0X74

#define REG_AS7341_ENABLE 0X80
#define REG_AS7341_ATIME 0X81
#define REG_AS7341_WTIME 0X83

#define REG_AS7341_SP_TH_L_LSB 0X84
#define REG_AS7341_SP_TH_L_MSB 0X85
#define REG_AS7341_SP_TH_H_LSB 0X86
#define REG_AS7341_SP_TH_H_MSB 0X87
#define REG_AS7341_AUXID 0X90
#define REG_AS7341_REVID 0X91

#define REG_AS7341_ID 0X92
#define REG_AS7341_STATUS_1 0X93
#define REG_AS7341_ASTATUS 0X94

#define REG_AS7341_CH0_DATA_L 0X95
#define REG_AS7341_CH0_DATA_H 0X96
#define REG_AS7341_CH1_DATA_L 0X97
#define REG_AS7341_CH1_DATA_H 0X98
#define REG_AS7341_CH2_DATA_L 0X99
#define REG_AS7341_CH2_DATA_H 0X9A
#define REG_AS7341_CH3_DATA_L 0X9B
#define REG_AS7341_CH3_DATA_H 0X9C
#define REG_AS7341_CH4_DATA_L 0X9D
#define REG_AS7341_CH4_DATA_H 0X9E
#define REG_AS7341_CH5_DATA_L 0X9F
#define REG_AS7341_CH5_DATA_H 0XA0

#define REG_AS7341_STATUS_2 0XA3
#define REG_AS7341_STATUS_3 0XA4
#define REG_AS7341_STATUS_5 0XA6
#define REG_AS7341_STATUS_6 0XA7
#define REG_AS7341_CFG_0 0XA9
#define REG_AS7341_CFG_1 0XAA
#define REG_AS7341_CFG_3 0XAC
#define REG_AS7341_CFG_6 0XAF
#define REG_AS7341_CFG_8 0XB1
#define REG_AS7341_CFG_9 0XB2
#define REG_AS7341_CFG_10 0XB3
#define REG_AS7341_CFG_12 0XB5

#define REG_AS7341_PERS 0XBD
#define REG_AS7341_GPIO_2 0XBE
#define REG_AS7341_ASTEP_L 0XCA
#define REG_AS7341_ASTEP_H 0XCB
#define REG_AS7341_AGC_GAIN_MAX 0XCF
#define REG_AS7341_AZ_CONFIG 0XD6
#define REG_AS7341_FD_TIME_1 0XD8
#define REG_AS7341_TIME_2 0XDA
#define REG_AS7341_CFG0 0XD7
#define REG_AS7341_STATUS 0XDB
#define REG_AS7341_INTENAB 0XF9
#define REG_AS7341_CONTROL 0XFA
#define REG_AS7341_FIFO_MAP 0XFC
#define REG_AS7341_FIFO_LVL 0XFD
#define REG_AS7341_FDATA_L 0XFE
#define REG_AS7341_FDATA_H 0XFF

#define AS7341_GPIO 4

#define ERR_OK 0          // OK
#define ERR_DATA_BUS -1   // Data Bus error
#define ERR_IC_VERSION -2 // Chip version mismatch

/**
 * @enum eMode_t
 * @brief The measurement mode of spectrum snesor
 */
typedef enum
{
    eSpm = 0,  /**<SPM>*/
    eSyns = 1, /**<SYNS*/
    eSynd = 3, /**<SYND>*/

} AS7341_eMode_t;
/**
 * @enum eChChoose_t
 * @brief The modes of channel mapping
 */
typedef enum
{
    eF1F4ClearNIR, /**<Map the values of the registers of 6 channels to F1,F2,F3,F4,clear,NIR>*/
    eF5F8ClearNIR, /**<Map the values of the registers of 6 channels to F5,F6,F7,F8,clear,NIR>*/
} AS7341_eChChoose_t;

/**
 * @enum eChannel_t
 * @brief represent 10 different photodiode measurement channels
 */
typedef enum
{
    eCH_F1,
    eCH_F2,
    eCH_F3,
    eCH_F4,
    eCH_F5,
    eCH_F6,
    eCH_F7,
    eCH_F8,
    eCH_CLEAR,
    eCH_NIR,
} AS7341_eChannel_t;

/**
 * @struct sModeOneData_t
 * The values of the registers of 6 channels under eF1F4ClearNIR
 */
typedef struct
{
    uint16_t ADF1;    /**<F1 diode data>*/
    uint16_t ADF2;    /**<F2 diode data>*/
    uint16_t ADF3;    /**<F3 diode data>*/
    uint16_t ADF4;    /**<F4 diode data>*/
    uint16_t ADCLEAR; /**<clear diode data>*/
    uint16_t ADNIR;   /**<NIR diode data>*/
} AS7341_sModeOneData_t;
/**
 * @struct sModeTwoData_t
 * The values of the registers of 6 channels under eF5F8ClearNIR
 */
typedef struct
{
    uint16_t ADF5;    /**<F5 diode data>*/
    uint16_t ADF6;    /**<F6 diode data>*/
    uint16_t ADF7;    /**<F7 diode data>*/
    uint16_t ADF8;    /**<F8 diode data>*/
    uint16_t ADCLEAR; /**<clear diode data>*/
    uint16_t ADNIR;   /**<NIR diode data>*/
} AS7341_sModeTwoData_t;

/**
 * @fn DFRobot_AS7341
 * @brief Construct the function
 * @param pWire IC bus pointer object and construction device, can both pass or not pass parameters, Wire in default.
 */
void AS7341_init();

/**
 * @fn begin
 * @brief init function
 * @param mode data read mode. (default: eSpm)
 * @return return 0 if the initialization succeeds, otherwise return non-zero and error code.
 */
int AS7341_begin(AS7341_eMode_t mode);

/**
 * @fn readID
 * @brief read sensor ID
 * @return the read sensor ID, one byte
 */
uint8_t AS7341_readID();

/**
 * @fn setAtime
 * @brief Set the value of register ATIME, through which the value of Integration time can be calculated. The value represents the time that must be spent during data reading.
 * @param value the value of register ATIME
 */
void AS7341_setAtime(uint8_t value);

/**
 * @fn setAstep
 * @brief Sets the integration time per step
 * @param value the value of register Astep
 */
void AS7341_setAstep(uint16_t value);

/**
 * @fn setAGAIN
 * @brief Set gain value (0~10 corresponds to X0.5,X1,X2,X4,X8,X16,X32,X64,X128,X256,X512)
 * @param value The value of register CFG1
 */
void AS7341_setAGAIN(uint8_t value);

/**
 * @fn setWtime
 * @brief Set Set the value of WTIME, through which wite time can be calculated. The value represents the time that
 * @ must be spent during data reading.
 * @param value The value of WTIME
 */
void AS7341_setWtime(uint8_t value);

/**
 * @fn startMeasure
 * @brief Start spectrum measurement
 * @param mode Channel mapping mode: 1.eF1F4ClearNIR,2.eF5F8ClearNIR
 */
void AS7341_startMeasure(AS7341_eChChoose_t mode);

/**
 * @fn readSpectralDataOne
 * @brief Read the value of sensor data channel 0~5, under eF1F4ClearNIR
 * @return The value of channel  ADF1,ADF2,ADF3,ADF4,ADCLEAR and ADNIR
 */

AS7341_sModeOneData_t AS7341_readSpectralDataOne();

/**
 * @fn readSpectralDataTwo
 * @brief Read the value of sensor data channel 0~5, under eF5F8ClearNIR
 * @return The value of channel ADF5,ADF6,ADF7,ADF8,ADCLEAR and ADNIR
 */
AS7341_sModeTwoData_t AS7341_readSpectralDataTwo();

/**
 * @fn readFlickerData
 * @brief Read the value of register flicker, through which the flicker frequency of the light source can be predicted
 * @return The data of register flicker
 */
uint8_t AS7341_readFlickerData();

/**
 * @fn measureComplete
 * @brief Set measurement mode
 * @return Boolean type, the result of measure
 * @retval true measurement completed
 * @retval false Incomplete measurement
 */
bool AS7341_measureComplete();

/**
 * @fn setGpioMode
 * @brief Set the electrical parameters of GPIO pins
 * @param mode INPUT and OUTPUT
 */
void AS7341_setGpioMode(uint8_t mode);

/**
 * @fn enableSpectralMeasure
 * @brief enable spectral measurement
 * @param on true：enable，false：disable
 */
void AS7341_enableSpectralMeasure(bool on);

/**
 * @fn enableLed
 * @brief whether to use LED lights
 * @param on true：yes，false：no
 */
void AS7341_enableLed(bool on);

/**
 * @fn controlLed
 * @brief Set the brightness level of led
 * @param current brightness level (1~20 corresponds to current 4mA,6mA,8mA,10mA,12mA,......,42mA)
 */
void AS7341_controlLed(uint8_t current);

float AS7341_getWtime();
void AS7341_config(AS7341_eMode_t mode);
void AS7341_clearInterrupt();
float AS7341_getIntegrationTime();
uint16_t AS7341_getChannelData(uint8_t channel);
bool AS7341_interrupt();
void AS7341_setThreshold(uint16_t lowTh, uint16_t highTh);
uint16_t AS7341_getLowThreshold();
uint16_t AS7341_getHighThreshold();
void AS7341_enableSpectralInterrupt(bool on);
void AS7341_setIntChannel(uint8_t channel);
void AS7341_setAPERS(uint8_t num);
uint8_t AS7341_getIntSource();

void AS7341_enableAS7341(bool on);

void AS7341_enableWait(bool on);
void AS7341_enableSMUX(bool on);
void AS7341_enableFlickerDetection(bool on);

void AS7341_setBank(uint8_t addr);

void AS7341_setGpio(bool connect);
void AS7341_setInt(bool connect);

void AS7341_enableSysInt(bool on);
void AS7341_enableFIFOInt(bool on);
void AS7341_enableSpectralInt(bool on);
void AS7341_enableFlickerInt(bool on);
void AS7341_F1F4_Clear_NIR();
void AS7341_F5F8_Clear_NIR();
void AS7341_FDConfig();
void AS7341_endSleep();
void AS7341_clearFIFO();
void AS7341_spectralAutozero();
bool AS7341_checkWtime();
/**
 * @brief Write value into register via IIC bus
 * @param reg  Register address 8bits
 * @param pBuf Storage cache of the data to be written into
 * @param size Length of the data to be written into
 * @return Return the actually read length, fails to read if return 0.
 */
void AS7341_writeReg(uint8_t reg, void *pBuf, size_t size);
void AS7341_writeReg_direct(uint8_t reg, uint8_t data);
/**
 * @brief Read register function
 * @param reg  Register address 8bits
 * @param pBuf Storage cache of the data to be read
 * @param size Length of the data to be read
 * @return Return the actually read length, fails to read if return 0.
 */
uint8_t AS7341_readReg(uint8_t reg, void *pBuf, size_t size);
uint8_t AS7341_readReg_direct(uint8_t reg);

#endif
