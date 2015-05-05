#include "reader.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <endian.h>
#include <pthread.h>

#include "dbg.h"
#include "util.h"

static factor_t * find_prime_with_one_occurrence();

/**
 * Reads BigEndian 64 bit integers from a file descriptor and puts them in
 * the waiting list.
 *
 * @arg A pointer to a reader_starting_state_t structure, containing :
 * - fd, the file descriptor and
 * - filename, a string containing the name of the file associated with the
 *   file descriptor
 *
 * @returns: NULL
 *
 * Note: if fd != stdin, it is closed at the end of the function.
 */
void *reader(void *arg)
{
    reader_starting_state_t * st
        = (reader_starting_state_t *) arg;

    uint64_t number;
    // While there is still a number to read
    while (fread(&number, sizeof(uint64_t), 1, st->stream) == 1)
    {
        // Convert the number from BigEndian
        number = be64toh(number);

        // Lock state mutex to protect waiting_list
        check(!pthread_mutex_lock(&mut_state),
                "pthread_mutex_lock");
        {
            factor_t new_factor = {number, 1, st->filename};
            // Divide the number by every factor in the prime list
            for (factor_t * it = list_begin(prime_list);
                    it != list_end(prime_list);
                    ++it)
            {
                divide_as_much_as_possible(&new_factor, it);
            }

            // Add the number to the waiting list if it's not 1
            if (new_factor.num != 1)
            {
                list_push(waiting_list, new_factor);

                // Push the full semaphore to indicate that a new number was
                // added to the waiting list
                check(!sem_post(&sem_full),
                        "sem_post");
            }
        }
        check(!pthread_mutex_unlock(&mut_state),
                "pthread_mutex_unlock");
    }

    debug("finished reading %s", st->filename);

    // Lock the state mutex to protect reader_count and prime_list
    check(!pthread_mutex_lock(&mut_state),
            "pthread_mutex_lock");
    {
        readers_active--;
        
        // If this is the last reader
        if (readers_active == 0)
        {
            factor_t * factor = find_prime_with_one_occurrence();
            // If there is a factor with a single occurrence in the prime
            // list, then, since no readers are left, it means that the
            // answer was found and that the program can terminate
            if (factor != NULL)
            {
                // Stop the main loop
                found = true;

                // Set the number used in the factorizers to 1, so that they
                // will instantly terminate
                to_fact.num = 1;

                // Print the answer
                print_solution(factor);
            }

            // The prime list is not useful anymore when there aren't any
            // readers left; we can thus safely free it
            list_free(prime_list);

            // Post the full semaphore one last time. This is done to prevent
            // the main thread from waiting on that semaphore indefinitely if
            // the waiting list is empty
            check(!sem_post(&sem_full),
                    "sem_post");
        }
    }
    check(!pthread_mutex_unlock(&mut_state),
            "pthread_mutex_unlock");

    // If the file descriptor is not stdin, then close it
    if (st->stream != stdin)
        check(!fclose(st->stream),
                "fclose");

    return NULL;
error:
    // In case of error in the check-s, exit the program
    exit(EXIT_FAILURE);
}

/**
 * Goes through the prime list looking for a prime with only one occurence.
 *
 * @return:
 * - a factor_t pointer to the prime factor if it was found;
 * - NULL otherwise.
 *
 * Note: this function is *not* thread-safe, and should only be used in a
 * section where the state mutex is locked to protect prime_list and to_fact.
 */
factor_t * find_prime_with_one_occurrence()
{
    for (factor_t *it = list_begin(prime_list);
            it != list_end(prime_list);
            it++)
    {
        if (it->occur == 1)
            return it;
    }

    return NULL;
}

