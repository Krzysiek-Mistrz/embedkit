#define _DEFAULT_SOURCE
#include "transport_tty.h"

#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

// @brief func 2 convert baud rate 2 speed val
static speed_t baud_to_speed(unsigned baud) {
    switch (baud) {
        case 9600U:     return B9600;
        case 57600U:    return B57600;
        case 115200U:   return B115200;
        case 230400U:   return B230400;
        default:        return B115200;
    }
}

// @brief func 2 write data 2 tty fd
static int tty_write(void *ctx, const uint8_t *data, size_t len) {
    int fd = *(int *)ctx; // cast context 2 int* and derefernce it back 2 int 4 fd
    ssize_t written = write(fd, data, len);

    if (written < 0 && (errno == EAGAIN || errno == EINTR)) {
        return 0;
    }

    return (int)written;
}

static int tty_read(void *ctx, uint8_t *buf, size_t len) {
    int fd = *(int *)ctx;
    ssize_t count = read(fd, buf, len);

    if (count < 0 && (errno == EAGAIN || errno == EINTR)) {
        return 0;
    }

    return (int)count;
}

static int tty_configure(int fd, unsigned baud) {
    struct termios tio; // create termios struct 2 hold tty config

    if (tcgetattr(fd, &tio) != 0) {
        return -1;
    }

    cfmakeraw(&tio); // set tty 2 raw mode
    cfsetspeed(&tio, baud_to_speed(baud));
    tio.c_cc[VMIN] = 0; // set min num of bytes 2 read 2 0
    tio.c_cc[VTIME] = 0; // set read timeout 2 0

    return tcsetattr(fd, TCSANOW, &tio); // apply config of tio 2 tty fd
}

int transport_tty_open(const char *path, unsigned int baud, transport_t *out) {
    static int fd_storage;

    int fd = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK); // open tty device file in read/write mode, no control term, non blocking

    if (fd < 0) {
        return -1;
    }
    if (tty_configure(fd, baud) != 0) {
        close(fd);
        return -1;
    }

    fd_storage = fd;
    out->write = tty_write;
    out->read = tty_read;
    // store fd in static storage and set cont 2 point 
    // 2 it so that read/write funcs can access it
    out->ctx = &fd_storage; 

    return fd;
}

void transport_tty_close(const transport_t *t) {
    // cast context 2 int* and dereference it back 2 
    // int 4 fd and close it
    close(*(int *)t->ctx);
}