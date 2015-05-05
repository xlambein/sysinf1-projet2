#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <curl/curl.h>

//TODO: remove this
#include <inttypes.h>

#include "dbg.h"
#include "threads_utils.h"
#include "factor_list.h"
#include "reader.h"
#include "curl_getter.h"
#include "factorizer.h"

#define ARGNAME_MAXTHREADS "-maxthreads"
#define ARGNAME_STDIN "-stdin"
#define STDIN_FILENAME "stdin"
#define PREFIX_URL "http://"
#define PREFIX_URL_LENGTH 7

static int num_readers;
static bool read_from_stdin = false;
static pthread_t *readers, *factorizers;
static reader_starting_state_t *readers_st;
static factorizer_starting_state_t *factorizers_st;
static curl_getter_starting_state_t *curl_getters_st;

static void get_num_readers_and_factorizers(int argc, char *argv[]);
static void init();
static void launch_readers(int argc, char *argv[]);
static void launch_factorizers();
static void main_loop();
static factor_t get_smallest_from_waiting_list();
static void cleanup();

int main(int argc, char *argv[])
{
    // Start the timer
    start_timer();
    
    get_num_readers_and_factorizers(argc, argv);
    init();
    
    launch_readers(argc, argv);
    launch_factorizers();
    
    main_loop();
    
    cleanup();

    // Stop timer and print elapsed time
    double elapsed_time;
    stop_timer(&elapsed_time);
    printf("%.3f\n", elapsed_time);

    return EXIT_SUCCESS;
}

static void get_num_readers_and_factorizers(int argc, char *argv[])
{
    num_readers = 0;
    for (int i = 1; i < argc; i++)
    {
        // If this argument indicates the maximum number computation threads
        if (!strcmp(argv[i], ARGNAME_MAXTHREADS))
        {
            // We skip ahead in the argument list
            i++;
            if (i == argc)
                break;
            // We remember it as the number of factorizers
            num_factorizers = atoi(argv[i]);
        }
        // All other arguments correspond to a reader
        else
            num_readers++;
    }
}

static void init()
{
    // Initialize semaphores and mutexes
    check(!sem_init(&sem_full, 0, 0), "sem_init");
    check(!sem_init(&sem_start, 0, 0), "sem_init");
    check(!sem_init(&sem_finish, 0, 0), "sem_init");
    check(!sem_init(&sem_handshake, 0, 0), "sem_init");
    check(!pthread_mutex_init(&mut_state, NULL), "pthread_mutex_init");
    check(!pthread_mutex_init(&mut_factorizers, NULL), "pthread_mutex_init");

    // Create our extendable lists
    check_mem((waiting_list = list_new()) != NULL);
    check_mem((prime_list = list_new()) != NULL);
    
    // Allocate the readers' structures
    readers = (pthread_t *) malloc(num_readers * sizeof(pthread_t));
    readers_st = (reader_starting_state_t *)
            malloc(num_readers * sizeof(reader_starting_state_t));
    curl_getters_st = (curl_getter_starting_state_t *)
            malloc(num_readers * sizeof(curl_getter_starting_state_t));
    check_mem(readers != NULL);
    check_mem(readers_st != NULL);
    check_mem(curl_getters_st != NULL);

    // Allocate the factorizers' structures
    factorizers = (pthread_t *) malloc(num_factorizers * sizeof(pthread_t));
    factorizers_st = (factorizer_starting_state_t *)
            malloc(num_factorizers * sizeof(factorizer_starting_state_t));
    check_mem(factorizers != NULL);
    check_mem(factorizers_st != NULL);

    // Initialize curl
    curl_global_init(CURL_GLOBAL_ALL);
    
    return;
error:
    exit(EXIT_FAILURE);
}

static void launch_readers(int argc, char *argv[])
{
    // Lock the state mutex to be sure a reader doesn't finish before another
    // is spawned
    check(!pthread_mutex_lock(&mut_state), "pthread_mutex_lock");
    {
        for (int i = 1; i < argc; i++)
        {
            if (strcmp(argv[i], ARGNAME_MAXTHREADS) == 0)
                i++;
            else if (!read_from_stdin && strcmp(argv[i], ARGNAME_STDIN) == 0)
            {
                debug("spawning a stdin reading thread...");

                read_from_stdin = true;
                int cur = readers_active;
                readers_active++;

                readers_st[cur].stream = stdin;
                readers_st[cur].filename = STDIN_FILENAME;

                check(!pthread_create(&readers[cur], NULL, &reader, &readers_st[cur]),
                        "pthread_create");
            }
            else if (strncmp(argv[i], PREFIX_URL, PREFIX_URL_LENGTH) == 0)
            {
                debug("spawning a network reading thread...");
                
                int cur = readers_active;
                readers_active++;

                int pipefd[2];
                check(!pipe(pipefd), "pipe");

                if ((readers_st[cur].stream = fdopen(pipefd[0], "r")) == NULL)
                {
                    close(pipefd[0]);
                    close(pipefd[1]);
                    continue;
                }

                readers_st[cur].filename = argv[i];

                check(!pthread_create(&readers[cur], NULL, &reader, &readers_st[cur]),
                        "pthread_create");

                // Spawn detached curl_getter thread
                curl_getters_st[cur].url = argv[i];
                curl_getters_st[cur].fd = pipefd[1];

                pthread_t curl_getter_thread;

                check(!pthread_create(&curl_getter_thread, NULL,
                            &curl_getter, &curl_getters_st[cur]),
                        "phtread_create");
                check(!pthread_detach(curl_getter_thread),
                        "pthread_detach");
            }
            else
            {
                debug("spawning a file reading thread...");

                int cur = readers_active;
                if ((readers_st[cur].stream = fopen(argv[i], "r")) == NULL)
                    continue;
                readers_active++;
                printf("%d\n", cur);

                readers_st[cur].filename = argv[i];

                check(!pthread_create(&readers[cur], NULL, &reader, &readers_st[cur]),
                        "pthread_create");
            }
        }
        num_readers = readers_active;
    }
    check(!pthread_mutex_unlock(&mut_state), "pthread_mutex_unlock");
    
    return;
error:
    exit(EXIT_FAILURE);
}

static void launch_factorizers()
{
    // Spawn the factorizer threads
    for (int i = 0; i < num_factorizers; i++)
    {
        check(!pthread_create(&factorizers[i], NULL, &factorize, &factorizers_st[i]),
                "pthread_create");
    }
    
    return;
error:
    exit(EXIT_FAILURE);
}

static void main_loop()
{
    while (!found)
    {
        check(!sem_wait(&sem_full), "sem_wait");
        if (found)
            break;
        if (readers_active == 0 && waiting_list->size == 0)
            goto error;

        // Get smallest number from waiting list
        to_fact = get_smallest_from_waiting_list();
        debug("choosing %llu", (unsigned long long) to_fact.num);

        // Set the factorizers to begin with different offsets
        for (int i = 0; i < num_factorizers; i++)
        {
            factorizers_st[i].start = 2+i;
            factorizers_st[i].step = num_factorizers;
        }
        
        // Restart the factorizers
        check(!sem_post(&sem_start), "sem_post");
        
        // Wait for the factorizers
        check(!sem_wait(&sem_finish), "sem_wait");
        debug("remaining: %llu", (unsigned long long) to_fact.num);
        
        // If the factor we just found is bigger than 1, it is prime, so we try
        // to divide all the factors in the waiting list by it
        if (to_fact.num > 1)
        {
            pthread_mutex_lock(&mut_state);
            {
                for (factor_t * it = list_begin(waiting_list);
                        it != list_end(waiting_list); it++)
                {
                    divide_as_much_as_possible(it, &to_fact);
                    
                    // If the number is now 1, remove it from the waiting list
                    if (it->num == 1)
                    {
                        list_remove(waiting_list, it);
                        it--;
                        
                        // We decrease the semaphore by one because there is one
                        // less number to process
                        check(!sem_wait(&sem_full), "sem_wait");
                    }
                }
                
                // If no readers are left and we found only one occurence, then
                // it is the solution
                if (readers_active == 0)
                {
                    if (to_fact.occur == 1)
                    {
                        found = true;
                        print_solution(&to_fact);
                    }
                }
                // Otherwise we add it to the prime list
                else
                {
                    debug("adding %llu to the prime list", (unsigned long long) to_fact.num);
                    list_push(prime_list, to_fact);
                }
            }
            pthread_mutex_unlock(&mut_state);
        }
    }
    
    return;
error:
    exit(EXIT_FAILURE);
}

static factor_t get_smallest_from_waiting_list()
{
    factor_t smallest;
    
    check(!pthread_mutex_lock(&mut_state), "pthread_mutex_lock");
    {
        // By default we try the first element
        factor_t * to_remove = list_begin(waiting_list);
        
        for (factor_t * it = list_begin(waiting_list);
                it != list_end(waiting_list); it++)
        {
            // If this one is smaller than the one we know, we choose this one
            if (it->num < to_remove->num)
                to_remove = it;
        }
        
        // We remember the smallest element and remove it
        smallest = *to_remove;
        list_remove(waiting_list, to_remove);
    }
    check(!pthread_mutex_unlock(&mut_state), "pthread_mutex_unlock");
    
    return smallest;
error:
    exit(EXIT_FAILURE);
}

static void cleanup()
{
    // Join the reader threads
    for (int i = 0; i < num_readers; i++)
        check(!pthread_join(readers[i], NULL), "pthread_join");
    
    // Send end signal to the factorizers
    to_fact.num = 0;
    check(!sem_post(&sem_start), "sem_post");
    
    // Join the factorizer threads
    for (int i = 0; i < num_factorizers; i++)
        check(!pthread_join(factorizers[i], NULL), "pthread_join");

    // Curl cleanup
    curl_global_cleanup();

    // Free arrays
    free(curl_getters_st);
    free(readers);
    free(readers_st);
    free(factorizers);
    free(factorizers_st);

    // Destroy semaphores and mutexes
    check(!sem_destroy(&sem_full), "sem_destroy");
    check(!sem_destroy(&sem_start), "sem_destroy");
    check(!sem_destroy(&sem_finish), "sem_destroy");
    check(!sem_destroy(&sem_handshake), "sem_destroy");
    check(!pthread_mutex_destroy(&mut_state), "pthread_mutex_destroy");
    check(!pthread_mutex_destroy(&mut_factorizers), "pthread_mutex_destroy");

    // Free the waiting list (the prime list is already freed)
    list_free(waiting_list);
    
    return;
error:
    exit(EXIT_FAILURE);
}
