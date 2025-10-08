#ifndef OS_UTILS_H
#define OS_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

typedef enum {
    STATUS_OK = 0,
    PIPE_ERROR = 1,
    FORK_ERROR = 2,
    EXEC_ERROR = 3,
    INVALID_INPUT = 4,
    FILE_OPEN_ERROR = 5,
    IO_ERROR = 6
} StatusCode;

void remove_vowels(char* str);

#endif