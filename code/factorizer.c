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
    
    while (true)
    {
        // Wait for the start signal
        check(!sem_wait(&sem_start), "sem_wait");
        check(!sem_post(&sem_start), "sem_wait");
        
        // Finish thread if the sentinel value is set
        if (to_fact.num == 0)
            return NULL;
        
        for (uint64_t i = st->start; i * i <= to_fact.num; i += st->step)
        {
            if (to_fact.num % i == 0)
            {
                check(!pthread_mutex_lock(&mut_state), "pthread_lock_mutex");
                {
                    factor_t factor_found = {i, 0, to_fact.filename};
                    divide_as_much_as_possible(&to_fact, &factor_found);
                    
                    if (factor_found.occur > 0)
                    {
                        debug("dividing by %llu, %d times", (ull) i, factor_found.occur);
                        
                        list_push(waiting_list, factor_found);
                        check(!sem_post(&sem_full), "sem_post");
                    }
                }
                check(!pthread_mutex_unlock(&mut_state), "pthread_unlock_mutex");
            }
        }
        
        // Counting the threads that have finished
        check(!pthread_mutex_lock(&mut_factorizers), "pthread_lock_mutex");
        factorizer_meeting++;
        if (factorizer_meeting == num_factorizers)
        {
            check(!sem_wait(&sem_start), "sem_wait");
            check(!sem_post(&sem_handshake), "sem_post");
        }
        check(!pthread_mutex_unlock(&mut_factorizers), "pthread_unlock_mutex");
        
        // Shaking hands once everybody has finished
        check(!sem_wait(&sem_handshake), "sem_wait");
        factorizer_meeting--;
        if (factorizer_meeting == 0)
        {
            check(!sem_post(&sem_finish), "sem_post");
        }
        else
        {
            check(!sem_post(&sem_handshake), "sem_post");
        }
    }
error:
    exit(EXIT_FAILURE);
}
