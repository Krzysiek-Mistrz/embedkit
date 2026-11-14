#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "frame_parser.h"
#include "transport.h"
#include "transport_mem.h"


// @brief on frame callback func 4 frame_parser
static void on_frame(const uint8_t *payload, uint8_t len, void *user) {
    (void)user;
    printf("frame received, len=%u payload=\"%.*s\"\n", len, len, (const char *)payload);
}

int main(int argc, char **argv) {
    // first we create init a transport_mem_t and transport_t
    transport_mem_t mem;
    transport_t transport;
    // transport_mem_t is conrete implementation of transport_t interface
    transport_mem_init(&mem, &transport);

    uint8_t parser_buf[64];
    frame_parser_t parser;
    // om_frame callback automaticaly called upon receiving proper frame by frame_parser_feed
    frame_parser_init(&parser, parser_buf, sizeof(parser_buf), on_frame, NULL);

    const char *message = "temp=23.5";
    uint8_t frame[64];
    // frame encode packages messg into proper frame with crc, header & shit
    size_t frame_len = frame_encode((const uint8_t *)message, (uint8_t)strlen(message), frame, sizeof(frame));

    // write frame to transport, which is memory buffer in this case
    transport_write_all(&transport, frame, frame_len);

    uint8_t rx[16];
    int count;

    // read from transport & feed to frame_parser
    while ((count = transport.read(transport.ctx, rx, sizeof(rx))) > 0) {
        for (int i = 0; i < count; i++) {
            frame_parser_feed(&parser, rx[i]);
        }
    }

    printf("ok=%u crc_err=%u\n", parser.frames_ok, parser.crc_errors);
    return 0;
}