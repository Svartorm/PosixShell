#ifndef AST_EXEC_H
#define AST_EXEC_H

#include "../functions/hash_functions.h"
#include "../variables/hash_variables.h"
#include "ast.h"

// ==================================================================
// RETURN CODES FOR 'ast_exec_*' FUNCTIONS:
//
// -1 and anything <0 signifies internal error
//  0                 signifies       no error
//  1 and anything >0 signifies external error
// ==================================================================

// This will help ensure forked processes don't interact with the main process.
int current_42sh_is_a_fork(void);

/**
 ** \brief Execute ast recursively, return value is bash return code
 */
int ast_exec(struct ast *ast);

#endif /* ! AST_EXEC_H */
