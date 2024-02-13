#ifndef AST_H
#define AST_H

#include <stdio.h>
#include <stdlib.h>

#include "../lexer/expansion.h"

enum ast_type
{
    AST_COMMAND, // For individual commands
    AST_ARGUMENT, // For command arguments
    AST_COMMAND_LIST, // For command lists
    AST_PIPELINE, // For command pipelines
    AST_PIPE, // For pipe redirection
    AST_CONDITIONAL, // For conditional statments (if, else)
    AST_WHILE, // For 'while' loops
    AST_UNTIL, // For 'until' loops

    AST_FOR, // For 'for' loops
    AST_NOT, // For ! operator
    AST_AND, // For && operator
    AST_OR, // For || operator

    AST_REDIR_FOLDER, // Contains all redirections relative to a command
    AST_REDIR_IN, // For '<' redirections
    AST_REDIR_OUT, // For '>' '>|' redirections
    AST_REDIR_APP_OUT, // For '>>' redirections
    AST_REDIR_DUP_IN, // For '<&' redirections
    AST_REDIR_DUP_OUT, // For '>&' redirections
    AST_REDIR_RW, // For '<>' redirections
    AST_INVALID,

    AST_FUNCTION, // For function definitions
    AST_VARIABLE, // For variable assignments
    AST_EXPANSION, // For expansions (variable, arithmetic)
    AST_EXPARG_NORM, // For arguments that are not expansions
    AST_EXPARG_DQ, // For arguments that are double-quoted expansions
    AST_SUBSHELL, // For subshell execution
    AST_FUNCDEC, // For function declaration
};

struct ast
{
    enum ast_type type; ///< The kind of node we're dealing with
    char *value; ///< String from which node is derived from
    struct ast *left_son; ///< First son of node, starting from the left
    struct ast *right_brother; ///< Right brother of node
    size_t nb_sons; ///< Number of sons
};

/**
 ** \brief Allocate a new ast with the given type
 */
struct ast *ast_new(enum ast_type type, char *value);

/**
 ** \brief Recursively free the given ast
 */
void ast_free(struct ast *ast);

/**
 ** \brief Get son at specified index. Indexes start at 0. NULL if error.
 */
struct ast *ast_get_son(struct ast *ast, size_t index);

/**
 ** \brief Insert son at specified index. Indexes start at 0. NULL if error.
 */
struct ast *ast_insert_son(struct ast *ast, size_t index, struct ast *new_son);

/**
 ** \brief Append son at the end of the other sons. NULL if error.
 */
struct ast *ast_append_son(struct ast *ast, struct ast *new_son);

/**
 ** \brief gets a string corresponding to the ast type.
 */
char *ast_type_string(enum ast_type type);

/**
 ** \brief Print the ast
 */
void ast_print(const struct ast *node, int level);

#endif /* ! AST_H */
