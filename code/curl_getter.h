#ifndef __curl_getter_h__
#define __curl_getter_h__

#include <stdlib.h>

typedef struct
{
    int fd;
    const char * url;
}
curl_getter_starting_state_t;

size_t write_callback(void *buffer, size_t size, size_t nmemb, void *userp);

void *curl_getter(void *arg);

#endif

