#include "debounce.h"
#include "fsm.h"


enum { 
    EV_HIGH = 0,
    EV_LOW,
    EV_STABLE
};

static void act_start_count(void *ctx)
{
    ((debounce_t *)ctx)->count = 1U;
}

static void act_emit_pressed(void *ctx) {
    ((debounce_t *)ctx)->pending = DEBOUNCE_EVT_PRESSED;
}

static void act_emit_released(void *ctx) {
    ((debounce_t *)ctx)->pending = DEBOUNCE_EVT_RELEASED;
}

static const fsm_transition_t k_table[] = {
    // {curr_state, ev, mxt_state, action} & by default pressed button = HIGH
    {DEBOUNCE_IDLE,       EV_HIGH,   DEBOUNCE_DEBOUNCING, act_start_count}, // 1
    {DEBOUNCE_DEBOUNCING, EV_LOW,    DEBOUNCE_IDLE,       NULL},
    {DEBOUNCE_DEBOUNCING, EV_STABLE, DEBOUNCE_PRESSED,    act_emit_pressed}, // 2
    {DEBOUNCE_PRESSED,    EV_LOW,    DEBOUNCE_RELEASING,  act_start_count}, // 3
    {DEBOUNCE_RELEASING,  EV_HIGH,   DEBOUNCE_PRESSED,    NULL}, // 4 cycle repeats
    {DEBOUNCE_RELEASING,  EV_STABLE, DEBOUNCE_IDLE,       act_emit_released},
};

void debounce_init(debounce_t *d, uint32_t stable_samples) {
    d->stable_samples = (stable_samples == 0U) ? 1U : stable_samples;
    d->count = 0U;
    d->pending = DEBOUNCE_NONE;
    // init statet is in idle
    fsm_init(&d->fsm, k_table, sizeof k_table / sizeof k_table[0], DEBOUNCE_IDLE, d);
}

debounce_event_t debounce_update(debounce_t *d, bool raw_pressed) {
    d->pending = DEBOUNCE_NONE;
    // get curr state & dispatch event based on input & get new state
    const int prev = fsm_state(&d->fsm);
    fsm_dispatch(&d->fsm, raw_pressed ? EV_HIGH : EV_LOW);
    const int now = fsm_state(&d->fsm);

    // generally if new state is 4 debouncing we count stable samples 
    // (4 which now == prev) -> after we change state 2 stable
    if (now == DEBOUNCE_DEBOUNCING || now == DEBOUNCE_RELEASING) {
        if (now == prev) {
            d->count++;
        }
        if (d->count >= d->stable_samples) {
            fsm_dispatch(&d->fsm, EV_STABLE);
        }
    }

    // its a little brainfuck but pending attrib is changed by callback funcs e.g. 
    // {DEBOUNCE_DEBOUNCING, EV_STABLE, DEBOUNCE_PRESSED, act_emit_pressed}
    return d->pending;  
}

debounce_state_t debounce_state(const debounce_t *d) {
    return (debounce_state_t)fsm_state(&d->fsm);
}