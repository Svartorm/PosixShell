#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "lexer.h"
#include "token.h"

// ==================================================================
// DISCLAIMER:
//
// This parser is very crude and might not work.
// It was coded to give a starting structure to parsing and hopefully
// allow us to implement ast execution next.
//
// It will most probably have to be rewritten later down the line.
// ==================================================================

enum parser_status
{
    PARSER_OK,
    PARSER_UNEXPECTED_TOKEN,
};

/**
 ** \brief Shortcut function. Frees arguments, sets *res to NULL,
 **        prints an error on stderr, and returns an error.
 */
enum parser_status error_handling(struct ast **res, struct ast *other_free,
                                  struct token *token, char *hint);

/**
 ** \brief Parses one shell 'input'.
 */
enum parser_status parse_input(struct ast **res, struct lexer *lexer);

#endif /* ! PARSER_H */
