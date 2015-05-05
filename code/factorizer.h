#ifndef _FACTORIZER_H
#define _FACTORIZER_H

#include <stdint.h>

/**
 * Constains the starting state of a factorizer. It indicates which number to
 * start with and the step that it has to make between tests.
 *
 * For example, with start=3 and step=4, it will try to divide by 3, 7, 11, etc.
 */
typedef struct
{
    uint64_t start, step;
}
factorizer_starting_state_t;

// Thread that factorizes numbers
void *factorizer(void *starting_state);

#endif
