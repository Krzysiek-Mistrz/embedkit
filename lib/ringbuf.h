#ifndef RINGBUF_H
#define RINGBUF_H

#include <sys/types.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>


#ifdef __cplusplus 
extern "C" {
#endif

// @brief ring buff 4 single prod, single cons
typedef struct {
    uint32_t head;
    uint32_t tail;
    uint32_t mask;
    uint8_t *buf;
} ringbuf_t;

// @brief initializes ring buf on mem "storage" with capacity B
// returns false if storage is null
bool ringbuf_init(ringbuf_t *rb, uint8_t *storage, size_t capacity);

// @brief pushes B 2 ringbuf & returns false if full
bool ringbuf_push(ringbuf_t *rb, uint8_t byte);

// @brief pops B from ringbuf & returns false if empty
bool ringbuf_pop(ringbuf_t *rb, uint8_t *out);

// @brief returns count of B in ringbuf
size_t ringbuf_count(const ringbuf_t *rb);

// @brief returns capacity of ringbuf
size_t ringbuf_capacity(const ringbuf_t *rb);

// @brief returns true if ringbuf is empty
bool ringbuf_is_empty(const ringbuf_t *rb);

// @brief returns 1 if full
bool ringbuf_is_full(const ringbuf_t *rb);

#ifdef __cplusplus
}
#endif

#endif // RINGBUF_H