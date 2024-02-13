#include "ll_variables.h"

#include <stdlib.h>
#include <string.h>

// if the variable does not exist, it is added at the beginning of the list.
// if the variable exists, its value is updated.
// returns the pointer to the variable, or NULL on error.
//
// the 'name' and 'value' parameters must be heap-allocated.
// they shall not be freed outside of the dedicated functions.
struct variable *ll_variable_set(struct variable **list, char *name,
                                 char *value)
{
    // if list is empty, add the new element and return it
    if (list == NULL || *list == NULL)
    {
        struct variable *new = variable_new(name, value);
        if (!new)
            return NULL;
        *list = new;
        return new;
    }

    // search for the old variable, if it exists
    struct variable *current = *list;
    while (current && strcmp(current->name, name) != 0)
    {
        current = current->next;
    }

    if (current) // if the variable exists, update it
    {
        free(name);
        free(current->value);
        current->value = value;
        return current;
    }
    else // otherwise, create a new variable
    {
        struct variable *new = variable_new(name, value);
        if (!new)
            return NULL;
        new->next = *list;
        *list = new;
        return new;
    }
}

// retrieves a variable in the list and returns it, or NULL on error.
// the 'name' parameter will not be freed inside the function.
struct variable *ll_variable_get(struct variable **list, char *name)
{
    // if the list is empty, then the variable does not exist
    if (list == NULL || *list == NULL)
        return NULL;

    // search for the old variable, if it exists
    struct variable *current = *list;
    while (current && strcmp(current->name, name) != 0)
    {
        current = current->next;
    }

    // return search result
    return current;
}

// deletes ONE instance of a variable from the list IF it exists.
// the 'name' parameter will not be freed inside the function.
void ll_variable_del(struct variable **list, char *name)
{
    // if list is empty, there is nothing to delete
    if (list == NULL || *list == NULL)
        return;

    // if the variable is the first in the list, delete it
    struct variable *prev = *list;
    if (strcmp(prev->name, name) == 0)
    {
        *list = prev->next;
        free(prev->name);
        free(prev->value);
        free(prev);
        return;
    }

    // otherwise, search for the old variable, if it exists
    struct variable *current = prev->next;
    while (current && strcmp(current->name, name) != 0)
    {
        prev = current;
        current = current->next;
    }

    if (current) // if the variable exists, delete it
    {
        prev->next = current->next;
        free(current->name);
        free(current->value);
        free(current);
    }
}

// frees all of the variable list.
// the list may be used afterwards.
void ll_variable_destroy(struct variable **list)
{
    // if the list is empty, then there is nothing to free
    if (list == NULL || *list == NULL)
        return;

    // free every variable
    struct variable *current = *list;
    while (current)
    {
        struct variable *next = current->next;
        free(current->name);
        free(current->value);
        free(current);
        current = next;
    }

    // set the list to NULL
    *list = NULL;
}
