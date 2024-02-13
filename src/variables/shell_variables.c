#include "shell_variables.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int get_random_number(void)
{
    int upper_limit = 32767;
    return rand() % upper_limit;
}

char *itoa(int num)
{
    int i = 0;
    int rem = 0;
    int len = 0;
    int n = 0;
    char *str = NULL;

    n = num;

    // Get the length of the number
    while (n != 0)
    {
        len++;
        n /= 10;
    }

    // Allocate memory for the string
    str = malloc(sizeof(char) * (len + 1));

    // Store the number in the string
    for (i = 0; i < len; i++)
    {
        rem = num % 10;
        num = num / 10;
        str[len - (i + 1)] = rem + '0';
    }

    // Add the null terminator
    str[len] = '\0';

    return str;
}

char *get_RANDOM(void)
{
    return itoa(get_random_number());
}

char *get_PWD(void)
{
    char *pwd = getenv("PWD");
    if (pwd == NULL)
    {
        fprintf(stderr, "get_PWD: getenv failed\n");
        return NULL;
    }

    return pwd;
}

char *get_OLDPWD(void)
{
    char *oldpwd = getenv("OLDPWD");
    if (oldpwd == NULL)
    {
        fprintf(stderr, "get_OLDPWD: getenv failed\n");
        return NULL;
    }

    return oldpwd;
}

char *get_ALL(void)
{
    return NULL;
}

void shell_variables_init(void)
{
    srand(time(0));
    //= $# (number of arguments)
    char *name = malloc(sizeof(char) * 2);
    name[0] = '#';
    name[1] = '\0';
    char *argc = itoa(0);
    hash_variable_set(name, argc);

    //= $? (exit status)
    name = malloc(sizeof(char) * 2);
    name[0] = '?';
    name[1] = '\0';
    char *exit_status = itoa(0);
    hash_variable_set(name, exit_status);

    //= $UID (user ID)
    name = malloc(sizeof(char) * 4);
    name[0] = 'U';
    name[1] = 'I';
    name[2] = 'D';
    name[3] = '\0';
    char *uid = itoa(getuid());
    hash_variable_set(name, uid);

    //= $$ (process ID)
    name = malloc(sizeof(char) * 2);
    name[0] = '$';
    name[1] = '\0';
    char *pid = itoa(getpid());
    hash_variable_set(name, pid);
}
