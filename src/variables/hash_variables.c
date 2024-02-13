#include "hash_variables.h"

#include <stdlib.h>

#include "ll_variables.h"

static struct variables_hash_table vars = { .table = { NULL } };

// returns the hashed value of the string.
static size_t hash_string(char *string)
{
    return (*string) % HASH_TABLE_SIZE;
}

// sets a variable in the hash table.
// if the variable does not exist, it is added at the beginning of the table.
// if the variable exists, its value is updated.
// returns a pointer to the variable, or NULL on error.
// the 'name' and 'value' parameters must be heap-allocated.
// they shall not be freed outside of the dedicated functions.
struct variable *hash_variable_set(char *name, char *value)
{
    size_t hash = hash_string(name);
    return ll_variable_set(&vars.table[hash], name, value);
}

// retrieves a variable in the hash_table and returns it, or NULL on error.
// the 'name' parameter will not be freed inside the function.
struct variable *hash_variable_get(char *name)
{
    size_t hash = hash_string(name);
    return ll_variable_get(&vars.table[hash], name);
}

void hash_variable_del(char *name)
{
    size_t hash = hash_string(name);
    ll_variable_del(&vars.table[hash], name);
}

// frees all of the hash table.
// the table shall not be used afterwards.
void hash_variable_destroy(void)
{
    for (size_t i = 0; i < HASH_TABLE_SIZE; ++i)
        ll_variable_destroy(&vars.table[i]);
}
