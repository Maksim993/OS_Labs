#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dlfcn.h> 
#include <unistd.h>
#include "../include/contract.h"

// Типы указателей на функции
typedef char* (*TranslationFunc)(long);
typedef int* (*SortFunc)(int*);

static TranslationFunc current_translation = NULL;
static SortFunc current_sort = NULL;

static void *lib_handle_1_v1 = NULL;
static void *lib_handle_1_v2 = NULL;
static void *lib_handle_2_v1 = NULL;
static void *lib_handle_2_v2 = NULL;

static int current_impl_1 = 1;  // 1 - двоичная, 2 - троичная
static int current_impl_2 = 1;  // 1 - пузырьковая, 2 - Хоара

#define LIB1_V1_PATH "bin/libtranslation_bin.so"
#define LIB1_V2_PATH "bin/libtranslation_ter.so"
#define LIB2_V1_PATH "bin/libsort_bubble.so"
#define LIB2_V2_PATH "bin/libsort_quick.so"

static void write_str(const char *str) {
    write(STDOUT_FILENO, str, strlen(str));
}

static void write_int(int num) {
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "%d", num);
    if (len > 0) {
        write(STDOUT_FILENO, buf, (size_t)len);
    }
}

static int read_line(char *buffer, size_t max_len) {
    ssize_t bytes_read = read(STDIN_FILENO, buffer, max_len - 1);
    if (bytes_read <= 0) {
        return 0;
    }
    buffer[bytes_read] = '\0';
    
    char *newline = strchr(buffer, '\n');
    if (newline) {
        *newline = '\0';
    }
    
    return (int)bytes_read;
}

static int load_libraries() {
    lib_handle_1_v1 = dlopen(LIB1_V1_PATH, RTLD_LAZY);
    lib_handle_1_v2 = dlopen(LIB1_V2_PATH, RTLD_LAZY);
    lib_handle_2_v1 = dlopen(LIB2_V1_PATH, RTLD_LAZY);
    lib_handle_2_v2 = dlopen(LIB2_V2_PATH, RTLD_LAZY);

    if (!lib_handle_1_v1 || !lib_handle_1_v2 || !lib_handle_2_v1 || !lib_handle_2_v2) {
        char err_msg[512];
        snprintf(err_msg, sizeof(err_msg), "Ошибка загрузки одной из библиотек: %s\n", dlerror());
        write(STDERR_FILENO, err_msg, strlen(err_msg));
        return 0;
    }

    current_translation = (TranslationFunc)dlsym(lib_handle_1_v1, "translation");
    current_sort = (SortFunc)dlsym(lib_handle_2_v1, "sort");
    
    if (!current_translation || !current_sort) {
        char err_msg[512];
        snprintf(err_msg, sizeof(err_msg), "Ошибка поиска символа 'translation' или 'sort': %s\n", dlerror());
        write(STDERR_FILENO, err_msg, strlen(err_msg));
        return 0;
    }

    return 1;
}

static void unload_libraries() {
    if (lib_handle_1_v1) dlclose(lib_handle_1_v1);
    if (lib_handle_1_v2) dlclose(lib_handle_1_v2);
    if (lib_handle_2_v1) dlclose(lib_handle_2_v1);
    if (lib_handle_2_v2) dlclose(lib_handle_2_v2);
}

static void switch_implementations() {
    // Переключаем реализацию перевода
    current_impl_1 = (current_impl_1 == 1) ? 2 : 1;
    void *target_handle_1 = (current_impl_1 == 1) ? lib_handle_1_v1 : lib_handle_1_v2;
    current_translation = (TranslationFunc)dlsym(target_handle_1, "translation");

    // Переключаем реализацию сортировки
    current_impl_2 = (current_impl_2 == 1) ? 2 : 1;
    void *target_handle_2 = (current_impl_2 == 1) ? lib_handle_2_v1 : lib_handle_2_v2;
    current_sort = (SortFunc)dlsym(target_handle_2, "sort");
    
    if (!current_translation || !current_sort) {
        write_str("Ошибка переключения: не удалось найти символ.\n");
        return;
    }

    write_str("--- РЕАЛИЗАЦИИ ПЕРЕКЛЮЧЕНЫ ---\n");
    
    char output[256];
    snprintf(output, sizeof(output), "Функция 1: %s\n", (current_impl_1 == 1) ? "Двоичная (V1)" : "Троичная (V2)");
    write_str(output);
    
    snprintf(output, sizeof(output), "Функция 2: %s\n", (current_impl_2 == 1) ? "Пузырьковая (V1)" : "Хоара (V2)");
    write_str(output);
}

static void handle_function_1(const char *arg_str) {
    if (!current_translation) {
        write_str("Ошибка: Функция 1 не загружена.\n");
        return;
    }

    long x;
    if (sscanf(arg_str, "%ld", &x) != 1) {
        write_str("Ошибка ввода: требуется целое число для функции 1.\n");
        return;
    }

    char *result = current_translation(x);
    
    if (result) {
        char output[256];
        snprintf(output, sizeof(output), "Результат (%s): %s\n", 
            (current_impl_1 == 1) ? "Двоичный" : "Троичный", result);
        write_str(output);
        free(result);
    } else {
        write_str("Ошибка выделения памяти.\n");
    }
}

static void handle_function_2(const char *arg_str) {
    if (!current_sort) {
        write_str("Ошибка: Функция 2 не загружена.\n");
        return;
    }

    int temp_array[101];  // +1 для завершающего нуля
    size_t count = 0;
    const char *p = arg_str;

    while (sscanf(p, "%d", &temp_array[count]) == 1) {
        count++;
        while (*p && !isspace(*p)) p++;
        while (*p && isspace(*p)) p++;
        if (count >= 100) break;
    }

    if (count == 0) {
        write_str("Ошибка ввода: требуется список целых чисел для функции 2.\n");
        return;
    }

    temp_array[count] = 0;  // Завершаем массив нулем
    
    int *array_to_sort = (int*)malloc((count + 1) * sizeof(int));
    if (!array_to_sort) {
        write_str("Ошибка выделения памяти.\n");
        return;
    }
    
    memcpy(array_to_sort, temp_array, (count + 1) * sizeof(int));
    
    write_str("Исходный массив: ");
    for(size_t i = 0; i < count; i++) {
        write_int(temp_array[i]);
        write_str(" ");
    }
    write_str("\n");

    int *sorted_array = current_sort(array_to_sort);
    
    write_str("Результат (");
    write_str((current_impl_2 == 1) ? "Пузырьковая" : "Хоара");
    write_str("): ");
    
    for(size_t i = 0; sorted_array[i] != 0; i++) {
        write_int(sorted_array[i]);
        write_str(" ");
    }
    write_str("\n");

    free(array_to_sort);
}

int main() {
    if (!load_libraries()) {
        write_str("Критическая ошибка инициализации. Выход.\n");
        unload_libraries();
        return EXIT_FAILURE;
    }

    char line[512];
    
    write_str("--- Программа 2: Динамическая загрузка ---\n");
    write_str("Начальные реализации: Ф1 - Двоичная, Ф2 - Пузырьковая.\n");
    write_str("Введите команду (0 - переключить, 1 [число], 2 [массив чисел], q - выход):\n");

    while (read_line(line, sizeof(line))) {
        if (line[0] == 'q' || line[0] == 'Q') break;
        
        int cmd = line[0] - '0';
        char *args = line + 1;
        while (*args == ' ' || *args == '\t') args++;
        
        switch (cmd) {
            case 0:
                switch_implementations();
                break;
            case 1:
                handle_function_1(args);
                break;
            case 2:
                handle_function_2(args);
                break;
            default:
                write_str("Неверная команда. Используйте 0, 1 или 2.\n");
                break;
        }
        
        write_str("\nВведите команду: ");
    }

    unload_libraries();
    return 0;
}