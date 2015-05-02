#include "curl_getter.h"

// Source for the curl code:
// http://curl.haxx.se/libcurl/c/getinmemory.html

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <curl/curl.h>
#include <curl/easy.h>

#include "dbg.h"

/**
 * Writes a buffer into a file descriptor. Used as a curl callback for
 * curl_easy_perform().
 *
 * @buffer A pointer to a data buffer
 * @size The size of an element of data in the buffer
 * @nmemb The number of elements of data in the buffer
 * @userp A pointer to a file descriptor
 *
 * @returns:
 * - on success, the number of bytes written;
 * - on error, -1 is returned, and errno is set appropriately.
 * (see `man 2 write`)
 */
size_t write_callback(void *buffer, size_t size, size_t nmemb, void *userp)
{
    int fd = *((int *) userp);

    return write(fd, buffer, size*nmemb);
}

/**
 * Performs an HTTP GET request to an URL and writes the body into a file
 * descriptor.
 *
 * @arg A pointer to a curl_getter_starting_state_t structure, containing
 * - fd, the file descriptor and
 * - url, the url
 *
 * @returns: NULL
 * 
 * Note: at the end of the function, fg is closed.
 */
void *curl_getter(void *arg)
{
    curl_getter_starting_state_t * st
        = (curl_getter_starting_state_t *) arg;

    debug("curling \"%s\"...", st->url);

    // Initialize the curl handler
    CURL *curl_handle = curl_easy_init();
    check(curl_handle != NULL,
            "curl_easy_init");

    // Set the URL and the HTTP method
    curl_easy_setopt(curl_handle, CURLOPT_URL, st->url);
    curl_easy_setopt(curl_handle, CURLOPT_HTTPGET, 1);

    // This is so that curl follows HTTP 3xx redirections
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1);

    // Set the callback function that will handle the data
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);

    // Set the data for the callback function (a file descriptor)
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *) &st->fd);

    // Perform the request
    curl_easy_perform(curl_handle);

    // Clean up everything
    curl_easy_cleanup(curl_handle);
    close(st->fd);

    debug("curling \"%s\" done", st->url);

error:
    return NULL;
}

