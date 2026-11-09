#ifndef FSM_H
#define FSM_H

#include <stddef.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif

void fsm_action(void *ctx);
typedef __typeof__(fsm_action) *fsm_action_fn;

// responsible for transitioning 4m state X to state Z when event Y occurs, and executing action W.
typedef struct {
    int state;
    int event;
    int next_state;
    fsm_action_fn action;
} fsm_transition_t;

typedef struct {
    const fsm_transition_t *table;  // fsm_transition_t is just a row
    size_t table_len;
    int state;
    void *ctx;
} fsm_t;

// @brief initializes fsm
void fsm_init(fsm_t *f, const fsm_transition_t *table, size_t table_len,
              int initial_state, void *ctx);

// @brief returns true if transition found. 
// order:
// 1. find row for (current state, event)
// 2. set new state
// 3. call action (in new state)
bool fsm_dispatch(fsm_t *f, int event);

// @brief returns current state
static inline int fsm_state(const fsm_t *f) {
    return f->state;
}

#ifdef __cplusplus 
}
#endif

#endif // FSM_H