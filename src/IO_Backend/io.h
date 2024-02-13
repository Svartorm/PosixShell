#ifndef IO_H
#define IO_H

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum IO_type
{
    IO_STDIN,
    IO_FILE,
    IO_STRING
};

struct IO
{
    FILE *input;
    long saved_pos;
};

/*
** \brief Creates a new file from a string.
*/
FILE *StringToFile(char *string);

/*
** \brief Creates a new IO struct given an input string.
*/
struct IO *IO_create(enum IO_type type, char *input);

/*
** \brief Free the given IO struct and closes its input file.
*/
void IO_free(struct IO *io);

/*
** \brief Returns the next character
*/
char get_char(struct IO *io);

/*
** \brief Returns the next character without moving the cursor
*/
char peek_char(struct IO *io);

/*
** \brief Saves the current position of the cursor
*/
void save(struct IO *io);

/*
** \brief Restores the cursor to the last saved position
*/
void restore(struct IO *io);

#endif // IO_H
