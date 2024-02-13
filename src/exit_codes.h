#ifndef EXIT_CODES_H
#define EXIT_CODES_H

// EC = Exit Code

#define EC_UNKNOWN -1
#define EC_SYNTAX -2

#define EC_COMMAND_NOT_EXECUTABLE -126
#define EC_COMMAND_NOT_FOUND -127

#define EC_MEMORY -3000
#define EC_FORK_PROBLEM -3001

#define EC_EXIT_MAX -4245
#define EC_EXIT_MIN -4500
#define EC_BREAK -5000
#define EC_CONTINUE -5001

#endif /* ! EXIT_CODES_H */
