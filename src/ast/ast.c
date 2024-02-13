#define _POSIX_C_SOURCE 200809L

#include "ast.h"

#include <string.h>

#include "../functions/hash_functions.h"

struct ast *ast_new(enum ast_type type, char *value)
{
    struct ast *new = calloc(1, sizeof(struct ast));
    if (!new)
        return NULL;
    new->type = type;
    new->value = value;
    return new;
}

void ast_free(struct ast *ast)
{
    if (ast == NULL)
        return;
    else if (ast->type == AST_FUNCDEC)
    {
        struct function *f = hash_function_get(ast->value);
        if (f != NULL && f->value == ast_get_son(ast, 0))
        {
            // we do not want to free a registered function name nor its ast
            ast_free(ast->right_brother);
            // free(ast->value);
            free(ast);
            return;
        }
        // otherwise, the function body has not been registered, we can free it
    }

    if (ast->value != NULL)
    {
        free(ast->value);
    }
    ast_free(ast->left_son);
    ast_free(ast->right_brother);
    free(ast);
}

void expand(struct ast *ast)
{
    if (ast == NULL || ast->type != AST_EXPANSION)
        return;

    struct ast *head = ast->left_son;
    for (size_t i = 0; i < ast->nb_sons; ++i)
    {
        char *word = head->value;
        if (head->type == AST_EXPARG_DQ)
            word = handle_expension(word);

        ssize_t s = ast->value == NULL ? 0 : strlen(ast->value);
        ast->value = realloc(ast->value, s + strlen(word) + 1);
        ast->value[s] = '\0';
        strcat(ast->value, word);

        if (head->type == AST_EXPARG_DQ)
            free(word);

        if (head->right_brother == NULL)
            break;
        head = head->right_brother;
    }
}

struct ast *ast_get_son(struct ast *ast, size_t index)
{
    if (index >= ast->nb_sons)
        return NULL;

    struct ast *nth_son = ast->left_son;

    while (nth_son && index != 0)
    {
        nth_son = nth_son->right_brother;
        --index;
    }

    //? Expand double-quoted arguments
    if (nth_son && nth_son->type == AST_EXPANSION)
    {
        if (nth_son->value)
            free(nth_son->value);
        nth_son->value = NULL;
        expand(nth_son);
    }
    return nth_son;
}

struct ast *ast_insert_son(struct ast *ast, size_t index, struct ast *new_son)
{
    if (index == 0)
    {
        new_son->right_brother = ast->left_son;
        ast->left_son = new_son;
        ++ast->nb_sons;
        return new_son;
    }

    struct ast *left_brother = ast_get_son(ast, index - 1);
    if (!left_brother)
        return NULL;

    new_son->right_brother = left_brother->right_brother;
    left_brother->right_brother = new_son;
    ++ast->nb_sons;
    return new_son;
}

struct ast *ast_append_son(struct ast *ast, struct ast *new_son)
{
    return ast_insert_son(ast, ast->nb_sons, new_son);
}

static char *array_ast_type_string[] = {
    [AST_COMMAND] = "COMMAND",
    [AST_ARGUMENT] = "ARGUMENT",
    [AST_COMMAND_LIST] = "COMMAND_LIST",
    [AST_PIPELINE] = "PIPELINE",
    [AST_PIPE] = "PIPE",
    [AST_CONDITIONAL] = "CONDITIONAL",
    [AST_WHILE] = "WHILE",
    [AST_UNTIL] = "UNTIL",
    [AST_FOR] = "FOR",
    [AST_NOT] = "NOT",
    [AST_AND] = "AND",
    [AST_OR] = "OR",
    [AST_REDIR_FOLDER] = "REDIR_FOLDER",
    [AST_REDIR_IN] = "REDIR_IN",
    [AST_REDIR_OUT] = "REDIR_OUT",
    [AST_REDIR_APP_OUT] = "REDIR_APP_OUT",
    [AST_REDIR_DUP_IN] = "REDIR_DUP_IN",
    [AST_REDIR_DUP_OUT] = "REDIR_DUP_OUT",
    [AST_REDIR_RW] = "REDIR_RW",
    [AST_FUNCTION] = "FUNCTION",
    [AST_VARIABLE] = "VARIABLE",
    [AST_EXPANSION] = "EXPANSION",
    [AST_SUBSHELL] = "SUBSHELL",
    [AST_FUNCDEC] = "FUNCDEC",
};

char *ast_type_string(enum ast_type type)
{
    return array_ast_type_string[type];
}

void ast_print(const struct ast *ast, int level)
{
    printf("%i\n", (ast == NULL) + level);
}
