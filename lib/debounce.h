#ifndef DEBOUNCE_H
#define DEBOUNCE_H

#include "fsm.h"
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DEBOUNCE_IDLE = 0,
    DEBOUNCE_DEBOUNCING,
    DEBOUNCE_PRESSED,
    DEBOUNCE_RELEASING
} debounce_state_t;

typedef enum {
    DEBOUNCE_NONE = 0,
    DEBOUNCE_EVT_PRESSED,
    DEBOUNCE_EVT_RELEASED
} debounce_event_t;

typedef struct {
    fsm_t fsm;
    uint32_t stable_samples;
    uint32_t count;
    debounce_event_t pending;
} debounce_t;

// @brief 4 init the debounce state machine
void debounce_init(debounce_t *d, uint32_t stable_samples);

// @brief updaing debounce state based on raw input
debounce_event_t debounce_update(debounce_t *d, bool raw_pressed);

// @brief get curr debounce state
debounce_state_t debounce_state(const debounce_t *d);

#ifdef __cplusplus
}
#endif

#endif // DEBOUNCE_H