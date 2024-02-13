#define _POSIX_C_SOURCE 200809L

#include "ast_exec.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../exit_codes.h"
#include "../lexer/lexer.h"
#include "../parser/parser.h"
#include "../variables/shell_variables.h"

// ==================================================================
// RETURN CODES FOR 'ast_exec_*' FUNCTIONS:
//
// -1 and anything <0 signifies internal error
//  0                 signifies       no error
//  1 and anything >0 signifies external error
// ==================================================================

int ast_exec_dot(struct ast *ast);
static int ast_exec_export(struct ast *ast);
int ast_exec_cd(struct ast *ast);

static int ast_exec_redir_in(struct ast *ast, struct ast *redir);
static int ast_exec_redir_out(struct ast *ast, struct ast *redir);
static int ast_exec_redir_app_out(struct ast *ast, struct ast *redir);
static int ast_exec_redir_dup_in(struct ast *ast, struct ast *redir);
static int ast_exec_redir_dup_out(struct ast *ast, struct ast *redir);
static int ast_exec_redir_rw(struct ast *ast, struct ast *redir);

// This will help ensure forked processes don't interact with the main process.
static int current_is_a_fork = 0;
static size_t number_of_loops = 0;
static size_t number_of_breaks = 0;
static size_t number_of_continues = 0;

int current_42sh_is_a_fork(void)
{
    return current_is_a_fork;
}

static void print_with_escapes(char *string, int escape)
{
    if (!string)
        return;

    while (*string)
    {
        if (escape && *string == '\\')
        {
            // Move to the escape character
            string++;
            switch (*string)
            {
            case 'n':
                putchar('\n');
                break;
            case 't':
                putchar('\t');
                break;
            case '\\':
                putchar('\\');
                break;
            default:
                // If it's not a recognized escape sequence, print the backslash
                // and the character
                if (string)
                {
                    putchar(*string);
                }
            }
        }
        else
        {
            putchar(*string);
        }
        if (*string)
        {
            string++; // Move to the next character
        }
    }
}

static int unset_var(char *name)
{
    hash_variable_del(name);
    return 0;
}

static int unset_fun(char *name)
{
    hash_function_del(name);
    return 0;
}

static int ast_exec_echo(struct ast *ast)
{
    int newline = 1; // echo prints a newline at the end
    int backslash_escapes = 0; // backslash escapes are not interpreted
    size_t start_index = 0; // Initialize the start index

    // Parse options
    for (; start_index < ast->nb_sons; ++start_index)
    {
        struct ast *son = ast_get_son(ast, start_index);
        char *arg = son->value;

        if (arg[0] != '-')
        {
            // Not an option, break
            break;
        }

        int is_option = 1;
        // Process each character as a separate option
        for (size_t j = 1; arg[j] != '\0' && is_option; j++)
        {
            switch (arg[j])
            {
            case 'n':
                newline = 0; // Do not print newline at the end
                break;
            case 'e':
                backslash_escapes = 1; // Enable backslash escapes
                break;
            case 'E':
                backslash_escapes = 0; // Disable backslash escapes
                break;
            default:
                // No character found
                is_option = 0;
                break;
            }
        }
        if (!is_option)
        {
            // Current arg is not an option -> break
            break;
        }
    }
    // Print arguments after options
    for (size_t i = start_index; i < ast->nb_sons; ++i)
    {
        if (i > start_index)
            printf(" "); // Print spaces between arguments

        struct ast *son = ast_get_son(ast, i);
        char *string = son->value;

        // Print with or without backslash escapes based on the option
        print_with_escapes(string, backslash_escapes);
    }

    if (newline)
        putchar('\n');
    return 0;
}

static int ast_exec_break(struct ast *ast)
{
    if (ast->nb_sons > 1)
    {
        fprintf(stderr, "ast_exec_break: Wrong number of sons (%zu).\n",
                ast->nb_sons);
        return EC_UNKNOWN;
    }
    int nb_breaks = ast->nb_sons == 0 ? 1 : atoi(ast_get_son(ast, 0)->value);
    number_of_breaks += nb_breaks;
    return EC_BREAK;
}

static int ast_exec_continue(struct ast *ast)
{
    if (ast->nb_sons > 1)
    {
        fprintf(stderr, "ast_exec_continue: Wrong number of sons (%zu).\n",
                ast->nb_sons);
        return EC_UNKNOWN;
    }
    int nb_continues = ast->nb_sons == 0 ? 1 : atoi(ast_get_son(ast, 0)->value);
    number_of_continues += nb_continues;
    return EC_CONTINUE;
}

static int ast_exec_exit(struct ast *ast)
{
    if (ast->nb_sons > 1)
    {
        return EC_EXIT_MIN;
    }
    int exit_code = 0;
    if (ast->nb_sons == 1)
        exit_code = atoi(ast_get_son(ast, 0)->value);
    else
    {
        struct variable *v = hash_variable_get("?");
        exit_code = atoi(v == NULL ? "0" : v->value);
    }
    return EC_EXIT_MIN + (exit_code % 256);
}

static int ast_exec_function(struct ast *ast, struct function *func)
{
    // we are currently missing the part where we set the variables
    // for the function arguements.
    if (ast == NULL)
        return EC_UNKNOWN;
    return ast_exec(func->value);
}

/*
 * Executes a non-builtin program.
 * It uses a fork + execvp method.
 */
int ast_exec_program(struct ast *ast)
{
    // (1) get all arguments (2) fork and execvp
    // get all arguments
    size_t argc = ast->nb_sons;
    char **argv = calloc(sizeof(char *), argc + 2);
    if (!argv)
    {
        fprintf(stderr, "ast_exec_program: Memory error.\n");
        return EC_MEMORY;
    }
    argv[0] = ast->value;
    for (size_t i = 0; i < argc; ++i)
    {
        struct ast *son = ast_get_son(ast, i);
        argv[i + 1] = son->value;
    }

    // fork and execvp
    int pid = fork();
    if (pid == -1)
    {
        fprintf(stderr, "ast_exec_program: Problem with fork.\n");
        free(argv);
        return EC_UNKNOWN;
    }
    else if (pid == 0)
    {
        current_is_a_fork = 1;
        if (execvp(argv[0], argv) == -1)
            fprintf(stderr, "ast_exec_program: Problem with execvp.\n");
        free(argv);
        return EC_COMMAND_NOT_FOUND;
    }
    int wait_status;
    waitpid(pid, &wait_status, 0);
    if (!WIFEXITED(wait_status))
    {
        fprintf(stderr, "ast_exec_program: Child did not terminate smoothly.");
        free(argv);
        return EC_UNKNOWN;
    }
    free(argv);
    return WEXITSTATUS(wait_status);
}

static int ast_exec_special_command(struct ast *ast)
{
    if (strcmp(ast->value, "true") == 0)
    {
        return 0;
    }
    else if (strcmp(ast->value, "false") == 0)
    {
        return 1;
    }
    else if (strcmp(ast->value, "exit") == 0)
    {
        return ast_exec_exit(ast);
    }
    else if (strcmp(ast->value, "break") == 0)
    {
        return ast_exec_break(ast);
    }
    else if (strcmp(ast->value, "continue") == 0)
    {
        return ast_exec_continue(ast);
    }
    else if (strcmp(ast->value, ".") == 0)
    {
        return ast_exec_dot(ast);
    }
    else if (strcmp(ast->value, "export") == 0)
    {
        return ast_exec_export(ast);
    }
    else if (strcmp(ast->value, "cd") == 0)
    {
        return ast_exec_cd(ast);
    }
    return ast_exec_program(ast);
}

static int ast_exec_unset(struct ast *ast)
{
    if (ast->nb_sons > 2)
    {
        fprintf(stderr,
                "unset: too many arguments, format: [OPTION] [NAME] (%zu).\n",
                ast->nb_sons);
        return -1; // Indique une erreur
    }

    if (ast->nb_sons == 2)
    {
        struct ast *option_ast = ast_get_son(ast, 0);
        char *option = option_ast->value;
        struct ast *name_ast = ast_get_son(ast, 1);
        char *name = name_ast->value;

        if (strcmp(option, "-v") == 0)
        {
            return unset_var(name);
        }
        else if (strcmp(option, "-f") == 0)
        {
            return unset_fun(name);
        }
        else
        {
            fprintf(stderr,
                    "unset: invalid option '%s'. Only '-v' (variable) and '-f' "
                    "(function) are supported.\n",
                    option);
            return -1;
        }
    }
    else if (ast->nb_sons == 1)
    {
        struct ast *name_ast = ast_get_son(ast, 0);
        char *name = name_ast->value;
        return unset_var(name);
    }
    else
    {
        fprintf(stderr, "unset: no arguments provided.\n");
        return -1;
    }
}

/*
 * Executes a list of commands.
 * If the command is a builtin, we have our own function for it.
 * Otherwise, we fork to execvp the relevant binary.
 */
static int ast_exec_command(struct ast *ast)
{
    int command_result = 0;
    // Search for a function, functions have priority over built-ins
    struct function *func = hash_function_get(ast->value);
    if (func != NULL)
    {
        return ast_exec_function(ast, func);
    }
    else if (strcmp(ast->value, "echo") == 0)
    {
        struct ast *filter_ast = ast_new(AST_COMMAND, strdup("echo"));
        if (!filter_ast)
        {
            // Handle memory allocation failure
            return EC_MEMORY;
        }

        for (size_t i = 0; i < ast->nb_sons; ++i)
        {
            struct ast *child = ast_get_son(ast, i);
            if (child->type == AST_ARGUMENT || child->type == AST_EXPANSION)
            {
                struct ast *argument_node =
                    ast_new(AST_ARGUMENT, strdup(child->value));
                if (!argument_node)
                {
                    // Handle memory allocation failure
                    ast_free(filter_ast);
                    return EC_MEMORY;
                }
                ast_append_son(filter_ast, argument_node);
            }
        }
        // Execute echo with the filtered arguments
        command_result = ast_exec_echo(filter_ast);
        ast_free(filter_ast);
    }
    else if (strcmp(ast->value, "unset") == 0)
    {
        return ast_exec_unset(ast);
    }
    else
    {
        return ast_exec_special_command(ast);
    }
    fflush(NULL);
    return command_result;
}

/*
 * Executes a list of commands.
 * The return code is the return code of the last command.
 */
static int ast_exec_command_list(struct ast *ast)
{
    size_t max_son = ast->nb_sons;
    int return_code = 0;
    for (size_t i = 0; i < max_son; ++i)
    {
        return_code = ast_exec(ast_get_son(ast, i));
        if (return_code < 0)
            break;
    }
    return return_code;
}

// forks from the current program, returning the pid of the child.
// additionally, inside the child, closes a specified fd.
static int pipe_fork_exec_close(struct ast *ast, int fd_to_close)
{
    int pid = fork();
    if (pid == -1)
    {
        fprintf(stderr, "ast_exec_pipe: Problem with fork.\n");
        return -1;
    }
    else if (pid == 0)
    {
        current_is_a_fork = 1;
        close(fd_to_close);
        return EC_EXIT_MIN + ast_exec(ast);
    }
    return pid;
}

static int ast_exec_pipe(struct ast *ast)
{
    int pipe_fds[2] = { -1, -1 }; // { read_pipe, write_pipe }
    if (pipe(pipe_fds) == -1)
    {
        fprintf(stderr, "ast_exec_pipe: Could not create pipe.\n");
        return EC_UNKNOWN;
    }

    int stdin_dup = dup(STDIN_FILENO);
    int read_pipe_fd_stdin = dup2(pipe_fds[0], STDIN_FILENO);
    if (read_pipe_fd_stdin == -1)
    {
        fprintf(stderr, "ast_exec_pipe: Could not create redirection.\n");
        return EC_UNKNOWN;
    }

    // second ast closes write side of pipe and waits for input
    int right_pid = pipe_fork_exec_close(ast_get_son(ast, 1), pipe_fds[1]);
    if (right_pid < 0)
    {
        return right_pid;
    }

    // restore stdin
    close(read_pipe_fd_stdin);
    dup2(stdin_dup, STDIN_FILENO);
    close(stdin_dup);

    int stdout_dup = dup(STDOUT_FILENO);
    int write_pipe_fd_stdout = dup2(pipe_fds[1], STDOUT_FILENO);
    if (write_pipe_fd_stdout == -1)
    {
        fprintf(stderr, "ast_exec_pipe: Could not create redirection.\n");
        return EC_UNKNOWN;
    }

    // first ast closes read side of pipe and gives input
    int left_pid = pipe_fork_exec_close(ast_get_son(ast, 0), pipe_fds[0]);
    if (left_pid < 0)
    {
        return left_pid;
    }

    // restore stdout
    close(write_pipe_fd_stdout);
    dup2(stdout_dup, STDOUT_FILENO);
    close(stdout_dup);

    waitpid(left_pid, NULL, 0);
    close(pipe_fds[1]);

    int wait_status;
    waitpid(right_pid, &wait_status, 0);
    close(pipe_fds[0]);
    if (!WIFEXITED(wait_status))
    {
        fprintf(stderr, "ast_exec_pipe: Child exited in an unexpected way.\n");
        return EC_UNKNOWN;
    }
    int right_return = WEXITSTATUS(wait_status);

    return right_return;
}

int ast_exec_assignment(struct ast *ast)
{
    //? Copy the variable name and the word
    char *var_name = malloc(strlen(ast->value) + 1);
    if (!var_name)
    {
        fprintf(stderr, "ast_exec_assignment: Memory error.\n");
        return EC_MEMORY;
    }
    strcpy(var_name, ast->value);
    struct ast *word_ast = ast_get_son(ast, 0);
    char *word = malloc(strlen(word_ast->value) + 1);
    if (!word)
    {
        fprintf(stderr, "ast_exec_assignment: Memory error.\n");
        free(var_name);
        return EC_MEMORY;
    }
    strcpy(word, word_ast->value);
    struct variable *var = hash_variable_set(var_name, word);
    if (!var)
    {
        fprintf(stderr, "ast_exec_assignment: Memory error.\n");
        free(var_name);
        free(word);
        return -1;
    }
    return 0;
}

/*
(AST: if) ;; if originel
+-- (AST: command) ;; condition du if
+-- (AST: command) ;; code à exécuter si la condition renvoie vrai
+-+ (AST: if) ;; elif
  +-- (AST: command) ;; condition du elif
  +-- (AST: command) ;; code à exécuter si la condition renvoie vrai
  +-- (AST: command) ;; code à éxecuter pour le else (comme le type n'est pas
'if' et que le 3ème fils existe, c'est qu'on a un else)
 */
static int ast_exec_conditional(struct ast *ast)
{
    int condition_result = ast_exec(ast_get_son(ast, 0));
    if (condition_result < 0) // internal error
    {
        return condition_result;
    }
    else if (condition_result == 0) // condition is true
    {
        return ast_exec(ast_get_son(ast, 1));
    }
    else // condition is false
    {
        // the third son can be elif/else/nothing.
        struct ast *son_else = ast_get_son(ast, 2);
        if (son_else == NULL) // no else clause is given
        {
            return 0;
        }
        else
        {
            return ast_exec(son_else);
        }
    }
}

static int ast_exec_while(struct ast *ast)
{
    ++number_of_loops;
    int inside_code = 0; // if loop never executes, return 0
    while (ast_exec(ast_get_son(ast, 0)) == 0) // while loop condition is true
    {
        inside_code = ast_exec(ast_get_son(ast, 1));
        if (inside_code == EC_BREAK && number_of_breaks > 0)
        {
            --number_of_breaks;
            if (number_of_breaks == 0 || number_of_loops == 1)
                inside_code = 0; // if we are finished breaking/continuing, we
                                 // can continue execution
            break;
        }
        else if (inside_code == EC_CONTINUE && number_of_continues > 0)
        {
            --number_of_continues;
            if (number_of_continues == 0 || number_of_loops == 1)
                inside_code = 0; // if we are finished breaking/continuing, we
                                 // can continue execution
            // if we have to continue on an enclosing loop,
            // and if there is an enclosing loop,
            // then we break out of the current one.
            if (number_of_continues > 0 && number_of_loops >= 2)
                break;
        }
        else if (inside_code < 0 && inside_code != EC_BREAK
                 && inside_code != EC_CONTINUE)
            break;
    }
    --number_of_loops;
    return inside_code;
}

static int ast_exec_until(struct ast *ast)
{
    ++number_of_loops;
    int inside_code = 0; // if loop never executes, return 0
    while (ast_exec(ast_get_son(ast, 0)) > 0) // while loop condition is false
    // (note that internal errors (<0) still exit the loop)
    {
        inside_code = ast_exec(ast_get_son(ast, 1));
        if (inside_code == EC_BREAK && number_of_breaks > 0)
        {
            --number_of_breaks;
            if (number_of_breaks == 0 || number_of_loops == 1)
                inside_code = 0; // if we are finished breaking/continuing, we
                                 // can continue execution
            break;
        }
        else if (inside_code == EC_CONTINUE && number_of_continues > 0)
        {
            --number_of_continues;
            if (number_of_continues == 0 || number_of_loops == 1)
                inside_code = 0; // if we are finished breaking/continuing, we
                                 // can continue execution
            // if we have to continue on an enclosing loop,
            // and if there is an enclosing loop,
            // then we break out of the current one.
            if (number_of_continues > 0 && number_of_loops >= 2)
                break;
        }
        else if (inside_code < 0 && inside_code != EC_BREAK
                 && inside_code != EC_CONTINUE)
            break;
    }
    --number_of_loops;
    return inside_code;
}

static int ast_exec_not(struct ast *ast)
{
    int return_code_inside = ast_exec(ast_get_son(ast, 0));
    if (return_code_inside < 0 || return_code_inside == -EC_COMMAND_NOT_FOUND)
        return return_code_inside;
    else if (return_code_inside == 0)
        return 1;
    else
        return 0;
}

static int ast_exec_and(struct ast *ast)
{
    int return_code_inside = ast_exec(ast_get_son(ast, 0));
    if (return_code_inside < 0)
        return return_code_inside;
    else if (return_code_inside == 0)
        return ast_exec(ast_get_son(ast, 1));
    else
        return return_code_inside;
}

static int ast_exec_or(struct ast *ast)
{
    int return_code_inside = ast_exec(ast_get_son(ast, 0));
    if (return_code_inside < 0)
        return return_code_inside;
    else if (return_code_inside == 0)
        return return_code_inside;
    else
        return ast_exec(ast_get_son(ast, 1));
}

static int ast_exec_for(struct ast *ast)
{
    ++number_of_loops;
    char *var_name = ast->value;
    struct ast *list = ast_get_son(ast, 0); // First child is the list
    struct ast *compound_list = ast_get_son(ast, 1); // Second child is the body

    int inside_code = 0; // if nothing is executed, return 0
    int loop_flag = 1;
    for (size_t i = 0; i < list->nb_sons && loop_flag; ++i)
    {
        struct ast *word_ast = ast_get_son(list, i);
        char *word = word_ast->value;

        setenv(var_name, word, 1); // Set the loop variable to the current word

        inside_code =
            ast_exec(compound_list); // Execute the body of the for loop
        if (inside_code == EC_BREAK && number_of_breaks > 0)
        {
            --number_of_breaks;
            if (number_of_breaks == 0 || number_of_loops == 1)
                inside_code = 0; // if we are finished breaking/continuing, we
                                 // can continue execution
            loop_flag = 0;
        }
        else if (inside_code == EC_CONTINUE && number_of_continues > 0)
        {
            --number_of_continues;
            if (number_of_continues == 0 || number_of_loops == 1)
                inside_code = 0; // if we are finished breaking/continuing, we
                                 // can continue execution
            // if we have to continue on an enclosing loop,
            // and if there is an enclosing loop,
            // then we break out of the current one.
            if (number_of_continues > 0 && number_of_loops >= 2)
                loop_flag = 0;
        }
        else if (inside_code < 0 && inside_code != EC_BREAK
                 && inside_code != EC_CONTINUE)
            loop_flag = 0;

        unsetenv(var_name); // Clean env variable after each iteration
    }
    --number_of_loops;
    return inside_code;
}

static int ast_exec_subshell(struct ast *ast)
{
    int pid = fork();
    if (pid == -1)
    {
        perror("fork");
        return -1;
    }
    else if (pid == 0)
    {
        current_is_a_fork = 1;
        int status = ast_exec(ast_get_son(ast, 0));
        exit(status);
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status))
        {
            return WEXITSTATUS(status);
        }
        else
        {
            fprintf(stderr, "Subshell did not terminate normally.\n");
            return -1;
        }
    }
}

static int ast_exec_funcdec(struct ast *ast)
{
    return hash_function_set(ast->value, ast_get_son(ast, 0)) == NULL
        ? EC_UNKNOWN
        : 0;
    // return hash_function_set(strdup(ast->value), ast_get_son(ast, 0)) == NULL
    // ? EC_UNKNOWN : 0;
}

int ast_exec_dot(struct ast *ast)
{
    if (ast->nb_sons < 1)
    {
        fprintf(stderr, "Error: No filename provided for the dot command.\n");
        return -1;
    }

    char *filename = ast_get_son(ast, 0)->value;
    if (!filename)
    {
        fprintf(stderr, "Error: Failed to retrieve filename from AST.\n");
        return -1;
    }

    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Error opening file");
        return -1;
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int status = 0;

    while ((read = getline(&line, &len, file)) != -1)
    {
        struct lexer *line_lexer = lexer_new_from_string(line);
        struct ast *line_ast = NULL;

        if (parse_input(&line_ast, line_lexer) != 0)
        {
            fprintf(stderr, "Error parsing line: %s\n", line);
            status = -1;
            lexer_free(line_lexer);
            break;
        }

        status = ast_exec(line_ast);

        ast_free(line_ast);
        lexer_free(line_lexer);

        if (status != 0)
        {
            fprintf(stderr, "Error executing line: %s\n", line);
            break;
        }
    }

    free(line);
    fclose(file);

    return status;
}

void ast_exported_variables(void)
{
    extern char **environ;
    for (char **env = environ; *env; ++env)
    {
        char *var = *env;
        char *value = strchr(var, '=');
        if (value)
        {
            *value = '\0';
            printf("%s='%s'\n", var, value + 1);
            *value = '=';
        }
    }
}

static int ast_exec_export(struct ast *ast)
{
    if (ast->nb_sons == 0)
    {
        ast_exported_variables();
        return 0;
    }

    for (size_t i = 0; i < ast->nb_sons; i++)
    {
        struct ast *var_ast = ast_get_son(ast, i);
        char *assignment = strdup(var_ast->value);
        if (!assignment)
        {
            perror("strdup");
            return -1;
        }

        char *equal = strchr(assignment, '=');

        if (equal)
        {
            *equal = '\0';
            char *var_name = assignment;
            char *var_value = equal + 1;

            if (setenv(var_name, var_value, 1) != 0)
            {
                perror("setenv");
                free(assignment);
                return 1;
            }
        }
        else
        {
            if (getenv(assignment) == NULL)
            {
                if (setenv(assignment, "", 1) != 0)
                {
                    perror("setenv");
                    free(assignment);
                    return 2; // Handle setenv failure
                }
            }
        }
        free(assignment);
    }

    return 0;
}

static int change_directory(const char *new_dir)
{
    char old_cwd[1000];
    if (getcwd(old_cwd, sizeof(old_cwd)) == NULL)
    {
        perror("cd: getcwd failed to get current directory");
        return 1;
    }

    if (chdir(new_dir) != 0)
    {
        perror("cd");
        return 1;
    }

    setenv("OLDPWD", old_cwd, 1);

    char new_cwd[1000];
    if (getcwd(new_cwd, sizeof(new_cwd)) == NULL)
    {
        perror("cd: getcwd failed to get new directory");
        return 1;
    }
    setenv("PWD", new_cwd, 1);

    return 0;
}

int ast_exec_cd(struct ast *ast)
{
    if (ast->nb_sons > 0 && strcmp(ast_get_son(ast, 0)->value, "-") == 0)
    {
        char *oldpwd = getenv("OLDPWD");
        if (!oldpwd)
        {
            fprintf(stderr, "cd: OLDPWD not set\n");
            return 1;
        }
        return change_directory(oldpwd);
    }

    if (ast->nb_sons == 0)
    {
        const char *home_dir = getenv("HOME");
        if (!home_dir)
        {
            fprintf(stderr, "cd: HOME not set\n");
            return 1;
        }
        return change_directory(home_dir);
    }

    const char *new_dir = ast_get_son(ast, ast->nb_sons - 1)->value;
    return change_directory(new_dir);
}

static int ast_exec_redir_folder_rec(struct ast *ast, struct ast *redir)
{
    if (redir == NULL)
    {
        fflush(NULL);
        int return_code = ast_exec(ast_get_son(ast, 0));
        fflush(NULL);
        return return_code;
    }
    else
    {
        if (redir->type == AST_REDIR_IN)
            return ast_exec_redir_in(ast, redir);
        else if (redir->type == AST_REDIR_OUT)
            return ast_exec_redir_out(ast, redir);
        else if (redir->type == AST_REDIR_APP_OUT)
            return ast_exec_redir_app_out(ast, redir);
        else if (redir->type == AST_REDIR_DUP_IN)
            return ast_exec_redir_dup_in(ast, redir);
        else if (redir->type == AST_REDIR_DUP_OUT)
            return ast_exec_redir_dup_out(ast, redir);
        else if (redir->type == AST_REDIR_RW)
            return ast_exec_redir_rw(ast, redir);
    }
    return EC_UNKNOWN;
}

static int ast_exec_redir_in(struct ast *ast, struct ast *redir)
{
    fflush(NULL);
    char *filename = redir->value;
    int redir_fd = redir->nb_sons;

    int fd_in = open(filename, O_RDONLY);
    if (fd_in == -1)
    {
        fprintf(stderr, "ast_exec_redir_in: Could not open '%s'\n", filename);
        return EC_UNKNOWN;
    }

    int stdin_dup = dup(redir_fd);

    int redirection = dup2(fd_in, redir_fd);
    if (redirection == -1)
    {
        fprintf(stderr, "ast_exec_redir_in: Could not create redirection.\n");
        return EC_UNKNOWN;
    }
    fflush(NULL);

    int return_code = ast_exec_redir_folder_rec(ast, redir->right_brother);
    fflush(NULL);

    close(redirection);
    close(fd_in);
    dup2(stdin_dup, redir_fd);
    close(stdin_dup);
    fflush(NULL);

    return return_code;
}

static int ast_exec_redir_out(struct ast *ast, struct ast *redir)
{
    fflush(NULL);
    char *filename = redir->value;
    int redir_fd = redir->nb_sons;

    // 6 * 64 + 4 * 8 + 4 = 420
    int fd_out = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 420);
    if (fd_out == -1)
    {
        fprintf(stderr, "ast_exec_redir_out: Could not open '%s'\n", filename);
        return EC_UNKNOWN;
    }

    int stdout_dup = dup(redir_fd);

    int redirection = dup2(fd_out, redir_fd);
    if (redirection == -1)
    {
        fprintf(stderr, "ast_exec_redir_out: Could not create redirection.\n");
        return EC_UNKNOWN;
    }
    fflush(NULL);

    int return_code = ast_exec_redir_folder_rec(ast, redir->right_brother);
    fflush(NULL);

    close(redirection);
    close(fd_out);
    dup2(stdout_dup, redir_fd);
    close(stdout_dup);
    fflush(NULL);

    return return_code;
}

static int ast_exec_redir_app_out(struct ast *ast, struct ast *redir)
{
    fflush(NULL);
    char *filename = redir->value;
    int redir_fd = redir->nb_sons;

    // 6 * 64 + 4 * 8 + 4 = 420
    int fd_app_out = open(filename, O_WRONLY | O_CREAT | O_APPEND, 420);
    if (fd_app_out == -1)
    {
        fprintf(stderr, "ast_exec_redir_app_out: Could not open '%s'\n",
                filename);
        return EC_UNKNOWN;
    }

    int stdapp_out_dup = dup(redir_fd);

    int redirection = dup2(fd_app_out, redir_fd);
    if (redirection == -1)
    {
        fprintf(stderr,
                "ast_exec_redir_app_out: Could not create redirection.\n");
        return EC_UNKNOWN;
    }
    fflush(NULL);

    int return_code = ast_exec_redir_folder_rec(ast, redir->right_brother);
    fflush(NULL);

    close(redirection);
    close(fd_app_out);
    dup2(stdapp_out_dup, redir_fd);
    close(stdapp_out_dup);
    fflush(NULL);

    return return_code;
}

static int ast_exec_redir_dup_in(struct ast *ast, struct ast *redir)
{
    return ast_exec_redir_in(ast, redir);
}

static int ast_exec_redir_dup_out(struct ast *ast, struct ast *redir)
{
    return ast_exec_redir_out(ast, redir);
}

static int ast_exec_redir_rw(struct ast *ast, struct ast *redir)
{
    fflush(NULL);
    char *filename = redir->value;
    int redir_fd = redir->nb_sons;

    int fd_rw = open(filename, O_RDWR | O_CREAT, 420);
    if (fd_rw == -1)
    {
        fprintf(stderr, "ast_exec_redir_rw: Could not open '%s'\n", filename);
        return EC_UNKNOWN;
    }

    int stdrw_dup = dup(redir_fd);

    int redirection = dup2(fd_rw, redir_fd);
    if (redirection == -1)
    {
        fprintf(stderr, "ast_exec_redir_rw: Could not create redirection.\n");
        return EC_UNKNOWN;
    }
    fflush(NULL);

    int return_code = ast_exec_redir_folder_rec(ast, redir->right_brother);
    fflush(NULL);

    close(redirection);
    close(fd_rw);
    dup2(stdrw_dup, redir_fd);
    close(stdrw_dup);
    fflush(NULL);

    return return_code;
}

static int ast_exec_redir_folder(struct ast *ast)
{
    if (ast->nb_sons < 2)
        return ast_exec(ast_get_son(ast, 0));
    else
        return ast_exec_redir_folder_rec(ast, ast_get_son(ast, 1));
}

#if 0
static int ast_exec_(struct ast *ast)
#endif /* 0 */

static int (*array_ast_exec[])(struct ast *ast) = {
    [AST_COMMAND] = ast_exec_command,
    [AST_COMMAND_LIST] = ast_exec_command_list,
    [AST_CONDITIONAL] = ast_exec_conditional,
    [AST_PIPE] = ast_exec_pipe,
    [AST_VARIABLE] = ast_exec_assignment,
    [AST_WHILE] = ast_exec_while,
    [AST_UNTIL] = ast_exec_until,
    [AST_NOT] = ast_exec_not,
    [AST_AND] = ast_exec_and,
    [AST_OR] = ast_exec_or,
    [AST_FOR] = ast_exec_for,
    [AST_SUBSHELL] = ast_exec_subshell,
    [AST_FUNCDEC] = ast_exec_funcdec,
    [AST_REDIR_FOLDER] = ast_exec_redir_folder,
#if 0
    [AST_] = ast_exec_,
#endif /* 0 */
};

int ast_exec(struct ast *ast)
{
    if (ast == NULL)
        return 0;
    int return_code = array_ast_exec[ast->type](ast);
    char *var_name = malloc(sizeof(char) * 2);
    var_name[0] = '?';
    var_name[1] = '\0';
    char *var_value = itoa(return_code);
    hash_variable_set(var_name, var_value);
    return return_code;
}
