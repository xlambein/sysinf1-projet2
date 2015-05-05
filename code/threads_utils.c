#include "threads_utils.h"

#include <stdio.h>
#include <inttypes.h>

sem_t sem_full, sem_start, sem_finish, sem_handshake;
pthread_mutex_t mut_state, mut_factorizers;
bool found = false;
factor_t to_fact = {0, 0, NULL};
factor_list_t *waiting_list, *prime_list;
int num_factorizers = 1, readers_active = 0, factorizer_meeting = 0;

void finish(const factor_t * factor)
{
    //TODO: print duration
    printf("%" PRId64 "\n%s\n", factor->num, factor->filename);
}

void divide_as_much_as_possible(factor_t *to_divide, factor_t *divisor)
{
    while (to_divide->num % divisor->num == 0)
    {
        to_divide->num /= divisor->num;
        divisor->occur += to_divide->occur;
    }
}
