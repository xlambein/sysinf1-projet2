#ifndef __reader_h__
#define __reader_h__

#include <stdio.h>

#include "factor_list.h"

typedef struct
{
    FILE * stream;
    const char * filename;
    //TODO: the rest
}
reader_starting_state_t;

void *reader(void *arg);

void find_prime_with_one_occurrence();

#endif

