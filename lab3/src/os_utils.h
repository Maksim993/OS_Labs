#ifndef OS_UTILS_H
#define OS_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 1024
#define SHARED_MEM_SIZE 4096
#define MAX_FILENAME_LENGTH 256

typedef enum {
    STATUS_OK = 0,
    MMAP_ERROR = 1,
    FORK_ERROR = 2,
    EXEC_ERROR = 3,        // Добавлена эта строка
    INVALID_INPUT = 4,
    FILE_OPEN_ERROR = 5,
    IO_ERROR = 6
} StatusCode;

// Структура для разделяемой памяти
typedef struct {
    char data[MAX_LINE_LENGTH];
    int data_ready;        // Флаг наличия данных
    int process_complete;  // Флаг завершения процесса
} shared_data_t;

void remove_vowels(char* str);

#endif