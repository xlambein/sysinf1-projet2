#ifndef __reader_h__
#define __reader_h__

#include <stdio.h>

typedef struct
{
    FILE * stream;
    //TODO: the rest
}
reader_starting_state_t;

void *reader(void *arg);

#endif

