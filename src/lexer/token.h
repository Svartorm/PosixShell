#ifndef TOKEN_H
#define TOKEN_H

enum token_type
{
    //$ Reserved words
    TOKEN_IF, // 'if'
    TOKEN_THEN, // 'then'
    TOKEN_ELIF, // 'elif'
    TOKEN_ELSE, // 'else'
    TOKEN_FI, // 'fi'
    TOKEN_FOR, // 'for'
    TOKEN_IN, // 'in'
    TOKEN_WHILE, // 'while'
    TOKEN_UNTIL, // 'until'
    TOKEN_DO, // 'do'
    TOKEN_DONE, // 'done'

    //$ Operators
    TOKEN_PIPE, // '|'
    TOKEN_NOT, // '!'
    TOKEN_AND, // '&&'
    TOKEN_OR, // '||'

    //$ Redirections
    TOKEN_REDIR_IN, // '<'
    TOKEN_REDIR_OUT, // '>' '>|'
    TOKEN_REDIR_APP_OUT, // '>>'
    TOKEN_REDIR_DUP_IN, // '<&'
    TOKEN_REDIR_DUP_OUT, // '>&'
    TOKEN_REDIR_RW, // '<>'
    TOKEN_IONUMBER, // '[0-9]+'

    //$ Special characters
    TOKEN_EOF, // end of input marker
    TOKEN_SEMI_COL, // ';'
    TOKEN_LF, // '\n'
    TOKEN_LPAREN, // '('
    TOKEN_RPAREN, // ')'
    TOKEN_LBRACKET, // '{'
    TOKEN_RBRACKET, // '}'

    //$ Others
    TOKEN_WORD, // '[a-zA-Z0-9_]+'
    TOKEN_EXPANDABLE, // '$[a-zA-Z0-9_]+'
    TOKEN_ASSIGNMENT_WORD, // '[a-zA-Z0-9_]+='
    TOKEN_FUNCTION_WORD, // '[a-zA-Z0-9_]+()'
    TOKEN_ERROR // it is not a real token, it is returned in case of invalid
                // input
};

enum expansion_type
{
    DOUBLE_QUOTE, // for $ and ""
    NORMAL,
};

struct expansion
{
    enum expansion_type type; // The kind of expansion
    char *value;
    struct expansion *next;
};

struct token
{
    enum token_type type; // The kind of token
    char *value; // The value of the token
    struct expansion *expansion; // The expansion of the token
    struct expansion *first; // The first expansion of the token
};

void token_free(struct token token);

#endif /* !TOKEN_H */
