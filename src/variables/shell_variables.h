#ifndef SHELL_VARIABLES_H
#define SHELL_VARIABLES_H

#include "hash_variables.h"

/*
 * Initializes the shell variables: $#, $?, etc.
 */
void shell_variables_init(void);

/*
 * Converts an integer to a string. Needs to be freed.
 */
char *itoa(int num);

/*
 * Returns a random number between 0 and 32767.
 */
char *get_RANDOM(void);

/*
 * Returns the current working directory.
 */
char *get_PWD(void);

char *get_ALL(void);

#endif // SHELL_VARIABLES_H