#include "transport_mem.h"
#include <stddef.h>
#include <stdint.h>


// @brief func 2 write data 2 ringbuf
static int mem_write(void *ctx, const uint8_t *data, size_t len) {
    transport_mem_t *mem = ctx;
    size_t written = 0U;

    while (written < len && ringbuf_push(&mem->rb, data[written])) {
        written++;
    }

    return (int)written;
}

// @brief func 2 read data 4m ringbuf
static int mem_read(void *ctx, uint8_t *buf, size_t len) {
    transport_mem_t *mem = ctx;
    size_t count = 0U;

    // reading till the buffer is empty or we have read 
    // the requested length, where wer savbing data in buffer
    while (count < len && ringbuf_pop(&mem->rb, &buf[count])) {
        count++;
    }

    return (int)count;
}

int transport_mem_init(transport_mem_t *mem, transport_t *out) {
    if (!ringbuf_init(&mem->rb, mem->storage, TRANSPORT_MEM_CAPACITY)) {
        return -1;
    }

    out->write = mem_write;
    out->read = mem_read;
    out->ctx = mem;

    return 0;
}