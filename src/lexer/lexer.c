#include "lexer.h"

#include <stdio.h>
#include <stdlib.h>

#include "IO_Backend/io.h"

struct arg_end_of_lex
{
    unsigned *word_size;
    char c;
};

struct lexer *lexer_new(struct IO *input)
{
    struct lexer *lexer = calloc(1, sizeof(struct lexer));
    if (lexer == NULL)
    {
        fprintf(stderr, "lexer_new: calloc failed\n");
        return NULL;
    }

    lexer->input = input;
    lexer->current_tok.type = TOKEN_ERROR;
    lexer->current_tok.value = NULL;
    lexer->state = LEXER_NORMAL;
    lexer->exp_state = EXP_NONE;

    return lexer;
}

void lexer_free(struct lexer *lexer)
{
    if (lexer)
    {
        if (lexer->input)
            IO_free(lexer->input);

        free(lexer);
    }
}

static enum token_type get_char_type(char c)
{
    switch (c)
    {
    case ';':
        return TOKEN_SEMI_COL;
    case '\n':
        return TOKEN_LF;
    case '|':
        return TOKEN_PIPE;
    case '!':
        return TOKEN_NOT;
    case '$':
        return TOKEN_EXPANDABLE;
    case '"':
        return TOKEN_EXPANDABLE;
    case '(':
        return TOKEN_LPAREN;
    case ')':
        return TOKEN_RPAREN;
    case '{':
        return TOKEN_LBRACKET;
    case '}':
        return TOKEN_RBRACKET;
    default:
        return TOKEN_ERROR;
    }
}

static enum token_type check_for_reserved_word(char *word)
{
    if (strcmp(word, "if") == 0)
        return TOKEN_IF;
    else if (strcmp(word, "then") == 0)
        return TOKEN_THEN;
    else if (strcmp(word, "elif") == 0)
        return TOKEN_ELIF;
    else if (strcmp(word, "else") == 0)
        return TOKEN_ELSE;
    else if (strcmp(word, "fi") == 0)
        return TOKEN_FI;
    else if (strcmp(word, "for") == 0)
        return TOKEN_FOR;
    else if (strcmp(word, "in") == 0)
        return TOKEN_IN;
    else if (strcmp(word, "while") == 0)
        return TOKEN_WHILE;
    else if (strcmp(word, "until") == 0)
        return TOKEN_UNTIL;
    else if (strcmp(word, "do") == 0)
        return TOKEN_DO;
    else if (strcmp(word, "done") == 0)
        return TOKEN_DONE;
    else
        return TOKEN_WORD;
}

static int is_number(char c)
{
    return c >= '0' && c <= '9';
}

static int is_letter(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

/*
** Assignement word is a word that contains a variable name and a value
** separated by an equal sign.
** variable name : [a-zA-Z_][a-zA-Z0-9_]*
** Example: var=value
*/
int is_assignment_word_str(char *word)
{
    int i = 0;
    if (word[i] == '=' || !(is_letter(word[i]) || word[i] == '_'))
        return 0;

    while (word[i] != '\0')
    {
        if (word[i] == '=')
            return 1;
        if (!(is_letter(word[i]) || is_number(word[i]) || word[i] == '_'))
            return 0;
        ++i;
    }

    return 0;
}

int is_assignment_word(struct token tok)
{
    if (tok.type != TOKEN_EXPANDABLE)
        return is_assignment_word_str(tok.value);

    struct expansion *head = tok.first;
    if (head->type != NORMAL)
        return 0;

    return is_assignment_word_str(head->value);
}

char *add_char(char *word, char c, int *index, unsigned *word_size)
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

static void get_quoted_string(struct lexer *lexer, char **word, int *i,
                              unsigned *word_size)
{
    char c = get_char(lexer->input);
    while (c != '\'')
    {
        if (c == EOF)
        {
            fprintf(stderr, "IO_read_word: EOF reached before closing quote\n");
            free(*word);
            lexer->state = LEXER_ERROR;
            return;
        }

        *word = add_char(*word, c, i, word_size);
        c = get_char(lexer->input);
    }
}

static char check_for_fun(struct lexer *lexer, struct token *tok, int i,
                          unsigned word_size)
{
    fseek(lexer->input->input, -1, SEEK_CUR);
    char c = get_char(lexer->input);

    while (c == ' ' || c == '\t')
        c = get_char(lexer->input);

    if (c == '(')
    {
        c = get_char(lexer->input);

        while (c == ' ' || c == '\t')
            c = get_char(lexer->input);

        if (c == ')')
        {
            tok->type = TOKEN_FUNCTION_WORD;
            tok->value = add_char(tok->value, '\0', &i, &word_size);
        }
        else
            fseek(lexer->input->input, -1, SEEK_CUR);
    }
    else
        fseek(lexer->input->input, -1, SEEK_CUR);

    return c;
}

static char is_io_number(struct lexer *lexer, struct token *tok, int *i,
                         unsigned wordsize)
{
    fseek(lexer->input->input, -1, SEEK_CUR);
    char c = get_char(lexer->input);

    while (c >= '0' && c <= '9')
    {
        tok->value = add_char(tok->value, c, i, &wordsize);
        c = get_char(lexer->input);
    }

    if (c == '>' || c == '<')
    {
        tok->type = TOKEN_IONUMBER;
        tok->value = add_char(tok->value, '\0', i, &wordsize);
    }
    else
        tok->type = TOKEN_WORD;

    return c;
}

static char handle_dq_var_utils(struct lexer *lexer, struct token *tok, int *j,
                                unsigned exp_size)
{
    fseek(lexer->input->input, -1, SEEK_CUR);
    char c = get_char(lexer->input);
    if (lexer->state == LEXER_DQUOTE)
    {
        while (c != EOF && c != '\n' && c != '"')
        {
            if (c == '\\')
            {
                c = get_char(lexer->input);
                if (c == '"' || c == '`' || c == '\\')
                {
                    tok->expansion->value =
                        add_char(tok->expansion->value, c, j, &exp_size);
                }
                else
                {
                    tok->expansion->value =
                        add_char(tok->expansion->value, '\\', j, &exp_size);
                    tok->expansion->value =
                        add_char(tok->expansion->value, c, j, &exp_size);
                }
            }
            else
            {
                tok->expansion->value =
                    add_char(tok->expansion->value, c, j, &exp_size);
            }
            c = get_char(lexer->input);
        }
    }
    else
    {
        while (c != EOF && c != '\n' && c != ' ' && c != ';' && c != '|'
               && c != '!' && c != '$' && c != '"')
        {
            tok->expansion->value =
                add_char(tok->expansion->value, c, j, &exp_size);
            c = get_char(lexer->input);
        }
    }

    return c;
}

static char handle_dq_variable(struct lexer *lexer, struct token *tok, int j,
                               unsigned exp_size)
{
    fseek(lexer->input->input, -1, SEEK_CUR);
    char c = get_char(lexer->input);
    c = handle_dq_var_utils(lexer, tok, &j, exp_size);

    if (c == '"' && lexer->state == LEXER_DQUOTE)
        c = get_char(lexer->input);

    tok->expansion->value =
        add_char(tok->expansion->value, '\0', &j, &exp_size);
    j = 0;

    if (c == '$' || c == '"')
    {
        if (c == '"')
        {
            if (lexer->state != LEXER_DQUOTE)
                lexer->state = LEXER_DQUOTE;
            else
                lexer->state = LEXER_NORMAL;
        }

        unsigned exp_size = BASE_VALUE_SIZE;

        struct expansion *next = calloc(1, sizeof(struct expansion));
        next->value = calloc(BASE_VALUE_SIZE, sizeof(char));
        next->type = DOUBLE_QUOTE;

        if (lexer->state == LEXER_NORMAL && c != '$')
            next->type = NORMAL;

        tok->expansion->next = next;
        tok->expansion = next;
        int j = 0;

        if (c != '"')
            tok->expansion->value =
                add_char(tok->expansion->value, c, &j, &exp_size);

        char b = get_char(lexer->input);

        if (c == '"' && b != '"' && b != '$')
            tok->expansion->type = NORMAL;

        c = b;

        if (c == '"')
        {
            if (lexer->state != LEXER_DQUOTE)
                lexer->state = LEXER_DQUOTE;
            else
                lexer->state = LEXER_NORMAL;
        }

        c = handle_dq_variable(lexer, tok, j, exp_size);
    }

    return c;
}

static struct expansion *get_through_expansions(struct expansion *expansion)
{
    if (expansion == NULL)
        return NULL;

    struct expansion *current = expansion;

    while (current->next != NULL)
        current = current->next;

    return current;
}

static char handle_redirection(struct lexer *lexer, struct token *tok, int *i,
                               unsigned word_size)
{
    fseek(lexer->input->input, -1, SEEK_CUR);
    char c = get_char(lexer->input);

    char next_c = peek_char(lexer->input);
    if (c == '>' && next_c == '>')
    {
        // Handle '>>' for append redirection
        get_char(lexer->input); // Consume the second '>'
        tok->type = TOKEN_REDIR_APP_OUT;
        tok->value = add_char(tok->value, '>', i, &word_size);
    }
    else if (c == '>' && next_c == '|')
    {
        // Handle '>|' for redirection with |
        get_char(lexer->input); // Consume the '|'
        tok->type = TOKEN_REDIR_OUT;
        tok->value = add_char(tok->value, '|', i, &word_size);
    }
    else if (c == '<' && next_c == '&')
    {
        // Handle '<&' for duplicating input file descriptor
        get_char(lexer->input); // Consume the '&'
        tok->type = TOKEN_REDIR_DUP_IN;
        tok->value = add_char(tok->value, '&', i, &word_size);
    }
    else if (c == '>' && next_c == '&')
    {
        // Handle '>&' for duplicating output file descriptor
        get_char(lexer->input); // Consume the '&'
        tok->type = TOKEN_REDIR_DUP_OUT;
        tok->value = add_char(tok->value, '&', i, &word_size);
    }
    else if (c == '<' && next_c == '>')
    {
        // Handle '<>' for read/write redirection
        get_char(lexer->input); // Consume the '>'
        tok->type = TOKEN_REDIR_RW;
        tok->value = add_char(tok->value, '>', i, &word_size);
    }
    else
    {
        tok->type = (c == '>')
            ? TOKEN_REDIR_OUT
            : TOKEN_REDIR_IN; //$ Single character redirection
    }

    return c;
}

static char handle_stopping_char(struct lexer *lexer, struct token *tok, int *i,
                                 unsigned *word_size)
{
    fseek(lexer->input->input, -1, SEEK_CUR);
    char c = get_char(lexer->input);

    if (*i == 0)
    {
        tok->value = add_char(tok->value, c, i, word_size);
        //? Check for 'or'
        if (c == '|' && peek_char(lexer->input) == '|')
        {
            get_char(lexer->input); // Consume the second '|'
            tok->type = TOKEN_OR;
            tok->value = add_char(tok->value, '|', i, word_size);
        }
        //? Check for 'and'
        else if (c == '&' && peek_char(lexer->input) == '&')
        {
            get_char(lexer->input); // Consume the second '&'
            tok->type = TOKEN_AND;
            tok->value = add_char(tok->value, '&', i, word_size);
        }
        else
            tok->type = get_char_type(c);
    }
    else //$ Stop reading word if you encounter a stoping character
    {
        if (c == '(') // Handle function word
        {
            tok->type = TOKEN_FUNCTION_WORD;
            c = get_char(lexer->input);
            while (c == ' ' || c == '\t')
                c = get_char(lexer->input);

            if (c != ')')
                tok->type = TOKEN_ERROR;
            return c;
        }

        fseek(lexer->input->input, -1, SEEK_CUR);
    }

    return c;
}

static char handle_simple_quote(struct lexer *lexer, struct token *tok, int *i,
                                unsigned *word_size)
{
    fseek(lexer->input->input, -1, SEEK_CUR);
    char c = get_char(lexer->input);

    get_quoted_string(lexer, &tok->value, i, word_size);
    if (lexer->state == LEXER_ERROR)
    {
        tok->type = TOKEN_ERROR;
        return c;
    }

    tok->type = TOKEN_WORD;
    lexer->state = LEXER_QUOTE;

    c = get_char(lexer->input);
    return c;
}

static char handle_expandable_state(struct lexer *lexer, struct token *tok,
                                    int *i, unsigned *exp_size)
{
    fseek(lexer->input->input, -1, SEEK_CUR);
    char c = get_char(lexer->input);

    struct expansion *next = calloc(1, sizeof(struct expansion));
    next->value = calloc(BASE_VALUE_SIZE, sizeof(char));
    next->type = NORMAL;

    tok->expansion->next = next;
    tok->expansion = get_through_expansions(tok->first);

    strcpy(tok->expansion->value, tok->value);

    // reset the tok value
    free(tok->value);
    tok->value = calloc(BASE_VALUE_SIZE, sizeof(char));

    tok->expansion->value = add_char(tok->expansion->value, '\0', i, exp_size);
    *i = 0;

    struct expansion *after = calloc(1, sizeof(struct expansion));
    after->value = calloc(*exp_size, sizeof(char));
    after->type = DOUBLE_QUOTE;

    tok->expansion->next = after;
    tok->expansion = after;

    if (c == '"')
        lexer->state = LEXER_DQUOTE;

    return c;
}

static char handle_dq_var_lex(struct lexer *lexer, struct token *tok, int *i)
{
    fseek(lexer->input->input, -1, SEEK_CUR);
    char c = get_char(lexer->input);

    if (c == '"')
    {
        if (lexer->state != LEXER_DQUOTE)
            lexer->state = LEXER_DQUOTE;
        else
            lexer->state = LEXER_NORMAL;
    }

    unsigned exp_size = BASE_VALUE_SIZE;

    if (lexer->exp_state != EXP_DQ_VAR)
    {
        tok->expansion = calloc(1, sizeof(struct expansion));
        tok->expansion->value = calloc(BASE_VALUE_SIZE, sizeof(char));

        tok->expansion->type = DOUBLE_QUOTE;

        if (lexer->state == LEXER_NORMAL && c != '$')
            tok->expansion->type = NORMAL;

        tok->first = tok->expansion; //? Save the first expansion
    }

    if (tok->type != TOKEN_EXPANDABLE && *i != 0)
    {
        strcpy(tok->expansion->value, tok->value);

        // reset the tok value
        free(tok->value);
        tok->value = calloc(BASE_VALUE_SIZE, sizeof(char));

        tok->expansion->type = NORMAL;
        tok->expansion->value =
            add_char(tok->expansion->value, '\0', i, &exp_size);

        struct expansion *next = calloc(1, sizeof(struct expansion));
        next->value = calloc(BASE_VALUE_SIZE, sizeof(char));
        next->type = DOUBLE_QUOTE;

        tok->expansion->next = next;
        tok->expansion = next;
        *i = 0;
    }
    else if (lexer->exp_state == EXP_DQ_VAR)
        c = handle_expandable_state(lexer, tok, i, &exp_size);

    tok->type = TOKEN_EXPANDABLE;

    int j = 0;

    if (c != '"')
        tok->expansion->value =
            add_char(tok->expansion->value, c, &j, &exp_size);

    c = get_char(lexer->input);

    c = handle_dq_variable(lexer, tok, j, exp_size);
    return c;
}

static char handle_end_of_lex(struct lexer *lexer, struct token *tok, int *i,
                              struct arg_end_of_lex aol)
{
    if (aol.c == EOF && strcmp(tok->value, "") == 0 && tok->first == NULL)
    {
        if (lexer->exp_state == EXP_DQ_VAR)
            lexer->exp_state = EXP_NONE;
        tok->type = TOKEN_EOF;
    }

    if ((aol.c == ' ' || aol.c == '\n') && strcmp(tok->value, "") == 0
        && lexer->exp_state == EXP_DQ_VAR)
        lexer->exp_state = EXP_NONE;

    if ((aol.c == ' ' || aol.c == '\t') && tok->type == TOKEN_WORD)
        aol.c = check_for_fun(lexer, tok, *i, *(aol.word_size));

    //? Add the null terminator
    if (tok->type != TOKEN_FUNCTION_WORD)
        tok->value = add_char(tok->value, '\0', i, aol.word_size);

    //? Handle assignment word
    if (is_assignment_word(*tok))
        tok->type = TOKEN_ASSIGNMENT_WORD;

    tok->expansion = get_through_expansions(tok->first);

    if (tok->type == TOKEN_WORD && lexer->state == LEXER_NORMAL)
        tok->type = check_for_reserved_word(tok->value);

    if (tok->type == TOKEN_EXPANDABLE && lexer->exp_state == EXP_DQ_VAR)
    {
        struct expansion *next = calloc(1, sizeof(struct expansion));
        next->value = calloc(BASE_VALUE_SIZE, sizeof(char));
        next->type = NORMAL;

        tok->expansion->next = next;
        tok->expansion = next;

        strcpy(tok->expansion->value, tok->value);
        lexer->state = LEXER_NORMAL;
        lexer->exp_state = EXP_NONE;
    }

    return aol.c;
}

static int clang_dq_var(struct lexer *lexer, struct token *tok, int *i, char *c)
{
    *c = handle_dq_var_lex(lexer, tok, i);

    if (*c != ' ' && *c != EOF && *c != '\n' && *c != ';' && *c != '|'
        && *c != '!')
        lexer->exp_state = EXP_DQ_VAR;
    else
    {
        // Feed the last char
        if (*c != EOF)
            fseek(lexer->input->input, -1, SEEK_CUR);
        return 1;
    }

    return 0;
}

static int check_for_stopping_char(char c)
{
    if (c == ';' || c == '\n' || c == '|' || c == '&' || c == '!' || c == '('
        || c == ')' || c == '{' || c == '}')
        return 1;

    return 0;
}

static int clang_escaping(struct lexer *lexer, char *c)
{
    *c = get_char(lexer->input);
    if (*c == EOF)
        *c = '\\';

    if (*c == '\n')
    {
        *c = get_char(lexer->input);
        return 1;
    }

    return 0;
}

static void clang_while_handler(struct lexer *lexer, char *c)
{
    while (*c != EOF && *c != '\n')
        *c = get_char(lexer->input);

    return;
}

struct token lex(struct lexer *lexer, char c, struct token tok,
                 unsigned word_size)
{
    int i = 0;

    while (c != EOF && c != ' ')
    {
        if (c == '>' || c == '<')
        {
            if (i == 0)
            {
                if (tok.type == TOKEN_IONUMBER)
                    break;
                tok.value = add_char(tok.value, c, &i, &word_size);
                c = handle_redirection(lexer, &tok, &i, word_size);
            }
            else
                fseek(lexer->input->input, -1, SEEK_CUR);
            break;
        }
        //? Check for stoping characters
        if (check_for_stopping_char(c) == 1)
        {
            c = handle_stopping_char(lexer, &tok, &i, &word_size);
            break;
        }

        if (c == '\'')
        {
            c = handle_simple_quote(lexer, &tok, &i, &word_size);
            if (tok.type == TOKEN_ERROR)
                return tok;
            continue;
        }

        //? Check for comment
        if (c == '#' && i == 0)
        {
            clang_while_handler(lexer, &c);

            if (c == EOF)
            {
                tok.type = TOKEN_EOF;
                break;
            }
            fseek(lexer->input->input, -1, SEEK_CUR);
            continue;
        }

        if (c == '$' || c == '"')
        {
            if (clang_dq_var(lexer, &tok, &i, &c) == 1)
                break;
        }

        //? Escaping
        if (c == '\\')
        {
            if (clang_escaping(lexer, &c) == 1)
                continue;
        }

        if (c >= '0' && c <= '9')
            c = is_io_number(lexer, &tok, &i, word_size);
        else
        {
            //? Add the character to the word
            tok.value = add_char(tok.value, c, &i, &word_size);
            c = get_char(lexer->input);
        }
    }

    struct arg_end_of_lex aol = { .word_size = &word_size, .c = c };
    c = handle_end_of_lex(lexer, &tok, &i, aol);
    return tok;
}

static struct token init_lex(struct lexer *lexer)
{
    struct token tok = { .type = TOKEN_WORD, .value = NULL };
    unsigned word_size = BASE_VALUE_SIZE;
    tok.value = calloc(word_size, sizeof(char));

    //? Reset state
    lexer->state = LEXER_NORMAL;

    //? Read word
    char c = get_char(lexer->input);

    //? Skip spaces
    while (c == ' ')
        c = get_char(lexer->input);

    return lex(lexer, c, tok, word_size);
}

struct token lexer_peek(struct lexer *lexer)
{
    save(lexer->input);
    struct token tok = init_lex(lexer);
    restore(lexer->input);

    return tok;
}
/*
 * TODO: rewrite with new lexer
struct token lexer_peek_nth(struct lexer *lexer, int n)
{
    if (n <= 0)
    {
        fprintf(stderr, "lexer_peek_nth: n must be > 0\n");
        return (struct token){ .type = TOKEN_ERROR, .value = NULL };
    }

    char *next_word = IO_peek_word(lexer->input, n);
    struct token tok = parse_word_for_tok(next_word);

    return tok;
}
struct token *lexer_peek_array(struct lexer *lexer, int n)
{
    if (n <= 0)
    {
        fprintf(stderr, "lexer_peek_array: n must be > 0\n");
        return NULL;
    }

    struct token *tokens = calloc(n, sizeof(struct token));
    for (int i = 1; i <= n; ++i)
    {
        tokens[i-1] = lexer_peek_nth(lexer, i);
    }

    return tokens;
}
*/

struct token lexer_pop(struct lexer *lexer)
{
    struct token tok = init_lex(lexer);
    lexer->current_tok = tok;

    return tok;
}

struct lexer *lexer_new_from_string(const char *inputString)
{
    // Create an IO object from the input string
    struct IO *io = IO_create(IO_STRING, (char *)inputString);
    if (!io)
    {
        fprintf(stderr, "Failed to create IO from string\n");
        return NULL;
    }

    // Create a new lexer with the IO object
    struct lexer *lexer = lexer_new(io);
    if (!lexer)
    {
        fprintf(stderr, "Failed to create lexer\n");
        IO_free(io);
        return NULL;
    }

    return lexer;
}
