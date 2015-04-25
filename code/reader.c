#include "reader.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <endian.h>
#include <pthread.h>

//TODO: remove this
#include <inttypes.h>

#include "dbg.h"
#include "threads_utils.h"

void *reader(void *arg)
{

    //debug("initializing reader");

    reader_starting_state_t * st
        = (reader_starting_state_t *) arg;

    //debug("starting reading...");
    uint64_t number;
    while (fread(&number, sizeof(uint64_t), 1, st->stream) == 1)
    {
        //debug("reading one number...");
        // Convert the number from BigEndian
        number = be64toh(number);
        //debug("reading %llu", (unsigned long long) number);
        check(number != 0 && number != 1,
                "number equal to 0 or 1");

        check(!pthread_mutex_lock(&mut_state),
                "pthread_mutex_lock");

        // Divide the number by every factor in the prime list
        for (factor_t * it = list_begin(prime_list);
                it != list_end(prime_list);
                ++it)
        {
            check(it->num != 0, "factor equal to 0");
            while ((number % it->num) == 0)
            {
                number /= it->num;
                it->occur++;
            }
        }

        // Add it to the waiting list if it's not 1
        if (number != 1)
        {
            factor_t tmp = {number, 1, st->filename};
            list_push(waiting_list, tmp);
            check(!sem_post(&sem_full),
                    "sem_post");
        }

        check(!pthread_mutex_unlock(&mut_state),
                "pthread_mutex_unlock");
    }

    debug("finished reading");

    check(!pthread_mutex_lock(&mut_state),
            "pthread_mutex_lock");
    reader_count--;
    
    // If this is the last reader
    if (reader_count == 0)
    {
        find_prime_with_one_occurrence();
        list_free(prime_list);
    }
    
    check(!pthread_mutex_unlock(&mut_state),
            "pthread_mutex_unlock");

    if (st->stream != stdin)
    {
        debug("closing stream");
        check(!fclose(st->stream),
                "fclose");
    }

    return NULL;

error:
    exit(EXIT_FAILURE);
}

void find_prime_with_one_occurrence()
{
    for (factor_t *it = list_begin(prime_list); it != list_end(prime_list); it++)
    {
        if (it->occur == 1)
        {
            found = true;
            to_fact.num = 1; // This will stop the factorizer threads
            finish(it);
            break;
        }
    }
}

