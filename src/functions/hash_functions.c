#include "hash_functions.h"

#include <stdlib.h>

#include "ll_functions.h"

static struct functions_hash_table funcs = { .table = { NULL } };

// returns the hashed value of the string.
static size_t hash_string2(char *string)
{
    return (*string) % HASH_TABLE_SIZE;
}

// sets a function in the hash table.
// if the function does not exist, it is added at the beginning of the table.
// if the function exists, its value is updated.
// returns a pointer to the function, or NULL on error.
// the 'name' and 'value' parameters must be heap-allocated.
// they shall not be freed outside of the dedicated functions.
struct function *hash_function_set(char *name, struct ast *value)
{
    size_t hash = hash_string2(name);
    return ll_function_set(&funcs.table[hash], name, value);
}

// retrieves a function in the hash_table and returns it, or NULL on error.
// the 'name' parameter will not be freed inside the function.
struct function *hash_function_get(char *name)
{
    size_t hash = hash_string2(name);
    return ll_function_get(&funcs.table[hash], name);
}

// deletes ONE instance of the function from the hash table IF one exists.
// the 'name' parameter will not be freed inside the function.
void hash_function_del(char *name)
{
    size_t hash = hash_string2(name);
    ll_function_del(&funcs.table[hash], name);
}

// frees all of the hash table.
// the table shall not be used afterwards.
void hash_function_destroy(void)
{
    for (size_t i = 0; i < HASH_TABLE_SIZE; ++i)
        ll_function_destroy(&funcs.table[i]);
}
