#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include "frame_parser.h"
#include "transport.h"
#include "transport_tty.h"


// @brief callback 2 be called upon receiving frame 4m MCU
static void on_frame(const uint8_t *payload, uint8_t len, void *user) {
    (void)user;
    printf("frame from mcu, len=%u payload=\"%.*s\"\n", len, len, (const char *)payload);
    fflush(stdout);
}

int main(int argc, char **argv) {
    const char *path = (argc > 1) ? argv[1] : "/dev/ttyACM0";
    // defining transport abstraction 
    transport_t transport;

    // init transport abstarction /w tty transport
    if (transport_tty_open(path, 115200U, &transport) < 0) {
        perror("open tty");

        return 1;
    }

    uint8_t parser_buf[64];
    frame_parser_t parser;
    // init frame parser
    frame_parser_init(&parser, parser_buf, sizeof(parser_buf), on_frame, NULL);
    uint8_t rx[64];
    
    for(;;) {
        // reading data 4m tty 
        int count = transport.read(transport.ctx, rx, sizeof(rx));

        if (count < 0) {
            perror("read");
            break;
        }

        // feeding data 2 frame parser, which will call on_frame callback if a complete frame is received
        for (int i = 0; i < count; i++) {
            frame_parser_feed(&parser, rx[i]);
        }

        usleep(1000);
    }

    transport_tty_close(&transport);

    return 0;
}