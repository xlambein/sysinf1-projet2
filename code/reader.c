#include "reader.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <endian.h>
#include <pthread.h>

#include "dbg.h"
#include "threads_utils.h"

void *reader(void *arg)
{
    reader_starting_state_t * st
        = (reader_starting_state_t *) arg;

    check(!pthread_mutex_lock(&mut_state),
            "pthread_mutex_lock");
    reader_count++;
    check(!pthread_mutex_unlock(&mut_state),
            "pthread_mutex_unlock");
    
    uint64_t number;
    while (fread(&number, sizeof(uint64_t), 1, st->stream) == sizeof(uint64_t))
    {
        // Convert the number from BigEndian
        number = be64toh(number);

        check(!pthread_mutex_lock(&mut_state),
                "pthread_mutex_lock");

        // Divide the number by every factor in the factor list
        for (factor_t * it = list_begin(waiting_list);
                it != list_end(waiting_list);
                ++it)
        {
            if ((number % it->num) == 0)
                number /= it->num;
        }

        // Add it to the waiting list if it's not 1
        if (number != 1)
        {
            factor_t tmp = {number, 1, st->filename};
            list_push(waiting_list, tmp);
            sem_post(&sem_full);
        }

        check(!pthread_mutex_unlock(&mut_state),
                "pthread_mutex_unlock");
        
        check(!sem_post(&sem_full),
                "sem_post");
    }

error:
    check(!pthread_mutex_lock(&mut_state),
            "pthread_mutex_lock");
    reader_count--;
    
    // If this is the last reader
    if (reader_count == 0)
    {
        find_prime_with_one_occurrence();
        list_free(factor_list);
    }
    
    check(!pthread_mutex_unlock(&mut_state),
            "pthread_mutex_unlock");

    if (st->stream != stdin)
        fclose(st->stream);
    free((void *) st->filename);
    free(st);

    return NULL;
}

void find_prime_with_one_occurrence()
{
    for (int i = 0; i < factor_list->size; i++)
    {
        if (factor_list->list[i].occur == 1)
        {
            found = true;
            to_fact.num = 1; // This will stop the factorizer threads
            finish(&factor_list->list[i]);
            break;
        }
    }
}

