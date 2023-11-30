/*
 * Copyright (c) 2018, Sensirion AG
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Sensirion AG nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "sensirion_i2c.h"
#include "sensirion_common.h"
#include "sensirion_config.h"
#include "sensirion_i2c_hal.h"

/**
 * @brief Generate a CRC8 checksum for the given data.
 *
 * @param data Pointer to the array of data bytes.
 * @param count Number of bytes in the data array.
 * @return Calculated CRC8 checksum.
 */
uint8_t sensirion_i2c_generate_crc(const uint8_t* data, uint16_t count) {
    uint16_t current_byte;
    uint8_t crc = CRC8_INIT;
    uint8_t crc_bit;

    /* calculates 8-Bit checksum with given polynomial */
    for (current_byte = 0; current_byte < count; ++current_byte) {
        crc ^= (data[current_byte]);
        for (crc_bit = 8; crc_bit > 0; --crc_bit) {
            if (crc & 0x80)
                crc = (crc << 1) ^ CRC8_POLYNOMIAL;
            else
                crc = (crc << 1);
        }
    }
    return crc;
}
/**
 * @brief Generate a CRC8 checksum for the given data.
 *
 * @param data Pointer to the array of data bytes.
 * @param count Number of bytes in the data array.
 * @return Calculated CRC8 checksum.
 */
int8_t sensirion_i2c_check_crc(const uint8_t* data, uint16_t count,
                               uint8_t checksum) {
    if (sensirion_i2c_generate_crc(data, count) != checksum)
        return CRC_ERROR;
    return NO_ERROR;
}
/**
 * @brief Send a general call reset command.
 *
 * @return Error code.
 */
int16_t sensirion_i2c_general_call_reset(void) {
    const uint8_t data = 0x06;
    return sensirion_i2c_hal_write(0, &data, (uint16_t)sizeof(data));
}
/**
 * @brief Fill the command send buffer with a command and its arguments.
 *
 * @param buf Pointer to the buffer where the command and arguments will be stored.
 * @param cmd Command code.
 * @param args Pointer to the array of arguments.
 * @param num_args Number of arguments.
 * @return Size of the filled buffer.
 */
uint16_t sensirion_i2c_fill_cmd_send_buf(uint8_t* buf, uint16_t cmd,
                                         const uint16_t* args,
                                         uint8_t num_args) {
    uint8_t i;
    uint16_t idx = 0;

    buf[idx++] = (uint8_t)((cmd & 0xFF00) >> 8);
    buf[idx++] = (uint8_t)((cmd & 0x00FF) >> 0);

    for (i = 0; i < num_args; ++i) {
        buf[idx++] = (uint8_t)((args[i] & 0xFF00) >> 8);
        buf[idx++] = (uint8_t)((args[i] & 0x00FF) >> 0);

        uint8_t crc = sensirion_i2c_generate_crc((uint8_t*)&buf[idx - 2],
                                                 SENSIRION_WORD_SIZE);
        buf[idx++] = crc;
    }
    return idx;
}
/**
 * @brief Read data as bytes from the I2C interface.
 *
 * @param address I2C device address.
 * @param data Pointer to the data array where the read bytes will be stored.
 * @param num_words Number of 16-bit words to read.
 * @return Error code.
 */
int16_t sensirion_i2c_read_words_as_bytes(uint8_t address, uint8_t* data,
                                          uint16_t num_words) {
    int16_t ret;
    uint16_t i, j;
    uint16_t size = num_words * (SENSIRION_WORD_SIZE + CRC8_LEN);
    uint16_t word_buf[SENSIRION_MAX_BUFFER_WORDS];
    uint8_t* const buf8 = (uint8_t*)word_buf;

    ret = sensirion_i2c_hal_read(address, buf8, size);
    if (ret != NO_ERROR)
        return ret;

    /* check the CRC for each word */
    for (i = 0, j = 0; i < size; i += SENSIRION_WORD_SIZE + CRC8_LEN) {

        ret = sensirion_i2c_check_crc(&buf8[i], SENSIRION_WORD_SIZE,
                                      buf8[i + SENSIRION_WORD_SIZE]);
        if (ret != NO_ERROR)
            return ret;

        data[j++] = buf8[i];
        data[j++] = buf8[i + 1];
    }

    return NO_ERROR;
}
/**
 * @brief Read 16-bit words from the I2C interface.
 *
 * @param address I2C device address.
 * @param data_words Pointer to the array where the read words will be stored.
 * @param num_words Number of 16-bit words to read.
 * @return Error code.
 */
int16_t sensirion_i2c_read_words(uint8_t address, uint16_t* data_words,
                                 uint16_t num_words) {
    int16_t ret;
    uint8_t i;

    ret = sensirion_i2c_read_words_as_bytes(address, (uint8_t*)data_words,
                                            num_words);
    if (ret != NO_ERROR)
        return ret;

    for (i = 0; i < num_words; ++i) {
        const uint8_t* word_bytes = (uint8_t*)&data_words[i];
        data_words[i] = ((uint16_t)word_bytes[0] << 8) | word_bytes[1];
    }

    return NO_ERROR;
}
/**
 * @brief Write a command to the I2C interface.
 *
 * @param address I2C device address.
 * @param command Command code.
 * @return Error code.
 */
int16_t sensirion_i2c_write_cmd(uint8_t address, uint16_t command) {
    uint8_t buf[SENSIRION_COMMAND_SIZE];

    sensirion_i2c_fill_cmd_send_buf(buf, command, NULL, 0);
    return sensirion_i2c_hal_write(address, buf, SENSIRION_COMMAND_SIZE);
}
/**
 * @brief Write a command with arguments to the I2C interface.
 *
 * @param address I2C device address.
 * @param command Command code.
 * @param data_words Pointer to the array of argument words.
 * @param num_words Number of argument words.
 * @return Error code.
 */
int16_t sensirion_i2c_write_cmd_with_args(uint8_t address, uint16_t command,
                                          const uint16_t* data_words,
                                          uint16_t num_words) {
    uint8_t buf[SENSIRION_MAX_BUFFER_WORDS];
    uint16_t buf_size;

    buf_size =
        sensirion_i2c_fill_cmd_send_buf(buf, command, data_words, num_words);
    return sensirion_i2c_hal_write(address, buf, buf_size);
}
/**
 * @brief Perform a delayed read command on the I2C interface.
 *
 * @param address I2C device address.
 * @param cmd Command code.
 * @param delay_us Delay in microseconds before reading data.
 * @param data_words Pointer to the array where the read words will be stored.
 * @param num_words Number of 16-bit words to read.
 * @return Error code.
 */
int16_t sensirion_i2c_delayed_read_cmd(uint8_t address, uint16_t cmd,
                                       uint32_t delay_us, uint16_t* data_words,
                                       uint16_t num_words) {
    int16_t ret;
    uint8_t buf[SENSIRION_COMMAND_SIZE];

    sensirion_i2c_fill_cmd_send_buf(buf, cmd, NULL, 0);
    ret = sensirion_i2c_hal_write(address, buf, SENSIRION_COMMAND_SIZE);
    if (ret != NO_ERROR)
        return ret;

    if (delay_us)
        sensirion_i2c_hal_sleep_usec(delay_us);

    return sensirion_i2c_read_words(address, data_words, num_words);
}
/**
 * @brief Read a command response from the I2C interface.
 *
 * @param address I2C device address.
 * @param cmd Command code.
 * @param data_words Pointer to the array where the read words will be stored.
 * @param num_words Number of 16-bit words to read.
 * @return Error code.
 */
int16_t sensirion_i2c_read_cmd(uint8_t address, uint16_t cmd,
                               uint16_t* data_words, uint16_t num_words) {
    return sensirion_i2c_delayed_read_cmd(address, cmd, 0, data_words,
                                          num_words);
}
/**
 * @brief Add a command to the buffer.
 *
 * @param buffer Pointer to the buffer.
 * @param offset Offset in the buffer to add the command.
 * @param command Command code.
 * @return Updated offset in the buffer.
 */
uint16_t sensirion_i2c_add_command_to_buffer(uint8_t* buffer, uint16_t offset,
                                             uint16_t command) {
    buffer[offset++] = (uint8_t)((command & 0xFF00) >> 8);
    buffer[offset++] = (uint8_t)((command & 0x00FF) >> 0);
    return offset;
}
/**
 * @brief Add a 32-bit unsigned integer to the buffer.
 *
 * @param buffer Pointer to the buffer.
 * @param offset Offset in the buffer to add the data.
 * @param data 32-bit unsigned integer.
 * @return Updated offset in the buffer.
 */
uint16_t sensirion_i2c_add_uint32_t_to_buffer(uint8_t* buffer, uint16_t offset,
                                              uint32_t data) {
    buffer[offset++] = (uint8_t)((data & 0xFF000000) >> 24);
    buffer[offset++] = (uint8_t)((data & 0x00FF0000) >> 16);
    buffer[offset] = sensirion_i2c_generate_crc(
        &buffer[offset - SENSIRION_WORD_SIZE], SENSIRION_WORD_SIZE);
    offset++;
    buffer[offset++] = (uint8_t)((data & 0x0000FF00) >> 8);
    buffer[offset++] = (uint8_t)((data & 0x000000FF) >> 0);
    buffer[offset] = sensirion_i2c_generate_crc(
        &buffer[offset - SENSIRION_WORD_SIZE], SENSIRION_WORD_SIZE);
    offset++;

    return offset;
}
/**
 * @brief Add a 32-bit signed integer to the buffer.
 *
 * @param buffer Pointer to the buffer.
 * @param offset Offset in the buffer to add the data.
 * @param data 32-bit signed integer.
 * @return Updated offset in the buffer.
 */
uint16_t sensirion_i2c_add_int32_t_to_buffer(uint8_t* buffer, uint16_t offset,
                                             int32_t data) {
    return sensirion_i2c_add_uint32_t_to_buffer(buffer, offset, (uint32_t)data);
}
/**
 * @brief Add a 16-bit unsigned integer to the buffer.
 *
 * @param buffer Pointer to the buffer.
 * @param offset Offset in the buffer to add the data.
 * @param data 16-bit unsigned integer.
 * @return Updated offset in the buffer.
 */
uint16_t sensirion_i2c_add_uint16_t_to_buffer(uint8_t* buffer, uint16_t offset,
                                              uint16_t data) {
    buffer[offset++] = (uint8_t)((data & 0xFF00) >> 8);
    buffer[offset++] = (uint8_t)((data & 0x00FF) >> 0);
    buffer[offset] = sensirion_i2c_generate_crc(
        &buffer[offset - SENSIRION_WORD_SIZE], SENSIRION_WORD_SIZE);
    offset++;

    return offset;
}
/**
 * @brief Add a 16-bit signed integer to the buffer.
 *
 * @param buffer Pointer to the buffer.
 * @param offset Offset in the buffer to add the data.
 * @param data 16-bit signed integer.
 * @return Updated offset in the buffer.
 */
uint16_t sensirion_i2c_add_int16_t_to_buffer(uint8_t* buffer, uint16_t offset,
                                             int16_t data) {
    return sensirion_i2c_add_uint16_t_to_buffer(buffer, offset, (uint16_t)data);
}
/**
 * @brief Add a float to the buffer.
 *
 * @param buffer Pointer to the buffer.
 * @param offset Offset in the buffer to add the data.
 * @param data Floating-point value.
 * @return Updated offset in the buffer.
 */
uint16_t sensirion_i2c_add_float_to_buffer(uint8_t* buffer, uint16_t offset,
                                           float data) {
    union {
        uint32_t uint32_data;
        float float_data;
    } convert;

    convert.float_data = data;

    buffer[offset++] = (uint8_t)((convert.uint32_data & 0xFF000000) >> 24);
    buffer[offset++] = (uint8_t)((convert.uint32_data & 0x00FF0000) >> 16);
    buffer[offset] = sensirion_i2c_generate_crc(
        &buffer[offset - SENSIRION_WORD_SIZE], SENSIRION_WORD_SIZE);
    offset++;
    buffer[offset++] = (uint8_t)((convert.uint32_data & 0x0000FF00) >> 8);
    buffer[offset++] = (uint8_t)((convert.uint32_data & 0x000000FF) >> 0);
    buffer[offset] = sensirion_i2c_generate_crc(
        &buffer[offset - SENSIRION_WORD_SIZE], SENSIRION_WORD_SIZE);
    offset++;

    return offset;
}
/**
 * @brief Add an array of bytes to the buffer.
 *
 * @param buffer Pointer to the buffer.
 * @param offset Offset in the buffer to add the data.
 * @param data Pointer to the array of bytes.
 * @param data_length Number of bytes in the array.
 * @return Updated offset in the buffer.
 */
uint16_t sensirion_i2c_add_bytes_to_buffer(uint8_t* buffer, uint16_t offset,
                                           uint8_t* data,
                                           uint16_t data_length) {
    uint16_t i;

    if (data_length % SENSIRION_WORD_SIZE != 0) {
        return BYTE_NUM_ERROR;
    }

    for (i = 0; i < data_length; i += 2) {
        buffer[offset++] = data[i];
        buffer[offset++] = data[i + 1];

        buffer[offset] = sensirion_i2c_generate_crc(
            &buffer[offset - SENSIRION_WORD_SIZE], SENSIRION_WORD_SIZE);
        offset++;
    }

    return offset;
}
/**
 * @brief Write data to an I2C device.
 *
 * @param address I2C device address.
 * @param data Pointer to the data array.
 * @param data_length Number of bytes in the data array.
 * @return Error code.
 */
int16_t sensirion_i2c_write_data(uint8_t address, const uint8_t* data,
                                 uint16_t data_length) {
    return sensirion_i2c_hal_write(address, data, data_length);
}
/**
 * @brief Read data from an I2C device in place.
 *
 * @param address I2C device address.
 * @param buffer Pointer to the buffer where the read data will be stored.
 * @param expected_data_length Expected number of bytes to read.
 * @return Error code.
 */
int16_t sensirion_i2c_read_data_inplace(uint8_t address, uint8_t* buffer,
                                        uint16_t expected_data_length) {
    int16_t error;
    uint16_t i, j;
    uint16_t size = (expected_data_length / SENSIRION_WORD_SIZE) *
                    (SENSIRION_WORD_SIZE + CRC8_LEN);

    if (expected_data_length % SENSIRION_WORD_SIZE != 0) {
        return BYTE_NUM_ERROR;
    }

    error = sensirion_i2c_hal_read(address, buffer, size);
    if (error) {
        return error;
    }

    for (i = 0, j = 0; i < size; i += SENSIRION_WORD_SIZE + CRC8_LEN) {

        error = sensirion_i2c_check_crc(&buffer[i], SENSIRION_WORD_SIZE,
                                        buffer[i + SENSIRION_WORD_SIZE]);
        if (error) {
            return error;
        }
        buffer[j++] = buffer[i];
        buffer[j++] = buffer[i + 1];
    }

    return NO_ERROR;
}
