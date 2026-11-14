#ifndef TRANSPORT_TTY_H
#define TRANSPORT_TTY_H

#include "transport.h"


#ifdef __cplusplus
extern "C" {
#endif

/*
@brief transport over serial dev eg. /dev/ttyACM0 port
is set 2 raw mode, 8N1, nonblocking read, returns fd or -1
also utilizes transport_t pipe obj w/ read & write funcs -> codec
*/
int transport_tty_open(const char *path, unsigned baud, transport_t *out);

// @brief just closes fd...
void transport_tty_close(const transport_t *t);

#ifdef __cplusplus
}
#endif

#endif // TRANSPORT_TTY_H