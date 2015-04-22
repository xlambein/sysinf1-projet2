#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <stdbool.h>

#include "dbg.h"
#include "threads_utils.h"
#include "factor_list.h"
#include "reader.h"

#define ARGNAME_MAXTHREADS "-maxthreads"
#define ARGNAME_STDIN "-stdin"
#define PREFIX_URL "http://"
#define PREFIX_URL_LENGTH 7

int main(int argc, char *argv[])
{
    int maxthreads = 1;
    bool read_from_stdin = false;
    bool found = false;
    factor_list_t * waiting_list,
                  * factor_list;

    //TODO: initialize stuff
    check(!sem_init(&sem_full, 0, 0),
            "sem_init");
    check(!pthread_mutex_init(&mut_state, NULL),
            "pthread_mutex_init");

    check((waiting_list = new_list()) != NULL,
            "new_list");
    check((factor_list = new_list()) != NULL,
            "new_list");

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], ARGNAME_MAXTHREADS) && i < argc-1)
        {
            maxthreads = atoi(argv[i+1]);
            i++;
        }
        else if (!read_from_stdin && strcmp(argv[i], ARGNAME_STDIN))
        {
            read_from_stdin = true;
            pthread_t thread;

            reader_starting_state_t * starting_state
                = (reader_starting_state_t *) malloc(sizeof(reader_starting_state_t));
            check(starting_state != NULL,
                    "malloc");

            starting_state->stream = stdin;

            check(!pthread_create(&thread, NULL, &reader, starting_state),
                    "pthread_create");

            //TODO: be sure it's ok to create a thread in a local stack variable
        }
        else if (strncmp(argv[i], PREFIX_URL, PREFIX_URL_LENGTH))
        {
            //TODO: spawn a network reading thread
        }
        else
        {
            pthread_t thread;

            reader_starting_state_t * starting_state
                = (reader_starting_state_t *) malloc(sizeof(reader_starting_state_t));
            check(starting_state != NULL,
                    "malloc");

            check((starting_state->stream = fopen(argv[i], "r")) != NULL,
                    "fopen");

            check(!pthread_create(&thread, NULL, &reader, starting_state),
                    "pthread_create");

            //TODO: be sure it's ok to create a thread in a local stack variable
        }
    }

    while (!found)
    {
        pthread_t *factorizers
            = (pthread_t *) calloc(sizeof(pthread_t), maxthreads);
        check(factorizers != NULL,
                "calloc");

        for (int i = 0; i < maxthreads; i++)
        {
            //TODO:
            //check(!pthread_create(&factorizers[i], NULL, &factorizer, starting_state),
            //        "pthread_create");
        }

        for (int i = 0; i < maxthreads; i++)
        {
            check(pthread_join(factorizers[i], NULL),
                    "pthread_join");
        }
        
        pthread_mutex_lock(&mut_state);
            //TODO: stuff
        pthread_mutex_unlock(&mut_state);
    } 

    check(!sem_destroy(&sem_full),
            "sem_destroy");
    check(!pthread_mutex_destroy(&mut_state),
            "pthread_mutex_destroy");

    free_list(waiting_list);
    free_list(factor_list);

    return EXIT_SUCCESS;
error:
    return EXIT_FAILURE;
}
