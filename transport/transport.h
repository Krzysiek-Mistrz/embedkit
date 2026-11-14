#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <stddef.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef int (*transport_write_fn)(void *ctx, const uint8_t *data, size_t len);
typedef int (*transport_read_fn)(void *ctx, uint8_t *buf, size_t len);

/*
@brief byte pipe obj w/ read & write funcs -> codec 
doesnt give a shit about what iface behind(uart, tcp, ...)
*/ 
typedef struct {
    transport_write_fn write;
    transport_read_fn read;
    void *ctx;
} transport_t;

/*
@brief writes all B through transport_t & returns 0 
on success or -1 on failure
*/
int transport_write_all(const transport_t *t, const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif // TRANSPORT_H