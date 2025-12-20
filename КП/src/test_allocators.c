#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Подключаем реализации аллокаторов
#include "best_fit.c"
#include "mkc.c"

#define MEMORY_SIZE (4 * 1024 * 1024)   // 4 МБ памяти
#define NUM_OPERATIONS 100000           // Количество операций для теста
#define MAX_BLOCK_SIZE 128              // Максимальный размер блока

// Функция для точного измерения времени
double get_current_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

// Тестирование аллокатора Best Fit
void test_best_fit() {
    printf("=== Тестирование Best Fit аллокатора ===\n");

    BestFitAllocator alloc;
    if (!createBestFitAllocator(&alloc, MEMORY_SIZE)) {
        printf("Ошибка инициализации Best Fit аллокатора\n");
        return;
    }

    // Массив для хранения указателей на выделенные блоки
    void** pointers = malloc(NUM_OPERATIONS * sizeof(void*));

    // Тест скорости выделения памяти
    double start_time = get_current_time();
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        size_t block_size = rand() % MAX_BLOCK_SIZE + 1;
        pointers[i] = best_fit_alloc(&alloc, block_size);
    }
    double alloc_time = get_current_time() - start_time;
    printf("Выделение %d блоков: %.6f секунд\n", NUM_OPERATIONS, alloc_time);

    // Тест скорости освобождения памяти
    start_time = get_current_time();
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        best_fit_free(&alloc, pointers[i]);
    }
    double free_time = get_current_time() - start_time;
    printf("Освобождение %d блоков: %.6f секунд\n", NUM_OPERATIONS, free_time);

    // Освобождаем аллокатор и создаём заново для теста использования памяти
    destroyBestFitAllocator(&alloc);
    if (!createBestFitAllocator(&alloc, MEMORY_SIZE)) {
        printf("Ошибка переинициализации Best Fit аллокатора\n");
        free(pointers);
        return;
    }

    // Тест эффективности использования памяти
    size_t total_requested = 0;
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        size_t block_size = rand() % MAX_BLOCK_SIZE + 1;
        pointers[i] = best_fit_alloc(&alloc, block_size);
        if (pointers[i]) {
            total_requested += block_size;
        }
    }

    // Вычисление фактора использования памяти
    size_t free_memory = best_fit_free_memory(&alloc);
    double used_memory = (double)(MEMORY_SIZE - free_memory);
    double utilization = used_memory > 0 ? 
        (double)total_requested / used_memory * 100.0 : 0.0;

    printf("Фактор использования памяти: %.2f%%\n\n", utilization);

    // Освобождение всех блоков
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        best_fit_free(&alloc, pointers[i]);
    }

    destroyBestFitAllocator(&alloc);
    free(pointers);
}

// Тестирование аллокатора McKusick-Karels
void test_mkc() {
    printf("=== Тестирование McKusick-Karels аллокатора ===\n");

    MKCAllocator* alloc = createMKCAllocator(MEMORY_SIZE);
    if (!alloc) {
        printf("Ошибка инициализации MKC аллокатора\n");
        return;
    }

    void** pointers = malloc(NUM_OPERATIONS * sizeof(void*));

    // Тест скорости выделения памяти
    double start_time = get_current_time();
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        size_t block_size = rand() % MAX_BLOCK_SIZE + 1;
        pointers[i] = mkc_alloc(alloc, block_size);
    }
    double alloc_time = get_current_time() - start_time;
    printf("Выделение %d блоков: %.6f секунд\n", NUM_OPERATIONS, alloc_time);

    // Тест скорости освобождения памяти
    start_time = get_current_time();
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        mkc_free(alloc, pointers[i]);
    }
    double free_time = get_current_time() - start_time;
    printf("Освобождение %d блоков: %.6f секунд\n", NUM_OPERATIONS, free_time);

    // Освобождаем аллокатор и создаём заново для теста использования памяти
    destroyMKCAllocator(alloc);
    alloc = createMKCAllocator(MEMORY_SIZE);
    if (!alloc) {
        printf("Ошибка переинициализации MKC аллокатора\n");
        free(pointers);
        return;
    }

    // Тест эффективности использования памяти
    size_t total_requested = 0;
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        size_t block_size = rand() % MAX_BLOCK_SIZE + 1;
        pointers[i] = mkc_alloc(alloc, block_size);
        if (pointers[i]) {
            total_requested += block_size;
        }
    }

    // Вычисление фактора использования памяти
    size_t free_memory = mkc_free_memory(alloc);
    double used_memory = (double)(MEMORY_SIZE - free_memory);
    double utilization = used_memory > 0 ? 
        (double)total_requested / used_memory * 100.0 : 0.0;

    printf("Фактор использования памяти: %.2f%%\n\n", utilization);

    // Освобождение всех блоков
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        mkc_free(alloc, pointers[i]);
    }

    destroyMKCAllocator(alloc);
    free(pointers);
}

int main() {
    srand(1234567);  // Фиксированный seed для воспроизводимости результатов
    
    printf("Сравнение аллокаторов памяти\n");
    printf("============================\n\n");
    
    test_best_fit();
    test_mkc();
    
    return 0;
}