#include "io.h"

FILE *StringToFile(char *string)
{
    return fmemopen(string, strlen(string), "r");
}

FILE *StdinToFile(void)
{
    FILE *fptr;
    char buffer[1024]; // Buffer pour stocker les données lues

    // Ouvrir un fichier pour l'écriture
    fptr = fopen("output.txt", "w");
    if (fptr == NULL)
    {
        fprintf(stderr, "Error opening file.\n");
        return NULL;
    }

    // Lire les données de stdin et les écrire dans le fichier
    while (fgets(buffer, sizeof(buffer), stdin))
        fputs(buffer, fptr);

    fclose(fptr);

    fptr = fopen("output.txt", "r");
    if (fptr == NULL)
    {
        fprintf(stderr, "Error reopening file for reading.\n");
        return NULL; // Return NULL to indicate error
    }

    return fptr;
}

struct IO *IO_create(enum IO_type type, char *input)
{
    struct IO *io = calloc(sizeof(struct IO), 1);
    if (io == NULL)
    {
        fprintf(stderr, "IO_create: malloc failed\n");
        return NULL;
    }

    switch (type)
    {
    case IO_STDIN:
        io->input = StdinToFile();
        break;

    case IO_FILE:
        io->input = fopen(input, "r");
        break;

    case IO_STRING:
        io->input = StringToFile(input);
        break;

    default:
        fprintf(stderr, "IO_create: invalid IO_type\n");
        free(io);
        return NULL;
    }

    save(io);
    return io;
}

void IO_free(struct IO *io)
{
    if (io)
    {
        if (io->input)
            fclose(io->input);

        free(io);
    }
}

char get_char(struct IO *io)
{
    return fgetc(io->input);
}

char peek_char(struct IO *io)
{
    char c = fgetc(io->input);
    ungetc(c, io->input);
    return c;
}

void save(struct IO *io)
{
    io->saved_pos = ftell(io->input);
}

void restore(struct IO *io)
{
    fseek(io->input, io->saved_pos, SEEK_SET);
}
