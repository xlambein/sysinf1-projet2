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
    //TODO: increment reader counter
    check(!pthread_mutex_unlock(&mut_state),
            "pthread_mutex_unlock");
    
    uint64_t number;
    while (fread(&number, sizeof(uint64_t), 1, st->stream) == sizeof(uint64_t))
    {
        // Convert the number from BigEndian
        number = be64toh(number);

        check(!pthread_mutex_lock(&mut_state),
                "pthread_mutex_lock");
        //TODO: Do something with the number
        check(!pthread_mutex_unlock(&mut_state),
                "pthread_mutex_unlock");
        
        check(!sem_post(&sem_full),
                "sem_post");
    }

    check(!pthread_mutex_lock(&mut_state),
            "pthread_mutex_lock");
    //TODO: decrement reader counter
    check(!pthread_mutex_unlock(&mut_state),
            "pthread_mutex_unlock");

error:
    free(st);

    return NULL;
}

