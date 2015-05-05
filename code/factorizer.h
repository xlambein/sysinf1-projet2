#ifndef _FACTORIZER_H
#define _FACTORIZER_H

#include <stdint.h>

#include "factor_list.h"

typedef long long ll;

typedef struct
{
    uint64_t start, step;
}
factorizer_starting_state_t;

void *factorizer(void *starting_state);

#endif
