#ifndef LL_VARIABLES_H
#define LL_VARIABLES_H

#include "variables.h"

// ==================================================================
// ABOUT THE VARIABLES LIST
// The list is a linked-list of 'struct variable's.
//
// We use a DOUBLE POINTER as the linked list, because it allows
// us to not worry about managing the list (the functions update
// the list by themselves), and the return values are clear.
// ==================================================================

// sets a variable in the list.
// if the variable does not exist, it is added at the beginning of the list.
// if the variable exists, its value is updated.
// returns a pointer to the variable, or NULL on error.
// the 'name' and 'value' parameters must be heap-allocated.
// they shall not be freed outside of the dedicated functions.
struct variable *ll_variable_set(struct variable **list, char *name,
                                 char *value);

// retrieves a variable in the list and returns it, or NULL on error.
// the 'name' parameter will not be freed inside the function.
struct variable *ll_variable_get(struct variable **list, char *name);

// deletes ONE instance of a variable from the list IF it exists.
// the 'name' parameter will not be freed inside the function.
void ll_variable_del(struct variable **list, char *name);

// frees all of the variable list.
// the list may be used afterwards.
void ll_variable_destroy(struct variable **list);

#endif /* ! LL_VARIABLES_H */
