#ifndef LEXER_H
#define LEXER_H

#include "IO_Backend/io.h"
#include "token.h"

#define BASE_VALUE_SIZE 16 // The base size of the buffer

enum lexer_state
{
    LEXER_NORMAL,
    LEXER_QUOTE,
    LEXER_DQUOTE,
    LEXER_ESCAPED,
    LEXER_EXPANSION,
    LEXER_ERROR
};

enum lexer_exp_state
{
    EXP_NONE,
    EXP_DQ_VAR
};

struct lexer
{
    struct IO *input; // The input data
    struct token current_tok; // The next token, if processed
    enum lexer_state state; // The current state of the lexer
    enum lexer_exp_state exp_state;
};

/**
 * \brief Creates a new lexer given an input string.
 */
struct lexer *lexer_new(struct IO *input);

/**
 ** \brief Free the given lexer, but not its input.
 */
void lexer_free(struct lexer *lexer);

/**
 * \brief Returns a token from the input string
 * This function goes through the input string character by character and
 * builds a token. lexer_peek and lexer_pop should call it. If the input is
 * invalid, you must print something on stderr and return the appropriate token.
 */
struct token parse_word_for_tok(char *word);

/**
 * \brief Returns the next token, but doesn't move forward: calling lexer_peek
 * multiple times in a row always returns the same result. This functions is
 * meant to help the parser check if the next token matches some rule.
 *
 * !IMPORTANT: This function allocates memory for the token value and DOES NOT
 * store it, !so you must free it after use.
 */
struct token lexer_peek(struct lexer *lexer);

/**
 * \brief Returns the n-th next token, but doesn't move forward: calling
 * lexer_peek multiple times in a row always returns the same result. This
 * functions is meant to help the parser check if the next token matches some
 * rule.
 *
 * !IMPORTANT: This function allocates memory for the token value and DOES NOT
 * store it, !so you must free it after use.
 *
 * \param lexer The lexer struct to read from.
 * \param n The index of the token to peek starting from current position.
 * n = 0 return a TOKEN_ERROR.
 */
// struct token lexer_peek_nth(struct lexer *lexer, int n);

/**
 * \brief Returns an array of n tokens, but doesn't move forward: calling
 * lexer_peek multiple times in a row always returns the same result. This
 * functions is meant to help the parser check if the next token matches some
 * rule.
 *
 * !IMPORTANT: This function allocates memory for the token value and DOES NOT
 * store it, !so you must free it after use.
 *
 * \param lexer The lexer struct to read from.
 * \param n The number of tokens to peek starting from current position.
 * n = 0 return NULL.
 */
// struct token *lexer_peek_array(struct lexer *lexer, int n);

/**
 * \brief Returns the next token, and removes it from the stream:
 *   calling lexer_pop in a loop will iterate over all tokens until EOF.
 */
struct token lexer_pop(struct lexer *lexer);

int is_assignment_word_str(char *word);

int is_assignment_word(struct token tok);

struct lexer *lexer_new_from_string(const char *inputString);

#endif /* !LEXER_H */
