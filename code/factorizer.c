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
    
    for (uint64_t i = st->start; i * i <= to_fact.num; i += st->step)
    {
        if (to_fact.num % i == 0)
        {
            check(!pthread_mutex_lock(&mut_state), "pthread_lock_mutex");
            {
                int occur = 0;
                while (to_fact.num % i == 0)
                {
                    to_fact.num /= i;
                    occur++;
                }
                if (occur > 0)
                {
                    debug("dividing by %llu, %d times", (ull) i, occur);
                    
                    factor_t factor_found;
                    factor_found.num = i;
                    factor_found.occur = occur;
                    factor_found.filename = to_fact.filename;
                    
                    list_push(waiting_list, factor_found);
                    check(!sem_post(&sem_full), "sem_post");
                }
                else
                {
                    debug("ah darn it was divisible by %llu but now it's %llu",
                            (ull) i, (ull) to_fact.num);
                }
            }
            check(!pthread_mutex_unlock(&mut_state), "pthread_unlock_mutex");
        }
    }
    return NULL;
error:
    exit(EXIT_FAILURE);
}
