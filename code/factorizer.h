#ifndef _FACTORIZER_H
#define _FACTORIZER_H

typedef struct
{
    factor_list_t *waiting_list;
    factor_t *to_fact;
    uint64_t start, step;
}
factorizer_starting_state_t;

void *factorize(void *starting_state);

#endif
