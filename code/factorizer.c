#include "dbg.h"
#include "factor_list.h"
#include "factorizer.h"
#include "threads_utils.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdint.h>

void *factorize(void *starting_state)
{
    factorizer_starting_state_t *st = (factorizer_starting_state_t *) starting_state;
    
    for (uint64_t i = st->start; i * i < to_fact.num; i += st->step)
    {
        if (to_fact.num % i == 0)
        {
            check(!pthread_mutex_lock(&mut_state), "pthread_lock_mutex");
            {
                if (to_fact.num % i == 0)
                {
                    factor_t factor_found;
                    factor_found.num = i;
                    factor_found.filename = to_fact.filename;
                    
                    list_push(waiting_list, factor_found);
                    to_fact.num /= i;
                    check(!sem_post(&sem_full), "sem_post");
                }
            }
            check(!pthread_mutex_unlock(&mut_state), "pthread_unlock_mutex");
        }
    }
    return NULL;
error:
    exit(EXIT_FAILURE);
}
