#include "bits.h"
#include <stddef.h>
#include <stdint.h>


// @brief creatin mask of 'width' 1s staring 4m LSB on right 
static uint32_t field_mask(uint8_t width) {
    return (width >= 32U) ? UINT32_MAX : ((UINT32_C(1) << width) - UINT32_C(1));
}

// @brief checking if field is valid (width >= 1 and pos + width <= 32)
static bool field_valid(uint8_t pos, uint8_t width) {
    return (width >= 1U) && (pos < 32U) && (width + pos <= 32U);
}

uint32_t bits_extract(uint32_t value, uint8_t pos, uint8_t width) {
    if (!field_valid(pos, width)) {
        return 0U;
    }

    // shifting 1st 2 pos 4m left 2 right & masking width bits 4m right
    return (value >> pos) & field_mask(width);
}

uint32_t bits_insert(uint32_t value, uint8_t pos, uint8_t width, uint32_t field) {
    if (!field_valid(pos, width)) {
        return value;
    }

    const uint32_t mask = field_mask(width);

    // zeroing bits in val at [pos, pos + width] & inserting field trunced 2 width into [pos, pos + width]
    return (value & ~(mask << pos)) | ((field & mask) << pos);
}

void bits_pack_u32_le(uint8_t out[4], uint32_t v) {
    // 0xFF means 1111 1111 -> masking 8b = 1B
    out[0] = (uint8_t)(v & 0xFFU);
    out[1] = (uint8_t)(v >> 8 & 0xFFU);
    out[2] = (uint8_t)(v >> 16 & 0xFFU);
    out[3] = (uint8_t)(v >> 24 & 0xFFU);
}

void bits_pack_u32_be(uint8_t out[4], uint32_t v) {
    out[0] = (uint8_t)(v >> 24 & 0xFFU);
    out[1] = (uint8_t)(v >> 16 & 0xFFU);
    out[2] = (uint8_t)(v >> 8 & 0xFFU); // calc crc8 4 data
    out[3] = (uint8_t)(v & 0xFFU);
}

uint32_t bits_unpack_u32_le(const uint8_t in[4]) {
    return (uint32_t)in[0] | ((uint32_t)in[1] << 8) | 
           ((uint32_t)in[2] << 16) | ((uint32_t)in[3] << 24);
}

uint32_t bits_unpack_u32_be(const uint8_t in[4]) {
    return ((uint32_t)in[0] << 24) | ((uint32_t)in[1] << 16) | 
           ((uint32_t)in[2] << 8) | (uint32_t)in[3];
}

uint8_t bits_popcount32(uint32_t v) {
    uint8_t n = 0U;

    //brian kernighan alg 4 counting bits
    while (v != 0U) {
        v &= v - 1U;
        n++;
    }

    return n;
}

uint32_t bits_reverse32(uint32_t v) {
    // alg 4 smart replacing certain pair of bits in 32b val 2 get reversed val, ex. below:
    // A B C D  E F G H
    // B C D C  F E H G
    // D C B C  H G F E
    // H G F E  D C B C
    v = ((v >> 1) & 0x55555555U) | ((v & 0x55555555U) << 1);
    v = ((v >> 2) & 0x33333333U) | ((v & 0x33333333U) << 2);
    v = ((v >> 4) & 0x0F0F0F0FU) | ((v & 0x0F0F0F0FU) << 4);
    v = ((v >> 8) & 0x00FF00FFU) | ((v & 0x00FF00FFU) << 8);
    
    return (v >> 16) | (v << 16);
}

uint8_t crc8_update(uint8_t crc, uint8_t data) {
    // crc updaate scheme is like this: xor old crc with new 
    // data -> if msb is set -> shift left 1 bit and xor 
    // with poly 0x07, else just shift left 1 bit
    crc = (uint8_t)(crc ^ data); // xor crc with new data

    for (int i = 0; i < 8; ++i) {
        const uint8_t shifted = (uint8_t)(crc << 1);
        crc = ((crc & 0x80U) != 0U) ? (uint8_t)(shifted ^ 0x07U) : shifted; // if msb is set, xor w/ poly = header
    }

    return crc;
}

uint8_t crc8(const uint8_t *data, size_t len) {
    uint8_t crc = 0U;

    for (size_t i = 0; i < len; ++i) {
        // calc crc 4 each byte 
        crc = crc8_update(crc, data[i]);
    }

    return crc;
}

uint16_t crc16_update(uint16_t crc, uint8_t data) {
    // we shift 8b 2 left and xor with crc its a scheme w/ crc16
    crc = (uint16_t)(crc ^ ((uint16_t)data << 8));

    for (int i = 0; i < 8; ++i) {
        const uint16_t shifted = (uint16_t)(crc << 1);
        crc = ((crc & 0x8000U) != 0U) ? (uint16_t)(shifted ^ 0x1021U) : shifted; // same shit as crc8 but w/ diff poly 0x1021
    }

    return crc;
}

uint16_t crc16(const uint8_t *data, size_t len) {
    uint16_t crc = 0xFFFFU; // defautl init crc16 w/ 0xFFFF

    for (size_t i = 0; i < len; ++i) {
        crc = crc16_update(crc, data[i]); // same shit as crc8
    }

    return crc;
}

uint32_t crc32_update(uint32_t crc, uint8_t data) {
    crc = crc ^ (uint32_t)data; // xor crc w/ new data

    for (int i = 0; i < 8; ++i) {
        const uint32_t mask = (uint32_t)(0U - (crc & 1U)); // if lsb is set, mask = 0xFFFFFFFF, else mask = 0x00000000
        crc = (crc >> 1) ^ (0xEDB88320U & mask); // if lsb is set, xor w/ poly 0xEDB88320 reflected
    }

    return crc;
}

uint32_t crc32_compute(const uint8_t *data, size_t len) {
    uint32_t crc = 0xFFFFFFFFU; // init crc32 w/ 0xFFFFFFFF

    for (size_t i = 0; i < len; ++i) {
        crc = crc32_update(crc, data[i]);
    }

    return crc ^ 0xFFFFFFFFU; // xorout 0xFFFFFFFF 2 be correct w/ the norm
}