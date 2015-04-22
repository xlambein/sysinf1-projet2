#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <stdbool.h>

#include "dbg.h"

#define ARGNAME_MAXTHREADS "-maxthreads"
#define ARGNAME_STDIN "-stdin"
#define PREFIX_URL "http://"
#define PREFIX_URL_LENGTH 7

sem_t sem_full;
pthread_mutex_t mut_state;

int main(int argc, char *argv[])
{
    int maxthreads = 1;
    bool readFromStdin = false;
    bool found = false;

    //TODO: initialize semaphores and mutexes
    check(!sem_init(&sem_full, 0, 0),
            "sem_init");
    check(!pthread_mutex_init(&mut_state, NULL),
            "pthread_mutex_init");

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], ARGNAME_MAXTHREADS) && i < argc-1)
        {
            maxthreads = atoi(argv[i+1]);
            i++;
        }
        else if (!readFromStdin && strcmp(argv[i], ARGNAME_STDIN))
        {
            readFromStdin = true;
            //TODO: spawn a stdin reading thread
        }
        else if (strncmp(argv[i], PREFIX_URL, PREFIX_URL_LENGTH))
        {
            //TODO: spawn a network reading thread
        }
        else
        {
            //TODO: spawn a reading thread
        }
    }

    while (!found)
    {
        //TODO: spawn maxthreads threads

        //TODO: join maxthreads threads
        
        pthread_mutex_lock(&mut_state);
            //TODO: stuff
        pthread_mutex_unlock(&mut_state);
    } 

    check(!sem_destroy(&sem_full),
            "sem_destroy");
    check(!pthread_mutex_destroy(&mut_state),
            "pthread_mutex_destroy");

    return EXIT_SUCCESS;
error:
    return EXIT_FAILURE;
}
