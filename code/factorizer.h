#ifndef _FACTORIZER_H
#define _FACTORIZER_H

#include <stdint.h>

#include "factor_list.h"

typedef long long ll;

typedef struct
{
    uint32_t start, step;
}
factorizer_starting_state_t;

void *factorize(void *starting_state);

#endif
