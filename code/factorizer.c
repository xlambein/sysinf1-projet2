#include <stdint.h>
#include <stdlib.h>

#include "dbg.h"
#include "factor_list.h"
#include "factorizer.h"
#include "util.h"

/**
 * Factorizes numbers that are given in its starting state. The factorizers are
 * launched by the sem_start signal and indicate they are finished through the
 * sem_finish signal.
 */
void *factorizer(void *starting_state)
{
    // The starting state will be updated by the main loop every time a new
    // number has to be factorized
    factorizer_starting_state_t *st = (factorizer_starting_state_t *)
            starting_state;
    
    while (true)
    {
        // Wait for the start signal then forward it
        check(!sem_wait(&sem_start), "sem_wait");
        check(!sem_post(&sem_start), "sem_wait");
        
        // Finish thread if the sentinel value is set
        if (to_fact.num == 0)
            break;
        
        // Try to find a divisor of the number, with an offset
        for (uint64_t i = st->start; i * i <= to_fact.num; i += st->step)
        {
            // If i is a divisor
            if (to_fact.num % i == 0)
            {
                check(!pthread_mutex_lock(&mut_to_fact), "pthread_lock_mutex");
                {
                    // Divide the number as many times as possible by the factor
                    // we just found, remembering the occurences
                    factor_t factor_found = {i, 0, to_fact.filename};
                    divide_as_much_as_possible(&to_fact, &factor_found);
                    
                    // It might happen that to_fact.num was divided by another
                    // thread before we locked the mutex
                    if (factor_found.occur > 0)
                    {
                        debug("dividing by %llu, %d times", (ull) i,
                                factor_found.occur);
                        
                        check(!pthread_mutex_lock(&mut_state),
                                "pthread_lock_mutex");
                        {
                            // Add the factor to the waiting list and increment
                            // the semaphore
                            list_push(waiting_list, factor_found);
                            check(!sem_post(&sem_full), "sem_post");
                        }
                        check(!pthread_mutex_unlock(&mut_state),
                                "pthread_unlock_mutex");
                    }
                }
                check(!pthread_mutex_unlock(&mut_to_fact),
                        "pthread_unlock_mutex");
            }
        }
        
        check(!pthread_mutex_lock(&mut_factorizers), "pthread_lock_mutex");
        {
            // Count the threads that have finished
            factorizer_meeting++;
            // If all the threads have arrived
            if (factorizer_meeting == num_factorizers)
            {
                // We compensate the additional sem_post performed by the last
                // thread to start the factorization
                check(!sem_wait(&sem_start), "sem_wait");
                // We start the handshake process
                check(!sem_post(&sem_handshake), "sem_post");
            }
        }
        check(!pthread_mutex_unlock(&mut_factorizers), "pthread_unlock_mutex");
        
        // Shake hands once everybody has finished
        // Note: this section does not have to be locked with a mutex since it
        // is already protected by the sem_handshake semaphore
        check(!sem_wait(&sem_handshake), "sem_wait");
        factorizer_meeting--;
        
        // If this is the last thread to handshake, send the finish signal to
        // the main loop
        // (Note: we have to add brackets here because of the macro)
        if (factorizer_meeting == 0)
        {
            check(!sem_post(&sem_finish), "sem_post");
        }
        // Otherwise, forward the handshake
        else
        {
            check(!sem_post(&sem_handshake), "sem_post");
        }
    }
    
    return NULL;
error:
    exit(EXIT_FAILURE);
}
