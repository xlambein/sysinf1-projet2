#ifndef __threads_utils_h__
#define __threads_utils_h__

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>

#include "factor_list.h"

extern sem_t sem_full;
extern pthread_mutex_t mut_state;
extern bool found;
extern factor_t to_fact;
extern factor_list_t * waiting_list,
              * factor_list;
extern int reader_count;

void finish(const factor_t * factor);

#endif

