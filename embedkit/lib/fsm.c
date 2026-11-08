#include "fsm.h"


void fsm_init(fsm_t *f, const fsm_transition_t *table, size_t table_len, int initial_state, void *ctx) {
    f->table = table;
    f->table_len = table_len;
    f->state = initial_state;
    f->ctx = ctx;
}

bool fsm_dispatch(fsm_t *f, int event) {
    for (size_t i = 0; i < f->table_len; i++) {
        // find row 4 (current state, event) & we have to have address thats why &f->table[i] not f->table[i]
        // ewually we could do f->table + i
        const fsm_transition_t *t = &f->table[i];

        // if state and event match with curr fsm state & new event, 
        // set 2 next state & call action 
        if (t->state == f->state && t->event == event) {
            f->state = t->next_state;

            // call the func with transition 4m 1 state 2 another
            if (t->action != NULL) {
                t->action(f->ctx);
            }

            return true;
        }
    }
    // if no transition found...
    return false;
}