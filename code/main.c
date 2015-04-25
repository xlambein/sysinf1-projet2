#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <stdbool.h>

//TODO: remove this
#include <inttypes.h>

#include "dbg.h"
#include "threads_utils.h"
#include "factor_list.h"
#include "reader.h"
#include "factorizer.h"

#define ARGNAME_MAXTHREADS "-maxthreads"
#define ARGNAME_STDIN "-stdin"
#define STDIN_FILENAME "stdin"
#define PREFIX_URL "http://"
#define PREFIX_URL_LENGTH 7

int main(int argc, char *argv[])
{
    int maxthreads = 1;
    bool read_from_stdin = false;
    char stdin_filename[] = STDIN_FILENAME;

    check(!sem_init(&sem_full, 0, 0),
            "sem_init");
    check(!pthread_mutex_init(&mut_state, NULL),
            "pthread_mutex_init");

    check((waiting_list = list_new()) != NULL,
            "list_new");
    check((factor_list = list_new()) != NULL,
            "list_new");

    pthread_t *readers
        = (pthread_t *) malloc(sizeof(pthread_t)*(argc-1));
    check_mem(readers != NULL);

    // Lock the state mutex to be sure a reader doesn't finish before another
    // is spawned
    check(!pthread_mutex_lock(&mut_state),
            "pthread_mutex_lock");

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], ARGNAME_MAXTHREADS) == 0 && i < argc-1)
        {
            debug("reading the maxthreads variable...");
            maxthreads = atoi(argv[i+1]);
            i++;
        }
        else if (!read_from_stdin && strcmp(argv[i], ARGNAME_STDIN) == 0)
        {
            debug("spawning a stdin reading thread...");

            read_from_stdin = true;

            reader_starting_state_t * starting_state
                = (reader_starting_state_t *) malloc(sizeof(reader_starting_state_t));
            check_mem(starting_state != NULL);

            starting_state->stream = stdin;
            starting_state->filename = stdin_filename;

            check(!pthread_create(&readers[i-1], NULL, &reader, starting_state),
                    "pthread_create");
            reader_count++;

            debug("...spawned !");
        }
        else if (strncmp(argv[i], PREFIX_URL, PREFIX_URL_LENGTH) == 0)
        {
            debug("spawning a network reading thread...");
            //TODO: spawn a network reading thread
        }
        else
        {
            debug("spawning file reader thread...");

            reader_starting_state_t * starting_state
                = (reader_starting_state_t *) malloc(sizeof(reader_starting_state_t));
            check_mem(starting_state != NULL);

            check((starting_state->stream = fopen(argv[i], "r")) != NULL,
                    "fopen");
            starting_state->filename = argv[i];

            check(!pthread_create(&readers[i-1], NULL, &reader, starting_state),
                    "pthread_create");
            reader_count++;

            debug("...spawned !");
        }
    }

    check(!pthread_mutex_unlock(&mut_state),
            "pthread_mutex_unlock");

    pthread_t *factorizers
        = (pthread_t *) malloc(sizeof(pthread_t)*maxthreads);
    check_mem(factorizers != NULL);

    while (!found)
    {
        check(!sem_wait(&sem_full),
                "sem_wait");

        // Get smallest number from waiting list
        
        check(!pthread_mutex_lock(&mut_state),
                "pthread_mutex_lock");

        to_fact.num = UINT64_MAX;
        factor_t * to_remove = NULL;
        for (factor_t * it = list_begin(waiting_list);
                it != list_end(waiting_list);
                ++it)
        {
            if (it->num <= to_fact.num)
                to_remove = it;
        }
        check(to_remove != NULL,
                "Couldn't get smallest number from waiting list");

        to_fact = *to_remove;
        list_remove(waiting_list, to_remove);

        check(!pthread_mutex_unlock(&mut_state),
                "pthread_mutex_unlock");

        // Spawn the factorizer threads

        for (int i = 0; i < maxthreads; i++)
        {
            factorizer_starting_state_t * starting_state
                = (factorizer_starting_state_t *) malloc(sizeof(factorizer_starting_state_t));
            check_mem(starting_state != NULL);

            starting_state->start = 2+i;
            starting_state->step = maxthreads;

            check(!pthread_create(&factorizers[i], NULL, &factorize, starting_state),
                    "pthread_create");
        }

        // Join the factorizer threads

        for (int i = 0; i < maxthreads; i++)
        {
            check(!pthread_join(factorizers[i], NULL),
                    "pthread_join");
        }
        
        pthread_mutex_lock(&mut_state);

        for (factor_t * it = list_begin(waiting_list);
                it != list_end(waiting_list);
                ++it)
        {
            check(to_fact.num != 0, "factor equal to 0");
            while (it->num % to_fact.num == 0)
            {
                it->num /= to_fact.num;
                to_fact.occur++;
            }
            
            // If the number is now 1, remove it from the waiting list
            if (it->num == 1)
            {
                list_remove(waiting_list, it);
                check(!sem_wait(&sem_full),
                        "sem_wait");
                --it;
            }
        }

        if (reader_count == 0)
        {
            if (to_fact.occur == 1)
            {
                found = true;
                finish(&to_fact);
            }
        }
        else
        {
            const char msg[] = "adding %" PRId64 " to factor list\n";
            fprintf(stderr, msg, to_fact.num);
            list_push(factor_list, to_fact);
        }

        pthread_mutex_unlock(&mut_state);
    } 

    free(factorizers);

    check(!sem_destroy(&sem_full),
            "sem_destroy");
    check(!pthread_mutex_destroy(&mut_state),
            "pthread_mutex_destroy");

    list_free(waiting_list);

    return EXIT_SUCCESS;
error:
    return EXIT_FAILURE;
}
