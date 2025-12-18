#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "../include/contract.h"

#define STDOUT_FD 1
#define STDIN_FD 0

// Вспомогательная функция: вывод строки
static void print_str(const char* s) {
    write(STDOUT_FD, s, strlen(s));
}

// Вспомогательная функция: вывод числа
static void print_int(int num) {
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "%d", num);
    if (len > 0) {
        write(STDOUT_FD, buf, (size_t)len);
    }
}

// Парсинг числа из строки
static int parse_long(const char *str, long *val) {
    if (!str || !*str) return 0;
    long result = 0;
    int sign = 1;
    const char *p = str;

    while (*p && isspace(*p)) p++;

    if (*p == '-') {
        sign = -1;
        p++;
    } else if (*p == '+') {
        p++;
    }

    if (!isdigit(*p)) return 0;

    while (isdigit(*p)) {
        result = result * 10 + (*p - '0');
        p++;
    }

    *val = result * sign;
    return 1;
}

// Обработка команды "1" (перевод числа)
static void handle_function_1(const char* arg_str) {
    long x;
    if (!parse_long(arg_str, &x)) {
        print_str("Ошибка ввода. Для функции 1 требуется целое число\n");
        return;
    }

    // ВЫЗОВ СТАТИЧЕСКОЙ ФУНКЦИИ из библиотеки
    // Эта функция ВШИТА в программу
    char* res = translation(x);
    if (res) {
        print_str("Результат (двоичный): ");
        print_str(res);
        print_str("\n");
        free(res);
    } else {
        print_str("Ошибка выделения памяти\n");
    }
}

// Обработка команды "2" (сортировка массива)
static void handle_function_2(const char* arg_str) {
    int temp_arr[101];  // +1 для завершающего нуля
    size_t count = 0;
    const char *p = arg_str;

    while (*p && count < 100) {
        long val;
        while (*p && isspace(*p)) p++;

        const char *start = p;
        if (*p == '-' || *p == '+') p++;
        while (*p && isdigit(*p)) p++;
        const char *end = p;

        if (end > start) {
            char num_str[30];
            size_t len = end - start;
            if (len >= sizeof(num_str)) len = sizeof(num_str) - 1;
            memcpy(num_str, start, len);
            num_str[len] = '\0';
            
            if (parse_long(num_str, &val)) {
                temp_arr[count++] = (int)val;
            }
        }
        
        if (*p == '\0' || end == start) break;
    }

    if (count == 0) {
        print_str("Ошибка ввода. Требуется список целых чисел\n");
        return;
    }

    temp_arr[count] = 0;  // Завершаем массив нулем
    
    int* arr_to_sort = (int *)malloc((count + 1) * sizeof(int));
    if (arr_to_sort == NULL) {
        print_str("Ошибка выделения памяти\n");
        return;
    }

    memcpy(arr_to_sort, temp_arr, (count + 1) * sizeof(int));
    
    print_str("Исходный массив: ");
    for (size_t i = 0; i < count; i++) {
        print_int(temp_arr[i]);
        print_str(" ");
    }
    print_str("\n");

    // ВЫЗОВ СТАТИЧЕСКОЙ ФУНКЦИИ сортировки
    int* sorted_arr = sort(arr_to_sort);
    
    print_str("Результат (Пузырьковая): ");
    for(size_t i = 0; sorted_arr[i] != 0; i++) {
        print_int(sorted_arr[i]);
        print_str(" ");
    }
    print_str("\n");

    free(arr_to_sort);
}

int main() {
    char line[512];
    ssize_t bytes_read;
    
    print_str("Программа 1: Статическая линковка\n");
    print_str("Функция 1: Перевод в двоичную систему счисления\n");
    print_str("Функция 2: Пузырьковая сортировка\n");
    print_str("Введите команду: (1 [число] или 2 [массив чисел] или 'q' для выхода)\n");

    while ((bytes_read = read(STDIN_FD, line, sizeof(line) - 1)) > 0) {
        line[bytes_read] = '\0';
        
        if (line[bytes_read - 1] == '\n') {
            line[bytes_read - 1] = '\0';
        }
        
        if (line[0] == 'q') {
            break;
        } 
        
        if (line[0] == '\0') continue;

        int cmd = line[0] - '0';
        char* args = line + 1;
        while (*args == ' ' || *args == '\t') {
            args++;
        }
        
        switch(cmd) {
            case 1:
                handle_function_1(args);
                break;
            case 2:
                handle_function_2(args);
                break;
            default:
                print_str("Недоступная команда, введите 1 или 2\n");
        }
        
        print_str("Введите команду: (1 [число] или 2 [массив чисел] или 'q' для выхода)\n");
    }
    return 0;
}