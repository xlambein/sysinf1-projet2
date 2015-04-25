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
    check((prime_list = list_new()) != NULL,
            "list_new");

    pthread_t *readers = (pthread_t *) malloc(argc * sizeof(pthread_t));
    check_mem(readers != NULL);

    bool *active_readers = (bool *) malloc(argc * sizeof(bool));
    check_mem(active_readers != NULL);
    for (int i = 0; i < argc; i++)
        active_readers[i] = false;

    reader_starting_state_t *readers_st = (reader_starting_state_t *) malloc(argc * sizeof(reader_starting_state_t));
    check_mem(readers_st != NULL);

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

            readers_st[i].stream = stdin;
            readers_st[i].filename = stdin_filename;

            check(!pthread_create(&readers[i], NULL, &reader, &readers_st[i]),
                    "pthread_create");
            active_readers[i] = true;
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

            check((readers_st[i].stream = fopen(argv[i], "r")) != NULL,
                    "fopen");
            readers_st[i].filename = argv[i];

            check(!pthread_create(&readers[i], NULL, &reader, &readers_st[i]),
                    "pthread_create");
            reader_count++;

            debug("...spawned !");
        }
    }

    check(!pthread_mutex_unlock(&mut_state),
            "pthread_mutex_unlock");

    pthread_t *factorizers = (pthread_t *) malloc(maxthreads * sizeof(pthread_t));
    check_mem(factorizers != NULL);
    factorizer_starting_state_t *factorizers_st = (factorizer_starting_state_t *) malloc(maxthreads * sizeof(factorizer_starting_state_t));

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
        debug("choosing %llu", (unsigned long long) to_fact.num);
        list_remove(waiting_list, to_remove);

        check(!pthread_mutex_unlock(&mut_state),
                "pthread_mutex_unlock");

        // Spawn the factorizer threads

        for (int i = 0; i < maxthreads; i++)
        {
            factorizers_st[i].start = 2+i;
            factorizers_st[i].step = maxthreads;

            check(!pthread_create(&factorizers[i], NULL, &factorize, &factorizers_st[i]),
                    "pthread_create");
        }

        // Join the factorizer threads

        for (int i = 0; i < maxthreads; i++)
        {
            check(!pthread_join(factorizers[i], NULL),
                    "pthread_join");
        }
        
        pthread_mutex_lock(&mut_state);
        
        debug("remaining: %llu", (unsigned long long) to_fact.num);

        if (to_fact.num > 1)
        {
            for (factor_t * it = list_begin(waiting_list);
                    it != list_end(waiting_list);
                    ++it)
            {
                //debug("start dividing other");
                check(to_fact.num != 0, "factor equal to 0");
                while (it->num % to_fact.num == 0)
                {
                    it->num /= to_fact.num;
                    to_fact.occur++;
                }
                //debug("stop dividing other");
                
                // If the number is now 1, remove it from the waiting list
                if (it->num == 1)
                {
                    list_remove(waiting_list, it);
                    check(!sem_wait(&sem_full),
                            "sem_wait");
                    --it;
                }
            }

            //debug("hey here");
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
                debug("adding %llu to the prime list", (unsigned long long) to_fact.num);
                list_push(prime_list, to_fact);
            }
            //debug("hey there");
        }

        pthread_mutex_unlock(&mut_state);
    } 

    for (int i = 0; i < argc; i++)
    {
        if (active_readers[i])
            check(!pthread_join(readers[i], NULL),
                    "pthread_join");
    }

    free(readers);
    free(active_readers);
    free(readers_st);
    free(factorizers);
    free(factorizers_st);

    check(!sem_destroy(&sem_full),
            "sem_destroy");
    check(!pthread_mutex_destroy(&mut_state),
            "pthread_mutex_destroy");

    list_free(waiting_list);

    return EXIT_SUCCESS;
error:
    return EXIT_FAILURE;
}
