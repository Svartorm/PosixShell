#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "ast.h"

struct function
{
    char *name;
    struct ast *value;
    struct function *next;
};

// WARNING: you should not use this function on your own.
//          it should only be used by internal functions.
// returns a new function structure, or NULL on error.
// the 'name' and 'value' parameters must be heap-allocated.
// they shall not be freed outside of the dedicated functions.
struct function *function_new(char *name, struct ast *value);

#endif /* ! FUNCTIONS_H */
