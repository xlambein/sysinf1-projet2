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
// The signal sent by the main loop to the factorizers to begin an execution
// of their factorization loop
             sem_start,
// The signal sent by the last factorizer to finish factorizing to initiate
// the handshaking process; it is forwarded by every factorizer when to allow
// the next one to pass the barrier
             sem_handshake,
// The signal sent by the last factorizer to handshake back to the main loop to
// indicate all the factorizers have finished and are ready to start back
             sem_finish;

// The mutex protecting the global state of the program (waiting_list,
// prime_list, readers_active, and found)
extern pthread_mutex_t mut_state,
// The mutex protecting to_fact
                       mut_to_fact,
// The mutex protecting the number of factorizers at the barrier
                       mut_factorizers;

// Boolean indicating whether a solution was found
extern bool found;

// The number to factorize used in the factorizers
extern factor_t to_fact;

// The waiting list, containing numbers to be factorized
extern factor_list_t *waiting_list,
// The prime list, containing prime factors computed by the factorizers
                     *prime_list;

// The number of factorizer threads allowed by the command-line argument
extern int num_factorizers,
// The number of active reader threads
           readers_active,
// The number of factorizer currently at the barrier
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

