#define _GNU_SOURCE
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <stdio.h>

// Минимальный размер блока (16 байт данных + заголовок)
#define MIN_BLOCK_SIZE 16
#define HEADER_SIZE sizeof(BlockHeader)

// Структура заголовка блока памяти
typedef struct BlockHeader {
    size_t size;            // Размер данных в блоке (без учета заголовка)
    int is_free;            // Флаг: 1 - свободен, 0 - занят
    struct BlockHeader* next;  // Указатель на следующий блок в списке
    struct BlockHeader* prev;  // Указатель на предыдущий блок
} BlockHeader;

// Структура аллокатора Best Fit
typedef struct {
    void* base;             // Указатель на начало выделенной памяти
    size_t total_size;      // Общий размер выделенной памяти
    BlockHeader* free_list; // Список свободных блоков
    BlockHeader* used_list; // Список занятых блоков (для быстрого доступа)
} BestFitAllocator;

// Функция выравнивания размера до границы слова
static inline size_t align_size(size_t size) {
    return (size + sizeof(void*) - 1) & ~(sizeof(void*) - 1);
}

// Удаление блока из списка (оптимизированная версия)
static void remove_block(BlockHeader** list, BlockHeader* block) {
    if (block->prev) {
        block->prev->next = block->next;
    } else {
        *list = block->next;
    }
    
    if (block->next) {
        block->next->prev = block->prev;
    }
}

// Вставка блока в начало списка
static void insert_front(BlockHeader** list, BlockHeader* block) {
    block->next = *list;
    block->prev = NULL;
    
    if (*list) {
        (*list)->prev = block;
    }
    
    *list = block;
}

// Создание аллокатора Best Fit
int createBestFitAllocator(BestFitAllocator* alloc, size_t size) {
    if (!alloc || size < MIN_BLOCK_SIZE * 4)
        return 0;

    // Выделяем память с помощью mmap
    void* memory = mmap(NULL, size,
                       PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS,
                       -1, 0);

    if (memory == MAP_FAILED)
        return 0;

    // Инициализируем структуру аллокатора
    alloc->base = memory;
    alloc->total_size = size;
    
    // Создаем первый свободный блок на всей выделенной памяти
    BlockHeader* first_block = (BlockHeader*)memory;
    first_block->size = size - HEADER_SIZE;
    first_block->is_free = 1;
    first_block->next = NULL;
    first_block->prev = NULL;
    
    alloc->free_list = first_block;
    alloc->used_list = NULL;
    
    return 1;
}

// Выделение памяти по алгоритму Best Fit (наиболее подходящий блок)
void* best_fit_alloc(BestFitAllocator* alloc, size_t size) {
    if (!alloc || size == 0)
        return NULL;
    
    // Выравниваем запрошенный размер и добавляем место под заголовок
    size_t needed_size = align_size(size) + HEADER_SIZE;
    
    // Поиск наиболее подходящего блока (с минимальным достаточным размером)
    BlockHeader* best_block = NULL;
    BlockHeader* current = alloc->free_list;
    size_t best_block_size = SIZE_MAX;
    
    while (current) {
        if (current->is_free && current->size >= needed_size) {
            // Находим блок с наиболее подходящим размером
            if (current->size < best_block_size) {
                best_block = current;
                best_block_size = current->size;
            }
        }
        current = current->next;
    }
    
    if (!best_block)
        return NULL;
    
    // Удаляем найденный блок из списка свободных
    remove_block(&alloc->free_list, best_block);
    
    // Проверяем, можно ли разделить блок на две части
    if (best_block->size >= needed_size + HEADER_SIZE + MIN_BLOCK_SIZE) {
        // Вычисляем размер остатка после выделения запрошенной памяти
        size_t remaining_size = best_block->size - needed_size;
        
        // Создаем новый свободный блок из остатка
        BlockHeader* new_free_block = (BlockHeader*)((char*)best_block + needed_size);
        
        new_free_block->size = remaining_size - HEADER_SIZE;
        new_free_block->is_free = 1;
        new_free_block->next = NULL;
        new_free_block->prev = NULL;
        
        // Вставляем новый свободный блок в начало списка свободных
        insert_front(&alloc->free_list, new_free_block);
        
        // Обновляем размер выделенного блока
        best_block->size = needed_size - HEADER_SIZE;
    }
    
    // Помечаем блок как занятый
    best_block->is_free = 0;
    
    // Добавляем блок в список занятых
    insert_front(&alloc->used_list, best_block);
    
    // Возвращаем указатель на данные (пропускаем заголовок)
    return (char*)best_block + HEADER_SIZE;
}

// Объединение соседних свободных блоков (оптимизированная версия)
static void coalesce_free_blocks(BestFitAllocator* alloc) {
    BlockHeader* current = alloc->free_list;
    
    while (current && current->next) {
        // Проверяем, являются ли блоки физически соседними в памяти
        if ((char*)current + HEADER_SIZE + current->size == (char*)current->next) {
            // Объединяем текущий блок со следующим
            current->size += HEADER_SIZE + current->next->size;
            
            // Удаляем следующий блок из списка
            current->next = current->next->next;
            if (current->next)
                current->next->prev = current;
            
            // Не переходим к следующему блоку, так как текущий блок увеличился
            // и может быть объединен с новым соседом
        } else {
            current = current->next;
        }
    }
}

// Освобождение памяти (оптимизированная версия)
void best_fit_free(BestFitAllocator* alloc, void* ptr) {
    if (!alloc || !ptr)
        return;
    
    // Получаем указатель на заголовок блока
    BlockHeader* block = (BlockHeader*)((char*)ptr - HEADER_SIZE);
    
    // Проверяем, не освобожден ли блок уже
    if (block->is_free)
        return;
    
    // Удаляем блок из списка занятых
    remove_block(&alloc->used_list, block);
    
    // Помечаем блок как свободный
    block->is_free = 1;
    
    // Добавляем блок в начало списка свободных
    insert_front(&alloc->free_list, block);
    
    // Периодически выполняем объединение (не каждый раз!)
    static int free_count = 0;
    if (++free_count % 1000 == 0) {
        coalesce_free_blocks(alloc);
    }
}

// Получение количества свободной памяти
size_t best_fit_free_memory(BestFitAllocator* alloc) {
    if (!alloc)
        return 0;
    
    size_t free_memory = 0;
    BlockHeader* current = alloc->free_list;
    
    // Суммируем размеры всех свободных блоков
    while (current) {
        free_memory += current->size + HEADER_SIZE;
        current = current->next;
    }
    
    return free_memory;
}

// Уничтожение аллокатора и освобождение памяти
void destroyBestFitAllocator(BestFitAllocator* alloc) {
    if (!alloc)
        return;
    
    // Освобождаем выделенную память
    munmap(alloc->base, alloc->total_size);
    
    // Обнуляем указатели
    alloc->base = NULL;
    alloc->total_size = 0;
    alloc->free_list = NULL;
    alloc->used_list = NULL;
}