#ifndef __threads_utils_h__
#define __threads_utils_h__

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>

#include "factor_list.h"

typedef unsigned long long ull;

extern sem_t sem_full, sem_start, sem_finish, sem_handshake;
extern pthread_mutex_t mut_state, mut_factorizers;
extern bool found;
extern factor_t to_fact;
extern factor_list_t *waiting_list, *prime_list;
extern int num_factorizers, readers_active, factorizer_meeting;

int start_timer();
int stop_timer(double * delta);

void finish(const factor_t *factor);
void divide_as_much_as_possible(factor_t *to_divide, factor_t *divisor);

#endif

