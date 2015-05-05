#include "factor_list.h"
#include <stdlib.h>

#define STARTING_SIZE 32

/**
 * Allocates a new factor_list_t and its inner array with a size of
 * STARTING_SIZE.
 *
 * @returns: a pointer to the list.
 */
factor_list_t *list_new()
{
    // Allocate the structure
    factor_list_t *list = (factor_list_t *) malloc(sizeof(factor_list_t));
    if (list == NULL)
        exit(EXIT_FAILURE);
    
    // Start the list with a reasonable initial size
    list->cont_size = STARTING_SIZE;
    list->list = (factor_t *) malloc(list->cont_size * sizeof(factor_t));
    if (list->list == NULL)
        exit(EXIT_FAILURE);
    
    // The list is initially empty
    list->size = 0;
    
    return list;
}

/**
 * Frees the memory allocated by a factor_list_t and its inner array.
 *
 * @list a pointer to the factor_list_t
 */
void list_free(factor_list_t *list)
{
    // Free the internal list then the structure
    free(list->list);
    free(list);
}

/**
 * Adds a factor to a facto_list_t, doubling the size of its inner array if
 * necessary.
 *
 * @list a pointer to the factor_list_t
 * @factor the factor to add
 */
void list_push(factor_list_t *list, factor_t factor)
{
    // If there is not enough space
    if (list->size == list->cont_size)
    {
        // Double the size
        list->cont_size = 2 * list->size;
        list->list = (factor_t *) realloc(list->list, list->cont_size
                * sizeof(factor_t));
    }
    
    // Add the factor at the end and increase the size
    *list_end(list) = factor;
    list->size++;
}

/**
 * Removes a factor from a facto_list_t by moving it to the end of the list
 * and then reducing its size.
 *
 * @list a pointer to the factor_list_t
 * @to_remove the factor to remove
 */
void list_remove(factor_list_t *list, factor_t *to_remove)
{
    // Decrease the size
    list->size--;
    
    // Move the last element over to the free position
    *to_remove = *list_end(list);
}

/**
 * Returns a pointer to the first element of a factor_list_t's inner array,
 * used for list traversal.
 *
 * @list a pointer to the factor_list_t
 *
 * @returns: a pointer to the first element of a factor_list_t's inner array.
 */
factor_t *list_begin(factor_list_t *list)
{
    return list->list;
}

/**
 * Returns a pointer to the address right after the last element of a
 * factor_list_t's inner array, used for list traversal.
 *
 * @list a pointer to the factor_list_t
 *
 * @returns: a pointer to the address right after the last element of a
 * factor_list_t's inner array.
 */
factor_t *list_end(factor_list_t *list)
{
    return list->list + list->size;
}
