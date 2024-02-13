#ifndef HASH_FUNCTIONS_H
#define HASH_FUNCTIONS_H

#include "functions.h"

#define HASH_TABLE_SIZE 8

struct functions_hash_table
{
    struct function *table[HASH_TABLE_SIZE];
};

// sets a function in the hash table.
// if the function does not exist, it is added at the beginning of the table.
// if the function exists, its value is updated.
// returns a pointer to the function, or NULL on error.
// the 'name' and 'value' parameters must be heap-allocated.
// they shall not be freed outside of the dedicated functions.
struct function *hash_function_set(char *name, struct ast *value);

// retrieves a function in the hash_table and returns it, or NULL on error.
// the 'name' parameter will not be freed inside the function.
struct function *hash_function_get(char *name);

// deletes ONE instance of the function from the hash table IF one exists.
// the 'name' parameter will not be freed inside the function.
void hash_function_del(char *name);

// frees all of the hash table.
// the table shall not be used afterwards.
void hash_function_destroy(void);

#endif /* ! HASH_FUNCTIONS_H */
