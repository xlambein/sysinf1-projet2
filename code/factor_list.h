#ifndef _FACTOR_LIST_H
#define _FACTOR_LIST_H

/**
 * This file defines structures and functions used to manipulate factors and
 * lists of factors.
 */

#include <stdint.h>

// Structure representing a factor
typedef struct
{
    uint64_t num;         // The number of the factor
    int occur;            // The occurrence count of the factor
    const char *filename; // The file where that factor comes from
}
factor_t;

// Structure representing a variable-size list of factors
typedef struct
{
    factor_t *list; // The array of factor_t used internally
    int size;       // The current number of factors in the list
    int cont_size;  // The size of the array used internally
}
factor_list_t;

// Creates a new factor_list_t
factor_list_t *list_new();

// Frees the memory allocated by a factor_list_t
void list_free(factor_list_t *list);

// Adds a factor to a factor_list_t
void list_push(factor_list_t *list, factor_t factor);

// Removes a factor from a factor_list_t
void list_remove(factor_list_t *list, factor_t *to_remove);

// Returns pointer to the beginning of the list, for list traversal
factor_t *list_begin(factor_list_t *list);

// Returns a pointer to the address directly after the end of the list, for
// list traversal
factor_t *list_end(factor_list_t *list);

#endif
