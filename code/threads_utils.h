#ifndef __threads_utils_h__
#define __threads_utils_h__

#include <pthread.h>
#include <semaphore.h>

#define lock(m) check(!pthread_mutex_lock(m), "pthread_mutex_lock");
#define unlock(m) check(!pthread_mutex_unlock(m), "pthread_mutex_unlock");

sem_t sem_full;
pthread_mutex_t mut_state;

#endif

