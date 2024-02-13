#ifndef LL_FUNCTIONS_H
#define LL_FUNCTIONS_H

#include "functions.h"

// ==================================================================
// ABOUT THE FUNCTIONS LIST
// The list is a linked-list of 'struct function's.
//
// We use a DOUBLE POINTER as the linked list, because it allows
// us to not worry about managing the list (the functions update
// the list by themselves), and the return values are clear.
// ==================================================================

// sets a function in the list.
// if the function does not exist, it is added at the beginning of the list.
// if the function exists, its value is updated.
// returns a pointer to the function, or NULL on error.
// the 'name' and 'value' parameters must be heap-allocated.
// they shall not be freed outside of the dedicated functions.
struct function *ll_function_set(struct function **list, char *name,
                                 struct ast *value);

// retrieves a function in the list and returns it, or NULL on error.
// the 'name' parameter will not be freed inside the function.
struct function *ll_function_get(struct function **list, char *name);

// deletes ONE instance of a function from the list IF it exists.
// the 'name' parameter will not be freed inside the function.
void ll_function_del(struct function **list, char *name);

// frees all of the function list.
// the list may be used afterwards.
void ll_function_destroy(struct function **list);

#endif /* ! LL_FUNCTIONS_H */
