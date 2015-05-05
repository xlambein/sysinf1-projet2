#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <curl/curl.h>

#include "curl_getter.h"
#include "dbg.h"
#include "factor_list.h"
#include "factorizer.h"
#include "reader.h"
#include "util.h"

#define ARGNAME_MAXTHREADS "-maxthreads"
#define ARGNAME_STDIN "-stdin"
#define STDIN_FILENAME "stdin"
#define PREFIX_URL "http://"
#define PREFIX_URL_LENGTH 7

// Total number of readers
static int num_readers;
// Whether an stdin reader has been launched already
static bool read_from_stdin = false;
// File readers and integer factorizers
static pthread_t *readers, *factorizers;
// Starting states to be sent as arguments to the threads
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
    
    // Initialize things and launch the threads
    get_num_readers_and_factorizers(argc, argv);
    init();
    launch_readers(argc, argv);
    launch_factorizers();
    
    // Main loop that synchronizes the factorization
    main_loop();
    
    // Free all the resources
    cleanup();

    // Stop timer and print elapsed time
    double elapsed_time;
    stop_timer(&elapsed_time);
    printf("%.3f\n", elapsed_time);

    // If nothing failed up to this point, everything is fine
    return EXIT_SUCCESS;
}

/**
 * Obtains the number of readers (number of paths) and the number of factorizers
 * (maxthreads argument) from the command-line arguments.
 */
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

/**
 * Initializes all the resources.
 */
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

/**
 * Launches the reader threads on the files provided in the arguments. They will
 * only start reading once all the readers have been launched.
 */
static void launch_readers(int argc, char *argv[])
{
    // Lock the state mutex to make sure no reader finishes before all of them
    // are launched
    check(!pthread_mutex_lock(&mut_state), "pthread_mutex_lock");
    {
        for (int i = 1; i < argc; i++)
        {
            // Just skip the maxthreads argument, since it was read already
            if (strcmp(argv[i], ARGNAME_MAXTHREADS) == 0)
                i++;
            // If the stdin argument is set, launch a reader on stdin
            else if (!read_from_stdin && strcmp(argv[i], ARGNAME_STDIN) == 0)
            {
                debug("spawning a stdin reading thread...");

                // Set the boolean so only one stdin reader is launched
                read_from_stdin = true;
                
                // Get the current position of the reader
                int cur = readers_active;
                readers_active++;

                // Associate the file descriptor and the filename
                readers_st[cur].fd = STDIN_FILENO;
                readers_st[cur].filename = STDIN_FILENAME;

                // Launch the reader
                check(!pthread_create(&readers[cur], NULL, &reader,
                        &readers_st[cur]), "pthread_create");
            }
            // If the given path starts with "http://", launch curl
            else if (strncmp(argv[i], PREFIX_URL, PREFIX_URL_LENGTH) == 0)
            {
                debug("spawning a network reading thread...");
                
                // Get the current position of the reader
                int cur = readers_active;
                readers_active++;

                // Files are read from the network by creating a pipe from which
                // the reader will read as if it were a normal file
                int pipefd[2];
                check(!pipe(pipefd), "pipe");

                // Associate the file descriptor and the filename
                readers_st[cur].fd = pipefd[0];
                readers_st[cur].filename = argv[i];

                // Launch the reader
                check(!pthread_create(&readers[cur], NULL, &reader,
                        &readers_st[cur]), "pthread_create");

                // Give curl the pipe to write into and the url
                curl_getters_st[cur].fd = pipefd[1];
                curl_getters_st[cur].url = argv[i];

                // Launch curl on a detached thread
                pthread_t curl_getter_thread;
                check(!pthread_create(&curl_getter_thread, NULL, &curl_getter,
                        &curl_getters_st[cur]), "phtread_create");
                check(!pthread_detach(curl_getter_thread), "pthread_detach");
            }
            // Otherwise, launch a simple file reader
            else
            {
                debug("spawning a file reading thread...");

                // Get the current position of the reader
                int cur = readers_active;
                
                // Only increment the number of readers if the path is valid
                if ((readers_st[cur].fd = open(argv[i], O_RDONLY)) == -1)
                    continue;
                readers_active++;

                // Associate the filename
                readers_st[cur].filename = argv[i];

                // Launch the reader
                check(!pthread_create(&readers[cur], NULL, &reader,
                        &readers_st[cur]), "pthread_create");
            }
        }
        // Refresh the number of readers in case some files failed to open
        num_readers = readers_active;
    }
    check(!pthread_mutex_unlock(&mut_state), "pthread_mutex_unlock");
    
    return;
error:
    exit(EXIT_FAILURE);
}

/**
 * Launches the factorizer threads. They will not start factorizing immediately,
 * since they have to wait for an input from the main loop.
 */
static void launch_factorizers()
{
    // Spawn the factorizer threads
    for (int i = 0; i < num_factorizers; i++)
    {
        check(!pthread_create(&factorizers[i], NULL, &factorizer,
                &factorizers_st[i]), "pthread_create");
    }
    
    return;
error:
    exit(EXIT_FAILURE);
}

/**
 * Executes the main loop of the program, i.e. takes a number to factorizer,
 * launch the factorizers on it then once they're finished, performs some scans
 * too see if it is used elsewhere. If it finds a prime that is not used
 * elsewhere, and the readers have all finished reading, that means it has found
 * the solution.
 */
static void main_loop()
{
    while (!found)
    {
        // Wait for a number to be added to the waiting list
        check(!sem_wait(&sem_full), "sem_wait");
        if (found)
            break;
        
        // If no prime with only one occurence was found
        if (readers_active == 0 && waiting_list->size == 0)
            goto error;

        // Get smallest number from waiting list
        to_fact = get_smallest_from_waiting_list();
        debug("choosing %llu", (ull) to_fact.num);

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
        debug("remaining: %llu", (ull) to_fact.num);
        
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
                    debug("adding %llu to the prime list", (ull) to_fact.num);
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

/**
 * Gets the smallest factor from the waiting list and removes it.
 *
 * @return: a factor_t containing the factor with its occurences and file.
 */
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

/**
 * Joins the threads and frees all the resources.
 */
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
