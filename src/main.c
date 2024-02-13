#include <stdio.h>

#include "IO_Backend/io.h"
#include "ast/ast_exec.h"
#include "exit_codes.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "variables/shell_variables.h"

#define SIZEOF_OPTIONS 1

struct IO *parse_argv(int argc, char **argv, char options[SIZEOF_OPTIONS])
{
    int index = 1;
    // Parse options and determine the input source
    if (argc > 1)
    {
        if (strcmp(argv[index], "--pretty-print") == 0)
        {
            printf("PRETTY-PRINT: Activated.\n");
            options[0] = 1;
            ++index;
            if (argc == index)
            {
                // if no argument besides options is given, use IO_STDIN
                return IO_create(IO_STDIN, NULL);
            }
        }
        if (strcmp(argv[index], "-c") == 0)
        {
            // if "-c" is specified, it means the next string is the input
            return IO_create(IO_STRING, argv[index + 1]);
        }
        else
        {
            // otherwise, the input is the file
            if (fopen(argv[index], "r"))
                return IO_create(IO_FILE, argv[index]);
            else
                return NULL;
        }
    }
    else
    {
        // if no argument is given, use IO_STDIN
        return IO_create(IO_STDIN, NULL);
    }
}

static int cleanup_and_exit(struct token next, struct lexer *lexer,
                            int exit_code)
{
    token_free(next);
    lexer_free(lexer);
    hash_variable_destroy();
    hash_function_destroy();
    return exit_code;
}

int main(int argc, char **argv)
{
    // options[0] == pretty print
    char options[SIZEOF_OPTIONS] = { 0 };
    struct IO *io = parse_argv(argc, argv, options);
    if (io == NULL)
    {
        fprintf(stderr, "Usage: %s [--pretty-print] [-c] [input]\n", argv[0]);
        return -EC_UNKNOWN;
    }

    // Initialize lexer with the input
    struct lexer *lexer = lexer_new(io);
    if (lexer == NULL)
    {
        fprintf(stderr, "Error: Lexer initialization failed.\n");
        return -EC_UNKNOWN;
    }

    // Initialize shell variables
    shell_variables_init();

    // While we have lines to lex, parse, and execute, do so.
    int exit_code = 0;
    struct token next = lexer_peek(lexer);
    while (next.type != TOKEN_EOF && next.type != TOKEN_ERROR
           && !current_42sh_is_a_fork())
    {
        // Initialize AST
        struct ast *ast = NULL;

        // Parse input, check for parsing errors
        if (parse_input(&ast, lexer) != PARSER_OK)
        {
            fprintf(stderr, "Error: Parsing failed\n");
            return cleanup_and_exit(next, lexer, -EC_SYNTAX);
        }

        if (ast != NULL)
        {
            // optional pretty print
            if (options[0])
                ast_print(ast, 0);

            // Execute the AST
            exit_code = ast_exec(ast);
            if (exit_code == EC_COMMAND_NOT_FOUND)
            {
                fprintf(stderr, "Error: Command not found.\n");
            }
            else if (EC_EXIT_MIN <= exit_code && exit_code <= EC_EXIT_MAX)
            {
                ast_free(ast);
                // fprintf(stderr, "exit with code %i\n", exit_code -
                // EC_EXIT_MIN);
                return cleanup_and_exit(next, lexer, exit_code - EC_EXIT_MIN);
            }
        }

        ast_free(ast);
        token_free(next);
        next = lexer_peek(lexer);
    }

    // fprintf(stderr, "return with code %i\n", exit_code);
    return cleanup_and_exit(next, lexer,
                            exit_code < 0 ? -exit_code : exit_code);
}
