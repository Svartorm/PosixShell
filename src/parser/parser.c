#define _POSIX_C_SOURCE 200809L
#include "parser.h"

#include <stdio.h>

#include "lexer/lexer.h"

// ==================================================================
static int could_be_word(struct token token);
static enum parser_status parse_list(struct ast **res, struct lexer *lexer);
static enum parser_status parse_and_or(struct ast **res, struct lexer *lexer);
static enum parser_status helper_pipeline(struct ast **res,
                                          struct lexer *lexer);
static enum parser_status parse_pipeline(struct ast **res, struct lexer *lexer);
static enum parser_status parse_command(struct ast **res, struct lexer *lexer);
static enum parser_status parse_simple_command(struct ast **res,
                                               struct lexer *lexer);
static enum parser_status parse_simple_command2(struct ast **res,
                                                struct lexer *lexer,
                                                struct ast *command_list,
                                                struct ast *redir_folder);
static struct ast *handle_expandable_token(struct token token);
static enum parser_status parse_element(struct ast **res, struct lexer *lexer);
static enum parser_status parse_shell_command(struct ast **res,
                                              struct lexer *lexer);
static enum parser_status helper_parse_if(struct ast **res,
                                          struct lexer *lexer);
static enum parser_status parse_rule_if(struct ast **res, struct lexer *lexer);
static enum parser_status parse_else_clause(struct ast **res,
                                            struct lexer *lexer);
static int ends_compound_list(struct token token);
static enum parser_status parse_compound_list(struct ast **res,
                                              struct lexer *lexer);
static enum parser_status parse_while_until(struct ast **res,
                                            struct lexer *lexer);
static enum parser_status parse_for(struct ast **res, struct lexer *lexer);
static enum parser_status parse_redirection(struct ast **res,
                                            struct lexer *lexer);
static enum parser_status parse_subshell(struct ast **res, struct lexer *lexer);
static enum parser_status parse_funcdec(struct ast **res, struct lexer *lexer);
static int is_redirection_token(struct token token);
static char *get_var_name(char *str);
static char *remove_and_free_first_x_chars(char *str, int x);
static char *get_var_value(char *str);
// ==================================================================

enum parser_status error_handling(struct ast **res, struct ast *other_free,
                                  struct token *token, char *hint)
{
    if (other_free)
        ast_free(other_free);
    if (res && *res)
        ast_free(*res);
    if (token)
        token_free(*token);

    *res = NULL;
    fprintf(stderr, "Parser received an error. Hint: '%s'.\n", hint);
    return PARSER_UNEXPECTED_TOKEN;
}

static struct token discard_token_type_all(struct lexer *lexer,
                                           enum token_type discard_type)
{
    struct token next = lexer_peek(lexer);
    if (next.type == discard_type)
    {
        token_free(lexer_pop(lexer));
        token_free(next);
        next = lexer_peek(lexer);
    }
    return next;
}

static int could_be_redir(struct token token)
{
    enum token_type type = token.type;
    return type == TOKEN_REDIR_IN || type == TOKEN_REDIR_OUT
        || type == TOKEN_REDIR_APP_OUT || type == TOKEN_REDIR_DUP_IN
        || type == TOKEN_REDIR_DUP_OUT || type == TOKEN_REDIR_RW
        || type == TOKEN_IONUMBER;
}

static int could_be_word(struct token token)
{
    enum token_type type = token.type;
    return !could_be_redir(token) && type != TOKEN_SEMI_COL
        && type != TOKEN_PIPE && type != TOKEN_AND && type != TOKEN_OR
        && type != TOKEN_LF && type != TOKEN_EOF && type != TOKEN_ERROR
        && type != TOKEN_RPAREN && type != TOKEN_LPAREN;
    //&& type != TOKEN_RBRACKET && type != TOKEN_LBRACKET;
}

// input =
// list '\n'
// | list EOF
// | '\n'
// | EOF
// ;
enum parser_status parse_input(struct ast **res, struct lexer *lexer)
{
    struct token next = lexer_peek(lexer);
    // if we do not have a command on this line
    if (next.type == TOKEN_EOF || next.type == TOKEN_LF)
    {
        *res = NULL;
        // if we have not reached the end, then we must go to the next line
        if (next.type == TOKEN_LF)
            token_free(lexer_pop(lexer));
    }
    // otherwise
    else
    {
        enum parser_status status = parse_list(res, lexer);
        if (status == PARSER_UNEXPECTED_TOKEN)
        {
            return error_handling(res, NULL, &next, "parse_input");
        }
        token_free(next);
        next = lexer_peek(lexer);
        if (next.type != TOKEN_EOF && next.type != TOKEN_LF)
        {
            return error_handling(res, NULL, &next,
                                  "parse_input expected EOF or LF");
        }
    }
    token_free(next);
    return PARSER_OK;
}

// list = and_or { ( ';' | '&' ) and_or } [ ';' | '&' ] ;
static enum parser_status parse_list(struct ast **res, struct lexer *lexer)
{
    struct token next = lexer_peek(lexer);
    if (next.type == TOKEN_LPAREN)
    {
        token_free(next);
        return parse_command(res, lexer);
    }
    if (!could_be_word(next))
        return error_handling(res, NULL, &next, "parse_list expected WORD");

    // create new ast node -> list of command
    struct ast *main = ast_new(AST_COMMAND_LIST, NULL);
    if (!main)
        return error_handling(res, NULL, &next, "parse_list MEMORY");
    // while there is valid input
    while (could_be_word(next))
    {
        struct ast *sub = NULL;
        // parse individual command
        if (parse_and_or(&sub, lexer) != PARSER_OK)
            return error_handling(res, main, &next, "parse_list");

        // append the parsed command sub as a child of the last command list
        ast_append_son(main, sub);

        // check if the next token is ';'
        token_free(next);
        next = lexer_peek(lexer);
        if (next.type == TOKEN_SEMI_COL)
        {
            token_free(next);
            token_free(lexer_pop(lexer));
            next = lexer_peek(lexer);
        }
        else
            break;
    }

    // set result to the constructed list
    *res = main;
    // free the last token peeked
    token_free(next);
    return PARSER_OK;
}

// and_or = pipeline { ( '&&' | '||' ) {'\n'} pipeline } ;
static enum parser_status parse_and_or(struct ast **res, struct lexer *lexer)
{
    // parse first pipeline rule
    struct ast *current = NULL;
    if (parse_pipeline(&current, lexer) != PARSER_OK)
        return error_handling(res, NULL, NULL, "parse_and_or");

    // while there are and/or conditions
    struct token next = lexer_peek(lexer);
    while (next.type == TOKEN_AND || next.type == TOKEN_OR)
    {
        // build a new ast for and/or, set its first son as the previous ast
        struct ast *new =
            ast_new(next.type == TOKEN_AND ? AST_AND : AST_OR, NULL);
        if (!new)
            return error_handling(res, current, &next, "parse_and_or MEMORY");
        ast_append_son(new, current);

        // remove any linefeeds
        token_free(next);
        token_free(lexer_pop(lexer));
        next = lexer_peek(lexer);
        while (next.type == TOKEN_LF)
        {
            token_free(next);
            token_free(lexer_pop(lexer));
            next = lexer_peek(lexer);
        }

        // parse the second son
        struct ast *second = NULL;
        if (parse_pipeline(&second, lexer) != PARSER_OK)
            return error_handling(res, new, &next, "parse_and_or");
        ast_append_son(new, second);

        current = new;
        token_free(next);
        next = lexer_peek(lexer);
    }

    *res = current;
    token_free(next);
    return PARSER_OK;
}

static enum parser_status helper_pipeline(struct ast **res, struct lexer *lexer)
{
    // parse first pipeline rule
    struct ast *current = NULL;
    if (parse_command(&current, lexer) != PARSER_OK)
        return error_handling(res, NULL, NULL, "helper_pipeline");

    // while there are and/or conditions
    struct token next = lexer_peek(lexer);
    while (next.type == TOKEN_PIPE)
    {
        token_free(lexer_pop(lexer));
        // build a new ast for the pipe, set its first son as the previous ast
        struct ast *new = ast_new(AST_PIPE, NULL);
        if (!new)
            return error_handling(res, current, &next,
                                  "helper_pipeline MEMORY");
        ast_append_son(new, current);

        // remove any linefeeds
        token_free(next);
        next = lexer_peek(lexer);
        while (next.type == TOKEN_LF)
        {
            token_free(next);
            token_free(lexer_pop(lexer));
            next = lexer_peek(lexer);
        }

        // parse the second son
        struct ast *second = NULL;
        if (parse_command(&second, lexer) != PARSER_OK)
            return error_handling(res, new, &next, "helper_pipeline");
        ast_append_son(new, second);

        current = new;
        token_free(next);
        next = lexer_peek(lexer);
    }

    *res = current;
    token_free(next);
    return PARSER_OK;
}

// pipeline = ['!'] command { '|' {'\n'} command } ;
static enum parser_status parse_pipeline(struct ast **res, struct lexer *lexer)
{
    struct ast *main = NULL;
    struct token next = lexer_peek(lexer);
    if (next.type == TOKEN_NOT)
    {
        token_free(lexer_pop(lexer));
        main = ast_new(AST_NOT, NULL);
        if (!main)
            return error_handling(res, NULL, &next, "parse_pipeline MEMORY");
        struct ast *sub = NULL;
        if (helper_pipeline(&sub, lexer) != PARSER_OK)
            return error_handling(res, main, &next, "parse_pipeline");
        ast_append_son(main, sub);
    }
    else
    {
        if (helper_pipeline(&main, lexer) != PARSER_OK)
            return error_handling(res, NULL, &next, "parse_pipeline");
    }

    *res = main;
    token_free(next);
    return PARSER_OK;
}

// shell_command {redirection}
static enum parser_status
helper_parse_command_shell_command(struct ast **res, struct lexer *lexer)
{
    struct ast *shell_command = NULL;
    if (parse_shell_command(&shell_command, lexer) != PARSER_OK)
        return error_handling(res, NULL, NULL,
                              "helper_parse_command_shell_command");
    struct ast *main = shell_command;

    // if a redirection follows the shell_command, we build the folder
    struct ast *redir_folder = NULL;
    struct token next = lexer_peek(lexer);
    while (could_be_redir(next))
    {
        // if we haven't built the folder yet
        if (redir_folder == NULL)
        {
            if ((redir_folder = ast_new(AST_REDIR_FOLDER, NULL)) == NULL)
                return error_handling(
                    res, shell_command, NULL,
                    "helper_parse_command_shell_command MEMORY");
            ast_append_son(redir_folder, shell_command); // gets command
            main = redir_folder; // replaces command in res (final ast)
        }
        // add the redirection to the folder
        struct ast *redir = NULL;
        if (parse_redirection(&redir, lexer) != PARSER_OK)
            return error_handling(res, redir_folder, NULL,
                                  "helper_parse_command_shell_command");
        ast_append_son(redir_folder, redir);

        token_free(next);
        next = lexer_peek(lexer);
    }

    *res = main;
    token_free(next);
    return PARSER_OK;
}

// funcdec {redirection}
static enum parser_status helper_parse_command_funcdec(struct ast **res,
                                                       struct lexer *lexer)
{
    struct ast *funcdec = NULL;
    if (parse_funcdec(&funcdec, lexer) != PARSER_OK)
        return error_handling(res, NULL, NULL, "helper_parse_command_funcdec");

    // if a redirection follows the funcdec, it is valid whenever func is called
    struct ast *redir_folder = NULL;
    struct token next = lexer_peek(lexer);
    while (could_be_redir(next))
    {
        // if we haven't built the folder yet
        if (redir_folder == NULL)
        {
            if ((redir_folder = ast_new(AST_REDIR_FOLDER, NULL)) == NULL)
                return error_handling(res, funcdec, NULL,
                                      "helper_parse_command_funcdec MEMORY");
            ast_append_son(redir_folder, ast_get_son(funcdec, 0)); // gets body
            funcdec->left_son = redir_folder; // swaps son (nb_sons stays same)
        }
        // add the redirection to the folder
        struct ast *redir = NULL;
        if (parse_redirection(&redir, lexer) != PARSER_OK)
            return error_handling(res, funcdec, NULL,
                                  "helper_parse_command_funcdec");
        ast_append_son(redir_folder, redir);

        token_free(next);
        next = lexer_peek(lexer);
    }

    *res = funcdec;
    token_free(next);
    return PARSER_OK;
}

static int starts_shell_command(struct token token)
{
    enum token_type type = token.type;
    return type == TOKEN_LBRACKET || type == TOKEN_LPAREN || type == TOKEN_FOR
        || type == TOKEN_WHILE || type == TOKEN_UNTIL || type == TOKEN_IF;
}

// command =
// simple_command
// | shell_command { redirection }
// | funcdec { redirection }
// ;
static enum parser_status parse_command(struct ast **res, struct lexer *lexer)
{
    struct token next = lexer_peek(lexer);
    if (next.type == TOKEN_FUNCTION_WORD)
    {
        token_free(next);
        return helper_parse_command_funcdec(res, lexer);
    }
    else if (starts_shell_command(next))
    {
        token_free(next);
        return helper_parse_command_shell_command(res, lexer);
    }
    else
    {
        token_free(next);
        return parse_simple_command(res, lexer);
    }
}

// shell_command =
// '{' compound_list '}'
// | '(' compound_list ')'
// | rule_for
// | rule_while
// | rule_until
// | rule_if
// ;
static enum parser_status parse_shell_command(struct ast **res,
                                              struct lexer *lexer)
{
    struct token next = lexer_peek(lexer);
    if (next.type == TOKEN_LBRACKET)
    {
        token_free(next);
        token_free(lexer_pop(lexer));
        if (parse_compound_list(res, lexer) != PARSER_OK)
            return error_handling(res, NULL, NULL, "parse_command");
        next = lexer_peek(lexer);
        if (next.type != TOKEN_RBRACKET)
            return error_handling(res, NULL, &next,
                                  "parse_command expected RBRACKET");
        token_free(next);
        token_free(lexer_pop(lexer));
        return PARSER_OK;
    }
    else if (next.type == TOKEN_LPAREN)
    {
        token_free(next);
        return parse_subshell(res, lexer);
    }
    else if (next.type == TOKEN_IF)
    {
        token_free(next);
        return parse_rule_if(res, lexer);
    }
    else if (next.type == TOKEN_WHILE || next.type == TOKEN_UNTIL)
    {
        token_free(next);
        return parse_while_until(res, lexer);
    }
    else if (next.type == TOKEN_FOR)
    {
        token_free(next);
        return parse_for(res, lexer);
    }
    else
        return error_handling(res, NULL, &next,
                              "parse_command expected something");
}

static enum parser_status parse_assignment(struct ast **res,
                                           struct lexer *lexer)
{
    struct token next = lexer_peek(lexer);
    //= Get the variable name
    char *name;
    if (next.first == NULL)
        name = get_var_name(next.value);
    else
    {
        name = get_var_name(next.first->value);
        next.first->value =
            remove_and_free_first_x_chars(next.first->value, strlen(name) + 1);
    }
    struct ast *main = ast_new(AST_VARIABLE, name);

    //= Get the variable value
    struct ast *sub = NULL;
    if (next.first == NULL)
        sub = ast_new(AST_ARGUMENT, get_var_value(next.value));
    else
        sub = handle_expandable_token(next);

    if (!main || !sub)
        return error_handling(
            res, NULL, &next,
            "parse_simple_command (variable assignment) MEMORY");

    ast_append_son(main, sub);

    token_free(lexer_pop(lexer)); // Consume the assignment word

    *res = main;
    token_free(next);
    return PARSER_OK;
}

// simple_command =
// prefix { prefix }
// | { prefix } WORD { element }
// ;
// OUR_SIMPLE_COMMAND =
// { assignment | redirection } [ WORD { WORD | redirection } ]
// ;
static enum parser_status parse_simple_command(struct ast **res,
                                               struct lexer *lexer)
{ // WARNING MAGIE NOIRE
    struct token next = lexer_peek(lexer);
    if (!could_be_word(next) && !could_be_redir(next))
        return error_handling(
            res, NULL, &next,
            "parse_simple_command expected WORD, ASSIGMENT_WORD or REDIR");

    struct ast *command_list = ast_new(AST_COMMAND_LIST, NULL);
    if (!command_list)
        return error_handling(res, NULL, &next, "parse_simple_command MEMORY");
    struct ast *redir_folder = NULL;
    while (could_be_redir(next) || next.type == TOKEN_ASSIGNMENT_WORD)
    { // { assignment | redirection }
        if (next.type == TOKEN_ASSIGNMENT_WORD)
        {
            struct ast *sub = NULL;
            if (parse_assignment(&sub, lexer) != PARSER_OK)
                return error_handling(
                    res, redir_folder == NULL ? command_list : redir_folder,
                    NULL, "helper_parse_command_funcdec MEMORY");
            ast_append_son(command_list, sub);
        }
        else // redirection
        {
            // if we haven't built the folder yet
            if (redir_folder == NULL)
            {
                if ((redir_folder = ast_new(AST_REDIR_FOLDER, NULL)) == NULL)
                    return error_handling(
                        res, command_list, NULL,
                        "helper_parse_command_funcdec MEMORY");
                ast_append_son(redir_folder, command_list); // gets command list
            }
            // add the redirection to the folder
            struct ast *redir = NULL;
            if (parse_redirection(&redir, lexer) != PARSER_OK)
                return error_handling(res, redir_folder, NULL,
                                      "helper_parse_command_funcdec");
            ast_append_son(redir_folder, redir);
        }

        token_free(next);
        next = lexer_peek(lexer);
    }

    token_free(next);
    return parse_simple_command2(res, lexer, command_list, redir_folder);
}

static enum parser_status parse_simple_command2(struct ast **res,
                                                struct lexer *lexer,
                                                struct ast *command_list,
                                                struct ast *redir_folder)
{
    struct token next = lexer_peek(lexer);
    if (!could_be_word(next))
    { // if there is no command, return
        *res = redir_folder == NULL ? command_list : redir_folder;
        token_free(next);
        return PARSER_OK;
    }
    struct ast *main = ast_new(AST_COMMAND, lexer_pop(lexer).value);
    if (!main)
        return error_handling(res, NULL, NULL, "parse_simple_command MEMORY");
    ast_append_son(command_list, main);

    token_free(next);
    next = lexer_peek(lexer);
    while (could_be_word(next) || is_redirection_token(next))
    {
        if (could_be_word(next))
        {
            struct ast *sub = NULL;
            if (parse_element(&sub, lexer) == PARSER_UNEXPECTED_TOKEN)
                return error_handling(
                    res, redir_folder == NULL ? command_list : redir_folder,
                    &next, "parse_simple_command");
            ast_append_son(main, sub);
        }
        else // redirection
        {
            // if we haven't built the folder yet
            if (redir_folder == NULL)
            {
                if ((redir_folder = ast_new(AST_REDIR_FOLDER, NULL)) == NULL)
                    return error_handling(
                        res, command_list, NULL,
                        "helper_parse_command_funcdec MEMORY");
                ast_append_son(redir_folder, command_list); // gets command list
            }
            // add the redirection to the folder
            struct ast *redir = NULL;
            if (parse_redirection(&redir, lexer) != PARSER_OK)
                return error_handling(res, redir_folder, NULL,
                                      "helper_parse_command_funcdec");
            ast_append_son(redir_folder, redir);
        }

        token_free(next);
        next = lexer_peek(lexer);
    }

    *res = redir_folder == NULL ? command_list : redir_folder;
    token_free(next);
    return PARSER_OK;
}

// element =
// WORD
// | redirection
// ;
// OUR_ELEMENT = WORD | EXPANDABLE ;
static enum parser_status parse_element(struct ast **res, struct lexer *lexer)
{
    struct token next = lexer_peek(lexer);
    if (!could_be_word(next))
        return error_handling(res, NULL, &next, "parse_element expected WORD");

    struct ast *main;

    //? Handle expandable tokens
    if (next.type == TOKEN_EXPANDABLE)
    {
        struct token tok = lexer_pop(lexer);
        main = handle_expandable_token(tok);
        if (!main)
            return error_handling(res, NULL, &next, "parse_element MEMORY");

        token_free(tok);
    }
    else
    {
        main = ast_new(AST_ARGUMENT, lexer_pop(lexer).value);
        if (!main)
            return error_handling(res, NULL, &next, "parse_element MEMORY");
    }

    *res = main;
    token_free(next);
    return PARSER_OK;
}

static int is_redirection_token(struct token token)
{
    switch (token.type)
    {
    case TOKEN_REDIR_OUT:
    case TOKEN_REDIR_IN:
    case TOKEN_REDIR_APP_OUT:
    case TOKEN_REDIR_DUP_OUT:
    case TOKEN_REDIR_DUP_IN:
    case TOKEN_REDIR_RW:
    case TOKEN_IONUMBER:
        return 1; // True, this is a redirection token
    default:
        return 0; // False, this is not a redirection token
    }
}

static char *get_var_name(char *str)
{
    char *name = calloc(1, strlen(str) + 1);
    if (!name)
        return NULL;

    size_t i = 0;
    while (str[i] != '=')
    {
        name[i] = str[i];
        ++i;
    }
    name[i] = '\0';
    return name;
}

static char *remove_and_free_first_x_chars(char *str, int x)
{
    char *new_str = strdup(str + x);
    free(str);
    return new_str;
}

static char *get_var_value(char *str)
{
    char *value = calloc(1, strlen(str) + 1);
    if (!value)
        return NULL;

    //? Skip the variable name
    size_t i = 0;
    while (str[i] != '=')
        ++i;
    ++i;
    //? Copy the variable value
    size_t u = 0;
    while (str[i] != '\0')
        value[u++] = str[i++];
    value[u] = '\0';
    return value;
}

static enum parser_status parse_for_first(struct ast **res, struct lexer *lexer,
                                          char **var_name, struct ast **list)
{
    struct token next = lexer_peek(lexer);
    if (next.type != TOKEN_FOR)
    {
        return error_handling(res, NULL, &next, "parse_for expected 'for'");
    }
    token_free(lexer_pop(lexer)); // Consume 'for'
    token_free(next);
    next = lexer_peek(lexer);
    if (!could_be_word(next))
    {
        return error_handling(res, NULL, &next,
                              "parse_for expected variable name");
    }
    *var_name = next.value; // Consume variable name
    token_free(lexer_pop(lexer));

    *list = ast_new(AST_COMMAND_LIST, NULL);
    if (!*list)
    {
        free(*var_name);
        return error_handling(res, NULL, &next,
                              "parse_for memory allocation failed for list");
    }

    next = lexer_peek(lexer);
    if (next.type == TOKEN_IN)
    {
        token_free(lexer_pop(lexer)); // Consume 'in'
        token_free(next);
        next = lexer_peek(lexer);
        while (could_be_word(next))
        {
            struct ast *item = ast_new(AST_ARGUMENT, lexer_pop(lexer).value);
            if (!item)
            {
                free(*var_name);
                return error_handling(res, *list, &next,
                                      "parse_for memory allocation failed");
            }
            ast_append_son(*list, item);
            token_free(next);
            next = lexer_peek(lexer);
        }
    }
    token_free(next);
    return PARSER_OK;
}

static enum parser_status handle_for_error(char *var_name, struct ast *list,
                                           struct ast *compound_list,
                                           struct token *next)
{
    if (var_name)
    {
        free(var_name);
    }
    return error_handling(&compound_list, list, next, "parse_for error");
}

// rule_for = 'for' WORD ( [';'] | [ {'\n'} 'in' { WORD } ( ';' | '\n' ) ] )
// {'\n'} 'do' compound_list 'done' ;
static enum parser_status parse_for(struct ast **res, struct lexer *lexer)
{
    char *var_name = NULL;
    struct ast *list = NULL;
    struct ast *compound_list = NULL;

    enum parser_status status = parse_for_first(res, lexer, &var_name, &list);
    if (status != PARSER_OK)
        return status;

    struct token next = lexer_peek(lexer);

    // Skip new lines before 'do'
    while (next.type == TOKEN_LF || next.type == TOKEN_SEMI_COL)
    {
        token_free(lexer_pop(lexer));
        token_free(next);
        next = lexer_peek(lexer);
    }

    if (next.type != TOKEN_DO)
        return handle_for_error(var_name, list, NULL, &next);

    token_free(lexer_pop(lexer)); // Consume 'do'
    token_free(next);

    // Skip new lines before processing compound list
    next = lexer_peek(lexer);
    while (next.type == TOKEN_LF || next.type == TOKEN_SEMI_COL)
    {
        token_free(lexer_pop(lexer));
        token_free(next);
        next = lexer_peek(lexer);
    }

    if (parse_compound_list(&compound_list, lexer) != PARSER_OK)
    {
        return handle_for_error(var_name, list, NULL, &next);
    }

    token_free(next);
    next = lexer_peek(lexer);
    if (next.type != TOKEN_DONE)
    {
        return handle_for_error(var_name, list, compound_list, &next);
    }

    token_free(lexer_pop(lexer)); // Consume 'done'

    struct ast *for_node = ast_new(AST_FOR, var_name);
    if (!for_node)
        return handle_for_error(var_name, list, compound_list, NULL);

    ast_append_son(for_node, list);
    ast_append_son(for_node, compound_list);

    *res = for_node;
    var_name = NULL;
    list = NULL;
    compound_list = NULL;
    token_free(next);
    return PARSER_OK;
}

static enum ast_type get_ast_type_from_token(enum token_type t_type)
{
    switch (t_type)
    {
    case TOKEN_REDIR_IN:
        return AST_REDIR_IN;
    case TOKEN_REDIR_OUT:
        return AST_REDIR_OUT;
    case TOKEN_REDIR_APP_OUT:
        return AST_REDIR_APP_OUT;
    case TOKEN_REDIR_DUP_IN:
        return AST_REDIR_DUP_IN;
    case TOKEN_REDIR_DUP_OUT:
        return AST_REDIR_DUP_OUT;
    case TOKEN_REDIR_RW:
        return AST_REDIR_RW;
    default:
        return AST_INVALID; // invalid type
    }
}

static struct ast *handle_expandable_token(struct token token)
{
    struct ast *main = ast_new(AST_EXPANSION, NULL);
    if (!main)
        return NULL;
    struct expansion *head = token.first;
    while (head)
    {
        // Copy the value
        char *val = calloc(1, strlen(head->value) + 1);
        val = strcpy(val, head->value);
        val[strlen(head->value)] = '\0';

        // Create the ast node
        struct ast *sub = ast_new(
            head->type == NORMAL ? AST_EXPARG_NORM : AST_EXPARG_DQ, val);
        if (!sub)
        {
            fprintf(stderr, "handle_expandable_token MEMORY\n");
            ast_free(main);
            return NULL;
        }
        ast_append_son(main, sub);
        head = head->next;
    }
    return main;
}

static enum parser_status helper_parse_if(struct ast **res, struct lexer *lexer)
{
    struct ast *main = ast_new(AST_CONDITIONAL, NULL);
    if (!main)
        return error_handling(res, NULL, NULL, "helper_parse_if MEMORY");

    // compound_list
    struct ast *sub = NULL;
    if (parse_compound_list(&sub, lexer) == PARSER_UNEXPECTED_TOKEN)
        return error_handling(res, main, NULL, "helper_parse_if");
    ast_append_son(main, sub);

    // then
    struct token next = lexer_peek(lexer);
    if (next.type != TOKEN_THEN)
        return error_handling(res, main, &next,
                              "helper_parse_if expected THEN");
    else
    {
        token_free(next);
        next = lexer_pop(lexer);
    }

    // compound_list
    sub = NULL;
    if (parse_compound_list(&sub, lexer) == PARSER_UNEXPECTED_TOKEN)
        return error_handling(res, main, &next, "helper_parse_if");
    ast_append_son(main, sub);

    // optional else_clause
    token_free(next);
    next = lexer_peek(lexer);
    if (next.type == TOKEN_ELSE || next.type == TOKEN_ELIF)
    {
        sub = NULL;
        if (parse_else_clause(&sub, lexer) == PARSER_UNEXPECTED_TOKEN)
            return error_handling(res, main, &next, "helper_parse_if");
        ast_append_son(main, sub);
    }

    *res = main;
    token_free(next);
    return PARSER_OK;
}

// rule_if = 'if' compound_list 'then' compound_list [ else_clause ] 'fi' ;
static enum parser_status parse_rule_if(struct ast **res, struct lexer *lexer)
{
    // if
    struct token next = lexer_peek(lexer);
    if (next.type != TOKEN_IF)
        return error_handling(res, NULL, &next, "parse_rule_if expected IF");
    token_free(lexer_pop(lexer));

    // inside
    if (helper_parse_if(res, lexer) == PARSER_UNEXPECTED_TOKEN)
        return error_handling(res, NULL, &next, "parse_rule_if");

    // fi
    token_free(next);
    next = lexer_peek(lexer);
    if (next.type != TOKEN_FI)
        return error_handling(res, NULL, &next, "parse_rule_if expected FI");
    else
        token_free(lexer_pop(lexer));

    token_free(next);
    return PARSER_OK;
}

// else_clause =
// 'else' compound_list
// | 'elif' compound_list 'then' compound_list [ else_clause ]
// ;
static enum parser_status parse_else_clause(struct ast **res,
                                            struct lexer *lexer)
{
    struct token next = lexer_peek(lexer);
    if (next.type == TOKEN_ELSE)
    {
        token_free(next);
        token_free(lexer_pop(lexer));
        return parse_compound_list(res, lexer);
    }
    else if (next.type == TOKEN_ELIF)
    {
        token_free(next);
        token_free(lexer_pop(lexer));
        return helper_parse_if(res, lexer);
    }
    else
    {
        token_free(next);
        return PARSER_UNEXPECTED_TOKEN;
    }
}

static int ends_compound_list(struct token token)
{
    enum token_type type = token.type;
    return type == TOKEN_THEN || type == TOKEN_ELIF || type == TOKEN_ELSE
        || type == TOKEN_FI || type == TOKEN_DO || type == TOKEN_DONE
        || type == TOKEN_RPAREN || type == TOKEN_RBRACKET;
}

// compound_list = {'\n'} and_or { ( ';' | '&' | '\n' ) {'\n'} and_or } [ ';' |
// '&' ] {'\n'} ;
static enum parser_status parse_compound_list(struct ast **res,
                                              struct lexer *lexer)
{
    struct token next = discard_token_type_all(lexer, TOKEN_LF);
    if (!could_be_word(next))
        return error_handling(res, NULL, &next,
                              "parse_compound_list expected WORD");

    // create new ast node -> list of command
    struct ast *main = ast_new(AST_COMMAND_LIST, NULL);
    if (!main)
        return error_handling(res, NULL, &next, "parse_compound_list MEMORY");

    // get first (and mandatory) and_or rule
    struct ast *sub = NULL;
    if (parse_and_or(&sub, lexer) != PARSER_OK)
        return error_handling(res, main, &next, "parse_compound_list");
    ast_append_son(main, sub);

    token_free(next);
    next = lexer_peek(lexer);
    // as long as there MIGHT be other and_or rules
    while (next.type == TOKEN_SEMI_COL || next.type == TOKEN_LF)
    {
        // handle starting or ending ( ';' | '\n' ) { '\n' }
        token_free(lexer_pop(lexer));
        token_free(next);
        next = discard_token_type_all(lexer, TOKEN_LF);

        // check if the compound list actually ends here LMAO
        if (ends_compound_list(next))
            break;

        if (!could_be_word(next))
            return error_handling(res, main, &next,
                                  "parse_compound_list expected WORD");
        // parse individual command
        sub = NULL;
        if (parse_and_or(&sub, lexer) != PARSER_OK)
            return error_handling(res, main, &next, "parse_compound_list");

        // append the parsed command sub as a child of the command list
        ast_append_son(main, sub);

        // check if the next token is ';'
        token_free(next);
        next = lexer_peek(lexer);
    }

    // set result to the constructed list
    *res = main;
    // free the last token peeked
    token_free(next);
    return PARSER_OK;
}

// rule_while = 'while' compound_list 'do' compound_list 'done' ;
// rule_until = 'until' compound_list 'do' compound_list 'done' ;
static enum parser_status parse_while_until(struct ast **res,
                                            struct lexer *lexer)
{
    struct token next = lexer_peek(lexer);
    if (next.type != TOKEN_WHILE && next.type != TOKEN_UNTIL)
        return error_handling(res, NULL, &next,
                              "parse_while_until expected WHILE or UNTIL");

    // build the ast containing the while rule
    struct ast *main =
        ast_new(next.type == TOKEN_WHILE ? AST_WHILE : AST_UNTIL, NULL);
    if (!main)
        return error_handling(res, NULL, &next, "parse_while_until MEMORY");
    token_free(lexer_pop(lexer));

    // condition compound list
    struct ast *sub = NULL;
    if (parse_compound_list(&sub, lexer) != PARSER_OK)
        return error_handling(res, main, &next, "parse_while_until");
    ast_append_son(main, sub);

    // 'do'
    token_free(next);
    next = lexer_peek(lexer);
    if (next.type != TOKEN_DO)
        return error_handling(res, main, &next,
                              "parse_while_until expected DO");
    token_free(lexer_pop(lexer));

    // loop compound list
    sub = NULL;
    if (parse_compound_list(&sub, lexer) != PARSER_OK)
        return error_handling(res, main, &next, "parse_while_until");
    ast_append_son(main, sub);

    // 'done'
    token_free(next);
    next = lexer_peek(lexer);
    if (next.type != TOKEN_DONE)
        return error_handling(res, main, &next,
                              "parse_while_until expected DONE");
    token_free(lexer_pop(lexer));

    token_free(next);
    *res = main;
    return PARSER_OK;
}

size_t default_ionumber(enum token_type type)
{
    switch (type)
    {
    case TOKEN_REDIR_IN:
    case TOKEN_REDIR_DUP_IN:
        return 0;
    case TOKEN_REDIR_OUT:
    case TOKEN_REDIR_APP_OUT:
    case TOKEN_REDIR_DUP_OUT:
    case TOKEN_REDIR_RW:
        return 1;
    default:
        return 1;
    }
}

// redirection =
// [IONUMBER] ( '>' | '<' | '>>' | '>&' | '<&' | '>|' | '<>' ) WORD
static enum parser_status parse_redirection(struct ast **res,
                                            struct lexer *lexer)
{
    struct token redir_tok = lexer_pop(lexer); // Consume the redirection token
    if (!could_be_redir(redir_tok))
    {
        // If the token is not a redirection token, this is unexpected
        return error_handling(res, NULL, &redir_tok,
                              "Expected redirection token");
    }

    struct token ionumber_tok = { .type = TOKEN_ERROR };
    if (redir_tok.type == TOKEN_IONUMBER)
    {
        ionumber_tok = redir_tok;
        redir_tok = lexer_pop(lexer); // Again
        if (!could_be_redir(redir_tok))
        {
            // If the token is not a redirection token, this is unexpected
            return error_handling(res, NULL, &redir_tok,
                                  "Expected redirection token");
        }
    }

    struct token file_tok = lexer_pop(lexer); // Consume the filename token
    if (!could_be_word(file_tok))
    {
        // If the filename is not a word, this is unexpected
        token_free(redir_tok);
        return error_handling(res, NULL, &file_tok,
                              "Expected filename for redirection");
    }

    // Create the AST node for the redirection with the filename
    struct ast *main =
        ast_new(get_ast_type_from_token(redir_tok.type), file_tok.value);
    if (main == NULL)
    {
        // Handle memory allocation failure
        token_free(file_tok);
        return error_handling(res, NULL, &redir_tok,
                              "Memory allocation failed for redirection node");
    }

    // If we have a specified IO number, we use it
    if (ionumber_tok.type != TOKEN_ERROR)
    {
        main->nb_sons = atoi(ionumber_tok.value);
        token_free(ionumber_tok);
    }
    // Otherwise, we get a default value
    else
        main->nb_sons = default_ionumber(redir_tok.type);

    *res = main;
    token_free(redir_tok);
    return PARSER_OK;
}

static enum parser_status parse_subshell(struct ast **res, struct lexer *lexer)
{
    struct token next = lexer_pop(lexer);
    if (next.type != TOKEN_LPAREN)
    {
        return error_handling(res, NULL, &next,
                              "Expected '(' for subshell start.");
    }
    struct ast *compound_list = NULL;
    enum parser_status status = parse_compound_list(
        &compound_list, lexer); // Analyse les commandes internes
    if (status != PARSER_OK)
    {
        token_free(next);
        return status;
    }
    token_free(next);
    next = lexer_peek(lexer);
    if (next.type != TOKEN_RPAREN)
    {
        ast_free(compound_list);
        return error_handling(res, NULL, &next,
                              "Expected ')' for subshell end.");
    }
    *res = ast_new(AST_SUBSHELL, NULL);
    if (*res == NULL)
    {
        ast_free(compound_list);
        return error_handling(res, NULL, &next,
                              "Memory allocation failed for subshell.");
    }
    ast_append_son(*res, compound_list);
    token_free(lexer_pop(lexer));
    token_free(next);
    return PARSER_OK;
}

static enum parser_status parse_funcdec(struct ast **res, struct lexer *lexer)
{
    struct token next = lexer_peek(lexer);
    if (next.type != TOKEN_FUNCTION_WORD)
        return error_handling(res, NULL, &next,
                              "parse_funcdec expected FUNCTION_WORD");
    struct ast *funcdec = ast_new(AST_FUNCDEC, lexer_pop(lexer).value);

    // trim interfering newlines
    token_free(next);
    next = lexer_peek(lexer);
    while (next.type == TOKEN_LF)
    {
        token_free(next);
        token_free(lexer_pop(lexer));
        next = lexer_peek(lexer);
    }

    struct ast *inside = NULL;
    if (parse_command(&inside, lexer) != PARSER_OK)
        return error_handling(res, funcdec, &next, "parse_funcdec");
    ast_append_son(funcdec, inside);

    *res = funcdec;
    token_free(next);
    return PARSER_OK;
}
