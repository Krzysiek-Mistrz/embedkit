#include "transport.h"


// @brief func 2 write all data 2 transport via transport_t
int transport_write_all(const transport_t *t, const uint8_t *data, size_t len) {
    size_t sent = 0U;

    while (sent < len) {
        int written = t->write(t->ctx, data + sent, len - sent);

        if (written <= 0) {
            return -1;
        }

        sent += (size_t)written;
    }

    return 0;
}