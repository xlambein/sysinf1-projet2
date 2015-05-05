#ifndef __util_h__
#define __util_h__

/**
 * This file contains all the global variables used throughout the program,
 * as well as many utility functions.
 */

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>

#include "factor_list.h"

typedef unsigned long long ull;

// The semaphore representing the number of elements in the waiting list
extern sem_t sem_full,
//TODO
             sem_start,
//TODO
             sem_finish,
//TODO
             sem_handshake;

// The mutex protecting the global state of the program (waiting_list,
// prime_list, readers_active, to_fact, found, TODO)
extern pthread_mutex_t mut_state,
//TODO
                       mut_factorizers;

// Boolean indicating whether a solution was found
extern bool found;

// The number to factorize used in the factorizers
extern factor_t to_fact;

// The waiting list, containing numbers to be factorized
extern factor_list_t *waiting_list,
// The prime list, containing prime factors computed by the factorizers
                     *prime_list;

//TODO
extern int num_factorizers,
// The number of active reader threads
           readers_active,
//TODO
           factorizer_meeting;

// Starts the timer that measures the execution time of the program
int start_timer();

// Stops the timer and computes the elapsed time
int stop_timer(double * elapsed_time);

// Prints the solution (represented by factor)
void print_solution(const factor_t *factor);

// Divide one factor by another as many times as possible
void divide_as_much_as_possible(factor_t *to_divide, factor_t *divisor);

#endif

