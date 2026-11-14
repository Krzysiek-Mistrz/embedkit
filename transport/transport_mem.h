#ifndef TRANSPORT_MEM_H
#define TRANSPORT_MEM_H

#include "transport.h"
#include "ringbuf.h"


#ifdef __cplusplus
extern "C" {
#endif

#define TRANSPORT_MEM_CAPACITY 256U

/*
@brief loopback transp in mem -> its for testing purposes
*/
typedef struct {
    ringbuf_t rb;
    uint8_t storage[TRANSPORT_MEM_CAPACITY];
} transport_mem_t;

/*
@brief prepares loopback & fills out transport handle
*/
int transport_mem_init(transport_mem_t *mem, transport_t *out);

#ifdef __cplusplus
}
#endif

#endif // TRANSPORT_MEM_H
