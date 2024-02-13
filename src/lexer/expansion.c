#include "expansion.h"

#include "../variables/shell_variables.h"
#include "string.h"

int is_special_char(char c)
{
    return c == '$' || c == '"' || c == '\\' || c == '`';
}

char *add_char_bis(char *word, char c, int *index, unsigned *word_size)
{
    if ((unsigned)*index >= *word_size)
    {
        *word_size *= 2;
        word = realloc(word, *word_size);
        if (word == NULL)
        {
            fprintf(stderr, "IO_read_word: realloc failed\n");
            return NULL;
        }
    }

    word[(*index)++] = c;

    return word;
}

char *handle_expension(char *word)
{
    unsigned word_size = 1;
    int index = 0;
    char *new_word = calloc(word_size, sizeof(char));
    if (!new_word)
        return NULL;

    ssize_t word_len = strlen(word);
    for (int i = 0; i < word_len; ++i)
    {
        if (word[i] == '\\') //$ Escaping
        {
            if (word[i + 1] && word[i + 1] == '$')
                ++i;

            new_word = add_char_bis(new_word, word[i], &index, &word_size);
        }

        else if (word[i] == '$')
        {
            if (!word[i + 1])
            {
                fprintf(stderr, "handle_expension: missing variable name\n");
                free(new_word);
                return NULL;
            }

            int bracket_delim = 0; // 0: no bracket, 1: ${, 2: $(
            ++i; // Skip dollar sign
            if (word[i] == '{')
            {
                if (!word[i + 1])
                {
                    fprintf(stderr,
                            "handle_expension: missing variable name\n");
                    free(new_word);
                    return NULL;
                }

                bracket_delim = 1;
                ++i; // Skip bracket
            }

            char *var_name = calloc(1, sizeof(char));
            int var_name_index = 0;
            unsigned var_name_size = 1;
            while (word[i] && !is_special_char(word[i]) && word[i] != ' '
                   && word[i] != '}') //$ Variable name
            {
                var_name = add_char_bis(var_name, word[i], &var_name_index,
                                        &var_name_size);
                ++i;
            }

            var_name =
                add_char_bis(var_name, '\0', &var_name_index, &var_name_size);

            if (bracket_delim == 1)
            {
                if (word[i] != '}')
                {
                    fprintf(stderr, "handle_expension: missing '}'\n");
                    return NULL;
                }
            }

            else
                --i;

            if (strcmp(var_name, "RANDOM") == 0)
            {
                char *random = get_RANDOM();
                if (random == NULL)
                {
                    fprintf(stderr, "handle_expension: get_RANDOM failed\n");
                    free(new_word);
                    free(var_name);
                    return NULL;
                }

                new_word =
                    add_char_bis(new_word, random[0], &index, &word_size);
                free(random);
            }

            else if (strcmp(var_name, "*") == 0 || strcmp(var_name, "@") == 0)
            {
                char *all = get_ALL();
                if (all == NULL)
                {
                    fprintf(stderr, "handle_expension: get_ALL failed\n");
                    free(new_word);
                    free(var_name);
                    return NULL;
                }

                for (size_t u = 0; u < strlen(all); ++u)
                    new_word =
                        add_char_bis(new_word, all[u], &index, &word_size);

                free(all);
            }

            else
            {
                // Check if the variable exists as an environment variable
                char *env_var = getenv(var_name);
                if (env_var != NULL)
                {
                    new_word =
                        realloc(new_word, word_size + strlen(env_var) + 1);

                    for (size_t u = 0; u < strlen(env_var); ++u)
                        new_word = add_char_bis(new_word, env_var[u], &index,
                                                &word_size);
                }

                else
                {
                    struct variable *var = hash_variable_get(var_name);
                    if (var != NULL)
                    {
                        new_word = realloc(new_word,
                                           word_size + strlen(var->value) + 1);

                        for (size_t u = 0; u < strlen(var->value); ++u)
                            new_word = add_char_bis(new_word, var->value[u],
                                                    &index, &word_size);
                    }
                }
            }

            free(var_name);
        }

        else
            new_word = add_char_bis(new_word, word[i], &index, &word_size);
    }

    new_word = add_char_bis(new_word, '\0', &index, &word_size);

    return new_word;
}
