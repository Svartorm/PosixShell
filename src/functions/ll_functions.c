#include "ll_functions.h"

#include <stdlib.h>
#include <string.h>

// sets a function in the list.
// if the function does not exist, it is added at the beginning of the list.
// if the function exists, its value is updated.
// returns the pointer to the function, or NULL on error.
//
// the 'name' and 'value' parameters must be heap-allocated.
// they shall not be freed outside of the dedicated functions.
struct function *ll_function_set(struct function **list, char *name,
                                 struct ast *value)
{
    // if list is empty, add the new element and return it
    if (list == NULL || *list == NULL)
    {
        struct function *new = function_new(name, value);
        if (!new)
            return NULL;
        *list = new;
        return new;
    }

    // search for the old function, if it exists
    struct function *current = *list;
    while (current && strcmp(current->name, name) != 0)
    {
        current = current->next;
    }

    if (current) // if the function exists, update it
    {
        free(current->name); // for double-free purposes
        current->name = name; // for double-free purposes
        ast_free(current->value);
        current->value = value;
        return current;
    }
    else // otherwise, create a new function
    {
        struct function *new = function_new(name, value);
        if (!new)
            return NULL;
        new->next = *list;
        *list = new;
        return new;
    }
}

// retrieves a function in the list and returns it, or NULL on error.
// the 'name' parameter will not be freed inside the function.
struct function *ll_function_get(struct function **list, char *name)
{
    // if the list is empty, then the function does not exist
    if (list == NULL || *list == NULL)
        return NULL;

    // search for the old function, if it exists
    struct function *current = *list;
    while (current && strcmp(current->name, name) != 0)
    {
        current = current->next;
    }

    // return search result
    return current;
}

// deletes ONE instance of a function from the list IF it exists.
// the 'name' parameter will not be freed inside the function.
void ll_function_del(struct function **list, char *name)
{
    // if list is empty, there is nothing to delete
    if (list == NULL || *list == NULL)
        return;

    // if the function is the first in the list, delete it
    struct function *prev = *list;
    if (strcmp(prev->name, name) == 0)
    {
        *list = prev->next;
        free(prev->name);
        ast_free(prev->value);
        free(prev);
        return;
    }

    // otherwise, search for the old function, if it exists
    struct function *current = prev->next;
    while (current && strcmp(current->name, name) != 0)
    {
        prev = current;
        current = current->next;
    }

    if (current) // if the function exists, delete it
    {
        prev->next = current->next;
        free(current->name);
        ast_free(current->value);
        free(current);
    }
}

// frees all of the function list.
// the list may be used afterwards.
void ll_function_destroy(struct function **list)
{
    // if the list is empty, then there is nothing to free
    if (list == NULL || *list == NULL)
        return;

    // free every function
    struct function *current = *list;
    while (current)
    {
        struct function *next = current->next;
        free(current->name);
        ast_free(current->value);
        free(current);
        current = next;
    }
    *list = NULL;
}
