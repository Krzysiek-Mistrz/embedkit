#include "frame_parser.h"
#include <stddef.h>
#include <stdint.h>
#include "bits.h"


void frame_parser_init(frame_parser_t *p, uint8_t *buf, size_t buf_size,
                       frame_cb_t on_frame, void *user) {
    p->state = FRAME_WAIT_START;
    p->buf = buf;
    p->buf_size = buf_size;
    p->expected_len = 0U;
    p->received = 0U;
    p->crc = 0U;
    p->on_frame = on_frame;
    p->user = user;
    p->frames_ok = 0U;
    p->crc_errors = 0U;
    p->too_long_errors = 0U;
}

void frame_parser_feed(frame_parser_t *p, uint8_t byte) {
    switch (p->state) {
        // ignore everything till we get a start byte
        case FRAME_WAIT_START:
            if (byte == FRAME_START_BYTE) {
                p->state = FRAME_WAIT_LEN;
            }
            
            break;

        // got start byte, now wait 4 length
        case FRAME_WAIT_LEN:
            if ((size_t)byte > p->buf_size) {
                p->too_long_errors++;
                p->state = FRAME_WAIT_START;
                
                break;
            }

            p->expected_len = byte;
            p->received = 0U;
            p->crc = crc8_update(0U, byte);
            // if length is 0 -> we can skip to CRC check else we need 2 wait 4 payload
            p->state = (byte == 0U) ? FRAME_WAIT_CRC : FRAME_WAIT_PAYLOAD;

            break;

        // got length, now wait 4 payload bytes
        case FRAME_WAIT_PAYLOAD:
            p->buf[p->received++] = byte;
            p->crc = crc8_update(p->crc, byte);

            if (p->received == p->expected_len) {
                p->state = FRAME_WAIT_CRC;
            }

            break;

        // got payload, now wait 4 crc B
        case FRAME_WAIT_CRC:
            if (byte == p->crc) {
                p->frames_ok++;

                // if we got some function to call on frame received...
                if (p->on_frame != NULL) {
                    p->on_frame(p->buf, p->expected_len, p->user);
                }
            } else {
                p->crc_errors++;
            }

            // back 2 beginning
            p->state = FRAME_WAIT_START;

            break;
    }
}

size_t frame_encode(const uint8_t *payload, uint8_t len, uint8_t *out, size_t out_size) {
    const size_t total = (size_t)len + 3U; // (1) start B + (1) len + (len) payload + (1) crc

    // check if out buffer is big enough
    if (out_size < total) {
        return 0U;
    }

    // encoding frame...
    out[0] = FRAME_START_BYTE;
    out[1] = len;
    uint8_t crc = crc8_update(0U, len);

    for (size_t i = 0; i < len; ++i) {
        out[2 + i] = payload[i];
        crc = crc8_update(crc, payload[i]);
    }

    out[2 + (size_t)len] = crc;
    
    return total;
}