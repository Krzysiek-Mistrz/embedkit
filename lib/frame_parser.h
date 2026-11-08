/*
 * @file frame_parser.h
 * @brief byte 4 byte frame parser w/ crc implemented as fsm
 * frame format:
 *   +------+-----+---------------+------+
 *   | 0x7E | len | payload[len]  | crc8 |
 *   +------+-----+---------------+------+
 */

#ifndef FRAME_PARSER_H
#define FRAME_PARSER_H

#include <stdint.h>
#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif

#define FRAME_START_BYTE 0x7Eu

// callback type 2 func 4 frame received
// old times... -> typedef void (*frame_cb_t)(const uint8_t *payload, uint8_t len, void *user);
void frame_cb(const uint8_t *payload, uint8_t len, void *user);
typedef typeof(frame_cb)* frame_cb_t;

typedef enum {
    FRAME_WAIT_START = 0,
    FRAME_WAIT_LEN,
    FRAME_WAIT_PAYLOAD,
    FRAME_WAIT_CRC,
} frame_state_t;

typedef struct {
    frame_state_t state;
    uint8_t *buf;
    uint8_t buf_size;
    uint8_t expected_len;
    size_t received;
    uint8_t crc;
    frame_cb_t on_frame;
    void *user;
    uint32_t frames_ok;
    uint32_t crc_errors;
    uint32_t too_long_errors;
} frame_parser_t;

// @brief 4 frame parser fsm init
void frame_parser_init(frame_parser_t *p, uint8_t *buf, size_t buf_size, 
                       frame_cb_t on_frame, void *user);

// @brief feed byte 2 the frame parser
void frame_parser_feed(frame_parser_t *p, uint8_t byte);

// @brief creates frame in out
size_t frame_encode(const uint8_t *payload, uint8_t len, uint8_t *out, size_t out_size);

#ifdef __cplusplus
}
#endif

#endif // FRAME_PARSER_H