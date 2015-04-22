#include "factor_list.h"
#include <stdlib.h>

#define STARTING_SIZE 32

factor_list_t *new_list()
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

void free_list(factor_list_t *list)
{
    // Free the internal list then the structure
    free(list->list);
    free(list);
}

void push(factor_list_t *list, factor_t factor)
{
    // If there is not enough space
    if (list->size == list->cont_size)
    {
        // Double the size
        list->cont_size = 2 * list->size;
        
        // Copy the data
        factor_t *copy = (factor_t *) malloc(list->cont_size * sizeof(factor_t));
        for (int i = 0; i < list->cont_size; i++)
            copy[i] = list->list[i];
        
        // Replace the internal list with the copy
        free(list->list);
        list->list = copy;
    }
    
    // Add the factor at the end and increase the size
    *end(list) = factor;
    list->size++;
}

void remove(factor_list_t *list, factor_t *to_remove)
{
    // Decrease the size
    list->size--;
    
    // Move the last element over to the free position
    *to_remove = *end(list);
}

factor_t *begin(factor_list_t *list)
{
    return list->list;
}

factor_t *end(factor_list_t *list)
{
    return list->list + list->size;
}
