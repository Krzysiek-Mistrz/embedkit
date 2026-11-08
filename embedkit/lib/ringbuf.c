#include "ringbuf.h"


#define RB_LOAD_RELAXED(p) __atomic_load_n((p), __ATOMIC_RELAXED)
#define RB_LOAD_ACQUIRE(p) __atomic_load_n((p), __ATOMIC_ACQUIRE)
#define RB_STORE_RELEASE(p, v) __atomic_store_n((p), (v), __ATOMIC_RELEASE)

bool ringbuf_init(ringbuf_t *rb, uint8_t *storage, size_t capacity) {
    if (rb == NULL || storage == NULL) {
        return false;
    }
    // if cap is 0 || cap > 2^31 0x8... is half of 2^32 e.g. 4 8b:
    // 0x80 = 1000 0000 = 128 wheras 0xFF = 1111 1111 = 255
    if (capacity == 0u || capacity > UINT32_C(0x80000000)) {
        return false;
    }
    // not power of 2 -> e.g. 3 = 0011, 3-1 = 0010, 0011 & 0010 = 0010 != 0
    if ((capacity & (capacity - 1U)) != 0U) { 
        return false;
    }

    rb->buf = storage;
    rb->mask = (uint32_t)capacity - 1U; // e.g. 1000 gives 0111
    rb->head = 0U;
    rb->tail = 0U;

    return true;
}

bool ringbuf_push(ringbuf_t *rb, uint8_t byte) {
    // atomic gettin addr of head & tail
    const uint32_t head = RB_LOAD_RELAXED(&rb->head); 
    const uint32_t tail = RB_LOAD_ACQUIRE(&rb->tail);

    if ((uint32_t)(head - tail) > rb->mask) {
        return false; // ringbuf is full e.g. head - tail = 8 = capacity wherase mask = 8 - 1 = 7
    }
     
    // smart calc of index e.g. head = 8, mask = 15, 1000 & 1111 = 1000 = 8 
    // and if head = 16, mask = 15, 10000 & 1111 = 0000 = 0 
    // and if head = 17, mask = 15, 10001 & 1111 = 0001 = 1 
    rb->buf[head & rb->mask] = byte; 
    RB_STORE_RELEASE(&rb->head, head + 1U); // head++ but release so that buf is saved completely before head is updated
    
    return true;
}

bool ringbuf_pop(ringbuf_t *rb, uint8_t *out) {
    const uint32_t tail = RB_LOAD_RELAXED(&rb->tail);
    const uint32_t head = RB_LOAD_ACQUIRE(&rb->head);

    if (head == tail) {
        return false; // ringbuf is empty
    }

    *out = rb->buf[tail & rb->mask]; // same shit logic as in push
    RB_STORE_RELEASE(&rb->tail, tail + 1U); // tail++ but release so that buf is read completely before tail is updated

    return true;
}

size_t ringbuf_count(const ringbuf_t *rb) {
    const uint32_t head = RB_LOAD_ACQUIRE(&rb->head);
    const uint32_t tail = RB_LOAD_ACQUIRE(&rb->tail);

    return (size_t)(head - tail); // head - tail = count of elements in ringbuf
}

size_t ringbuf_capacity(const ringbuf_t *rb) {
    return (size_t)rb->mask + 1U; // mask = capacity - 1
}

bool ringbuf_is_empty(const ringbuf_t *rb) {
    return ringbuf_count(rb) == 0U;
}

bool ringbuf_is_full(const ringbuf_t *rb) {
    return ringbuf_count(rb) == ringbuf_capacity(rb);
}