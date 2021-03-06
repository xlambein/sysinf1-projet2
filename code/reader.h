#ifndef __reader_h__
#define __reader_h__

/**
 * This file defines the thread function (and its subroutines) used to read
 * input numbers from a file decriptor and add them to the waiting list.
 */

#include <stdio.h>

#include "factor_list.h"

// Structure used to pass arguments to the thread function reader()
typedef struct
{
    int fd;                // A file descriptor
    const char * filename; // The name of the file associated
}
reader_starting_state_t;

// Thread function that reads the input numbers from a file descriptor and
// adds them to the waiting list
void *reader(void *arg);

#endif

