#include "util.h"

#include <stdio.h>
#include <sys/time.h>
#include <inttypes.h>

sem_t sem_full, sem_start, sem_finish, sem_handshake;
pthread_mutex_t mut_state, mut_factorizers;
bool found = false;
factor_t to_fact = {0, 0, NULL};
factor_list_t *waiting_list, *prime_list;
int num_factorizers = 1, readers_active = 0, factorizer_meeting = 0;

// The timeval used in start_timer and stop_timer
static struct timeval timer_tv_start;

/**
 * Initializes timer_tv_start to the current time.
 *
 * @returns: 0 for success, or -1 for failure (in which case errno is set
 * appropriately for the function gettimeofday).
 */
int start_timer()
{
    return gettimeofday(&timer_tv_start, NULL);
}

/**
 * Compares timer_tv_start with the current time and sets the difference in
 * elapsed_time.
 *
 * @elapsed_time: a non-NULL pointer to a double that will be set to the
 * elapsed time, in seconds
 *
 * @returns: 0 for success, or -1 for failure (in which case errno is set
 * appropriately for the function gettimeofday).
 */
int stop_timer(double * elapsed_time)
{
    struct timeval timer_tv_stop;
    int retval = gettimeofday(&timer_tv_stop, NULL);
    if (!retval)
    {
        long diff = (timer_tv_stop.tv_usec + 1e6 * timer_tv_stop.tv_sec)
            - (timer_tv_start.tv_usec + 1e6 * timer_tv_start.tv_sec);
        *elapsed_time = ((double) diff)/1e6;
    }
    
    return retval;
}

/**
 * Prints the solution given by factor, according to the specifications given
 * in projet.pdf.
 *
 * @factor: a pointer to the factor_t structure containing the solution and
 * the file it from which it comes
 */
void print_solution(const factor_t * factor)
{
    // PRId64 is the format for uint64_t
    printf("%" PRIu64 "\n%s\n",
            factor->num,
            factor->filename);
}

/**
 * Devides one factor by another as many time possible---that is, as long as
 * the rest of the division is 0---and updates the divisor's occurrences
 * accordingly.
 *
 * @to_devide: a pointer to a factor_t containing the factor to divide
 * @divisor: a pointer to a factor_t containing the divisor factor
 */
void divide_as_much_as_possible(factor_t *to_divide, factor_t *divisor)
{
    while (to_divide->num % divisor->num == 0)
    {
        to_divide->num /= divisor->num;
        divisor->occur += to_divide->occur;
    }
}

