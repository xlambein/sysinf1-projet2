#ifndef __curl_getter_h__
#define __curl_getter_h__

/**
 * This file defines functions used to perform HTTP GET requests in a thread
 * and write the response into a file descriptor.
 */

#include <stdlib.h>

// Structure used to pass arguments to the thread function curl_getter()
typedef struct
{
    int fd; // A file descriptor
    const char * url; // An url
}
curl_getter_starting_state_t;

// Curl callback function, writes a buffer into a file descriptor
size_t write_callback(void *buffer, size_t size, size_t nmemb, void *userp);

// Thread function that GETs a file from an URL and writes it in a file
// descriptor
void *curl_getter(void *arg);

#endif

