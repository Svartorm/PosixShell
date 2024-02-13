#include "variables.h"

#include <stdlib.h>

// WARNING: you should not use this function on your own.
//          it should only be used by internal functions.
// returns a new variable structure, or NULL on error.
// the 'name' and 'value' parameters must be heap-allocated.
// they shall not be freed outside of the dedicated functions.
struct variable *variable_new(char *name, char *value)
{
    struct variable *new = calloc(1, sizeof(struct variable));
    if (!new)
        return NULL;
    new->name = name;
    new->value = value;
    return new;
}
