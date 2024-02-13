#ifndef HASH_VARIABLES_H
#define HASH_VARIABLES_H

#include "shell_variables.h"
#include "variables.h"

#define HASH_TABLE_SIZE 8

struct variables_hash_table
{
    struct variable *table[HASH_TABLE_SIZE];
};

// sets a variable in the hash table.
// if the variable does not exist, it is added at the beginning of the table.
// if the variable exists, its value is updated.
// returns a pointer to the variable, or NULL on error.
// the 'name' and 'value' parameters must be heap-allocated.
// they shall not be freed outside of the dedicated functions.
struct variable *hash_variable_set(char *name, char *value);

// retrieves a variable in the hash_table and returns it, or NULL on error.
// the 'name' parameter will not be freed inside the function.
struct variable *hash_variable_get(char *name);

void hash_variable_del(char *name);

// frees all of the hash table.
// the table shall not be used afterwards.
void hash_variable_destroy(void);

#endif /* ! HASH_VARIABLES_H */
