#include "threads_utils.h"

#include <stdio.h>
#include <sys/time.h>
#include <inttypes.h>

sem_t sem_full, sem_start, sem_finish, sem_handshake;
pthread_mutex_t mut_state, mut_factorizers;
bool found = false;
factor_t to_fact = {0, 0, NULL};
factor_list_t *waiting_list, *prime_list;
int num_factorizers = 1, readers_active = 0, factorizer_meeting = 0;

static struct timeval timer_tv_start;

int start_timer()
{
    return gettimeofday(&timer_tv_start, NULL);
}

int stop_timer(double * delta)
{
    struct timeval timer_tv_stop;
    int retval = gettimeofday(&timer_tv_stop, NULL);
    if (!retval)
    {
        long diff = (timer_tv_stop.tv_usec + 1e6 * timer_tv_stop.tv_sec)
            - (timer_tv_start.tv_usec + 1e6 * timer_tv_start.tv_sec);
        *delta = ((double) diff)/1e6;
    }
    
    return retval;
}

void finish(const factor_t * factor)
{
    printf("%" PRId64 "\n%s\n",
            factor->num,
            factor->filename);
}

void divide_as_much_as_possible(factor_t *to_divide, factor_t *divisor)
{
    while (to_divide->num % divisor->num == 0)
    {
        to_divide->num /= divisor->num;
        divisor->occur += to_divide->occur;
    }
}
