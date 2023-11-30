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

#include "sensirion_common.h"
#include "sensirion_config.h"

/**
 * @brief Convert an array of bytes to a uint16_t value (big-endian).
 *
 * @param bytes Pointer to the array of bytes.
 * @return Converted 16-bit unsigned integer value.
 */
uint16_t sensirion_common_bytes_to_uint16_t(const uint8_t* bytes) {
    return (uint16_t)bytes[0] << 8 | (uint16_t)bytes[1];
}

/**
 * @brief Convert an array of bytes to a uint32_t value (big-endian).
 *
 * @param bytes Pointer to the array of bytes.
 * @return Converted 32-bit unsigned integer value.
 */
uint32_t sensirion_common_bytes_to_uint32_t(const uint8_t* bytes) {
    return (uint32_t)bytes[0] << 24 | (uint32_t)bytes[1] << 16 |
           (uint32_t)bytes[2] << 8 | (uint32_t)bytes[3];
}

/**
 * @brief Convert an array of bytes to an int16_t value (big-endian).
 *
 * @param bytes Pointer to the array of bytes.
 * @return Converted 16-bit signed integer value.
 */
int16_t sensirion_common_bytes_to_int16_t(const uint8_t* bytes) {
    return (int16_t)sensirion_common_bytes_to_uint16_t(bytes);
}

/**
 * @brief Convert an array of bytes to an int32_t value (big-endian).
 *
 * @param bytes Pointer to the array of bytes.
 * @return Converted 32-bit signed integer value.
 */
int32_t sensirion_common_bytes_to_int32_t(const uint8_t* bytes) {
    return (int32_t)sensirion_common_bytes_to_uint32_t(bytes);
}

/**
 * @brief Convert an array of bytes to a float value (big-endian).
 *
 * @param bytes Pointer to the array of bytes.
 * @return Converted floating-point value.
 */
float sensirion_common_bytes_to_float(const uint8_t* bytes) {
    union {
        uint32_t u32_value;
        float float32;
    } tmp;

    tmp.u32_value = sensirion_common_bytes_to_uint32_t(bytes);
    return tmp.float32;
}
/**
 * @brief Convert a uint32_t value to an array of bytes (big-endian).
 *
 * @param value Input uint32_t value.
 * @param bytes Pointer to the array where the bytes will be stored.
 */
void sensirion_common_uint32_t_to_bytes(const uint32_t value, uint8_t* bytes) {
    bytes[0] = value >> 24;
    bytes[1] = value >> 16;
    bytes[2] = value >> 8;
    bytes[3] = value;
}
/**
 * @brief Convert a uint16_t value to an array of bytes (big-endian).
 *
 * @param value Input uint16_t value.
 * @param bytes Pointer to the array where the bytes will be stored.
 */
void sensirion_common_uint16_t_to_bytes(const uint16_t value, uint8_t* bytes) {
    bytes[0] = value >> 8;
    bytes[1] = value;
}
/**
 * @brief Convert an int32_t value to an array of bytes (big-endian).
 *
 * @param value Input int32_t value.
 * @param bytes Pointer to the array where the bytes will be stored.
 */
void sensirion_common_int32_t_to_bytes(const int32_t value, uint8_t* bytes) {
    bytes[0] = value >> 24;
    bytes[1] = value >> 16;
    bytes[2] = value >> 8;
    bytes[3] = value;
}
/**
 * @brief Convert an int16_t value to an array of bytes (big-endian).
 *
 * @param value Input int16_t value.
 * @param bytes Pointer to the array where the bytes will be stored.
 */
void sensirion_common_int16_t_to_bytes(const int16_t value, uint8_t* bytes) {
    bytes[0] = value >> 8;
    bytes[1] = value;
}
/**
 * @brief Convert a float value to an array of bytes (big-endian).
 *
 * @param value Input floating-point value.
 * @param bytes Pointer to the array where the bytes will be stored.
 */
void sensirion_common_float_to_bytes(const float value, uint8_t* bytes) {
    union {
        uint32_t u32_value;
        float float32;
    } tmp;
    tmp.float32 = value;
    sensirion_common_uint32_t_to_bytes(tmp.u32_value, bytes);
}
/**
 * @brief Copy an array of bytes from source to destination.
 *
 * @param source Pointer to the source array.
 * @param destination Pointer to the destination array.
 * @param data_length Number of bytes to copy.
 */
void sensirion_common_copy_bytes(const uint8_t* source, uint8_t* destination,
                                 uint16_t data_length) {
    uint16_t i;
    for (i = 0; i < data_length; i++) {
        destination[i] = source[i];
    }
}
