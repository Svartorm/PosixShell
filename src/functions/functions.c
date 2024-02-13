#include "functions.h"

#include <stdlib.h>

// WARNING: you should not use this function on your own.
//          it should only be used by internal functions.
// returns a new function structure, or NULL on error.
// the 'name' and 'value' parameters must be heap-allocated.
// they shall not be freed outside of the dedicated functions.
struct function *function_new(char *name, struct ast *value)
{
    struct function *new = calloc(1, sizeof(struct function));
    if (!new)
        return NULL;
    new->name = name;
    new->value = value;
    return new;
}
