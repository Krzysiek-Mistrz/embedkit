/*
* @file bits.h
* @brief bit operations, bit packing, crc8
* bit numeration: 0 - LSB, 31 - MSB
*/

#ifndef BITS_H
#define BITS_H

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif

// @brief setting certain bit in 32v val
static inline uint32_t bits_set(uint32_t v, uint8_t bit) {
    return (bit < 32U) ? (v | (UINT32_C(1) << bit)) : v;
}

// @brief clearing certain bit in 32b val
static inline uint32_t bits_clear(uint32_t v, uint8_t bit) {
    return (bit < 32U) ? (v & ~(UINT32_C(1) << bit)) : v;
}

// @brief testing certain b in 32b val if diff than 0
static inline bool bits_test(uint32_t v, uint8_t bit) {
    return (bit < 32U) && (((v >> bit) & UINT32_C(1)) != 0U);
}

// @brief toggling (0->1 / 1->0) certain bit in 32b val
static inline uint32_t bits_toggle(uint32_t v, uint8_t bit) {
    return (bit < 32U) ? (v ^ (UINT32_C(1) << bit)) : v;
}

// @brief extracting field of width 'width' from 32b val starting from bit 'pos' from LSB
uint32_t bits_extract(uint32_t value, uint8_t pos, uint8_t width);

// @brief inserting field truc 2 'width' into [pos, pos + width] of val
uint32_t bits_insert(uint32_t value, uint8_t pos, uint8_t width, uint32_t field);

// @brief packing 32b val into 4 bytes in little endian (lsb on 0 idx) order
void bits_pack_u32_le(uint8_t out[4], uint32_t v);

// @brief packing 32b val into 4 bytes in big endian (msb on 0 idx) order
void bits_pack_u32_be(uint8_t out[4], uint32_t v);

// @brief unpacking 4 bytes in little endian (lsb on 0 idx) order into 32b val
uint32_t bits_unpack_u32_le(const uint8_t in[4]);

// @brief unpacking 4 bytes in big endian (msb on 0 idx) order into 32b val
uint32_t bits_unpack_u32_be(const uint8_t in[4]);

// @brief counting number of bits set in 32b val
uint8_t bits_popcount32(uint32_t v);

// @brief reverse bits...
uint32_t bits_reverse32(uint32_t v);

// @brief updating crc8 with new data
uint8_t crc8_update(uint8_t crc, uint8_t data);

// @brief calc crc8 4 data
uint8_t crc8(const uint8_t *data, size_t len);

// @brief updating crc16 w/ new data poly 0x1021 & init 0xFFFF
uint16_t crc16_update(uint16_t crc, uint8_t data);

// @brief calc crc16 4 data
uint16_t crc16(const uint8_t *data, size_t len);

// @brief updating crc32 w/ new data poly 0xEDB88320 reflected, init 0xFFFFFFFF, xorout 0xFFFFFFFF
uint32_t crc32_update(uint32_t crc, uint8_t data);

// @brief calc crc32 4 data
uint32_t crc32_compute(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif // BITS_H