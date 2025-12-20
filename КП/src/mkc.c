#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>

#define PAGE_SIZE 4096U                 // Размер страницы памяти (4KB)
#define MKC_PAGE_FREE  0xFFFFu          // Индикатор свободной страницы
#define MKC_PAGE_LARGE 0xFFFEu          // Индикатор страницы с большим блоком

// Классы размеров для алгоритма McKusick-Karels (степени двойки)
static const size_t mkc_classes[] = {
    16, 32, 64, 128, 256, 512, 1024, 2048
};
#define MKC_NUM_CLASSES (sizeof(mkc_classes)/sizeof(mkc_classes[0]))

// Структура страницы в алгоритме McKusick-Karels
typedef struct MKC_Page {
    uint16_t size_class_index;   // Индекс класса размера или специальный флаг
    uint16_t free_count;         // Количество свободных слотов в странице
    struct MKC_Page* next;       // Указатель на следующую страницу в списке
    uint32_t bitmap[8];          // Битовая карта свободных слотов (256 бит)
} MKC_Page;

// Структура аллокатора McKusick-Karels
typedef struct {
    size_t total_size;           // Общий размер выделенной памяти
    void* base_data;             // Указатель на начало области данных
    size_t pages_count;          // Количество страниц в области данных

    MKC_Page* free_pages;        // Список полностью свободных страниц
    MKC_Page* class_pages[MKC_NUM_CLASSES];  // Списки страниц по классам
    MKC_Page* large_pages;       // Список страниц с большими блоками
} MKCAllocator;

// Получение указателя на страницу по её индексу
static inline MKC_Page* page_at(MKCAllocator* alloc, size_t index) {
    return (MKC_Page*)((char*)alloc->base_data + index * PAGE_SIZE);
}

// Очистка битовой карты (все биты устанавливаются в 0)
static inline void bitmap_clear(uint32_t* bitmap) {
    memset(bitmap, 0, sizeof(uint32_t) * 8);
}

// Поиск первого свободного бита в битовой карте
static inline int bitmap_find_free(uint32_t* bitmap, int limit) {
    for (int i = 0; i < limit; i++) {
        // Проверяем, свободен ли i-й бит (0 - свободен, 1 - занят)
        if (!(bitmap[i >> 5] & (1u << (i & 31)))) {
            return i;
        }
    }
    return -1;
}

// Вычисление максимального количества слотов в странице для заданного класса
static inline int max_slots_in_class(int class_index) {
    // Вычисляем сколько слотов помещается в странице (минус заголовок)
    int slots = (PAGE_SIZE - sizeof(MKC_Page)) / mkc_classes[class_index];
    // Ограничиваем 256 слотами для использования 256-битной битовой карты
    return slots > 256 ? 256 : slots;
}

// Определение индекса класса для запрошенного размера
static inline int find_class_index(size_t size) {
    for (int i = 0; i < MKC_NUM_CLASSES; i++) {
        if (size <= mkc_classes[i])
            return i;
    }
    return -1;  // Размер больше максимального класса
}

// Извлечение свободной страницы из списка свободных
static MKC_Page* take_free_page(MKCAllocator* alloc) {
    MKC_Page* page = alloc->free_pages;
    if (!page) return NULL;
    alloc->free_pages = page->next;
    page->next = NULL;
    bitmap_clear(page->bitmap);
    return page;
}

// Возвращение страницы в список свободных
static void return_free_page(MKCAllocator* alloc, MKC_Page* page) {
    page->size_class_index = MKC_PAGE_FREE;
    page->free_count = 0;
    bitmap_clear(page->bitmap);
    page->next = alloc->free_pages;
    alloc->free_pages = page;
}

// Создание аллокатора McKusick-Karels
MKCAllocator* createMKCAllocator(size_t size) {
    // Минимальный размер - 2 страницы (одна для структуры, одна для данных)
    if (size < PAGE_SIZE * 2)
        return NULL;
    
    // Выделяем память с помощью mmap
    void* memory = mmap(NULL, size, PROT_READ | PROT_WRITE, 
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    if (memory == MAP_FAILED)
        return NULL;
    
    // Используем первую страницу для структуры аллокатора
    MKCAllocator* alloc = (MKCAllocator*)memory;
    memset(alloc, 0, sizeof(MKCAllocator));
    
    alloc->total_size = size;
    alloc->base_data = (char*)memory + PAGE_SIZE;  // Данные начинаются со второй страницы
    alloc->pages_count = (size - PAGE_SIZE) / PAGE_SIZE;
    
    // Инициализируем все страницы как свободные
    for (size_t i = 0; i < alloc->pages_count; i++) {
        MKC_Page* page = page_at(alloc, i);
        page->size_class_index = MKC_PAGE_FREE;
        bitmap_clear(page->bitmap);
        page->next = alloc->free_pages;
        alloc->free_pages = page;
    }
    
    return alloc;
}

// Уничтожение аллокатора McKusick-Karels
void destroyMKCAllocator(MKCAllocator* alloc) {
    if (!alloc) return;
    munmap(alloc, alloc->total_size);
}

// Выделение памяти с помощью алгоритма McKusick-Karels
void* mkc_alloc(MKCAllocator* alloc, size_t size) {
    if (!alloc || size == 0)
        return NULL;

    // Определяем класс для запрошенного размера
    int class_idx = find_class_index(size);

    // Если размер попадает в один из классов (мелкий блок)
    if (class_idx >= 0) {
        // Ищем страницу с данным классом, в которой есть свободные слоты
        MKC_Page* page = alloc->class_pages[class_idx];
        while (page && page->free_count == 0)
            page = page->next;

        // Если не нашли подходящей страницы, берем новую из свободных
        if (!page) {
            page = take_free_page(alloc);
            if (!page) return NULL;
            
            // Инициализируем страницу для данного класса
            page->size_class_index = class_idx;
            page->free_count = max_slots_in_class(class_idx);
            page->next = alloc->class_pages[class_idx];
            alloc->class_pages[class_idx] = page;
        }

        // Находим свободный слот в битовой карте
        int slot = bitmap_find_free(page->bitmap, max_slots_in_class(class_idx));
        if (slot < 0) return NULL;

        // Помечаем слот как занятый
        page->bitmap[slot >> 5] |= 1u << (slot & 31);
        page->free_count--;

        // Вычисляем адрес выделенного слота
        return (char*)page + sizeof(MKC_Page) + slot * mkc_classes[class_idx];
    }

    // Большой блок (не помещается ни в один класс)
    size_t pages_needed = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    size_t consecutive_free = 0;
    size_t start_index = 0;
    int found = 0;

    // Ищем последовательность свободных страниц
    for (size_t i = 0; i < alloc->pages_count; i++) {
        MKC_Page* page = page_at(alloc, i);
        if (page->size_class_index == MKC_PAGE_FREE) {
            if (!consecutive_free) start_index = i;
            if (++consecutive_free == pages_needed) {
                found = 1;
                break;
            }
        } else {
            consecutive_free = 0;
        }
    }

    if (!found) return NULL;

    // Помечаем найденные страницы как занятые для большого блока
    MKC_Page* first_page = page_at(alloc, start_index);
    first_page->size_class_index = MKC_PAGE_LARGE;
    first_page->free_count = pages_needed;

    for (size_t i = 1; i < pages_needed; i++) {
        page_at(alloc, start_index + i)->size_class_index = MKC_PAGE_LARGE;
    }

    // Удаляем эти страницы из списка свободных
    MKC_Page** prev_ptr = &alloc->free_pages;
    while (*prev_ptr) {
        MKC_Page* current = *prev_ptr;
        size_t idx = ((char*)current - (char*)alloc->base_data) / PAGE_SIZE;
        
        if (idx >= start_index && idx < start_index + pages_needed) {
            // Пропускаем страницу, так как она теперь используется
            *prev_ptr = current->next;
        } else {
            prev_ptr = &current->next;
        }
    }

    // Добавляем первую страницу блока в список больших блоков
    first_page->next = alloc->large_pages;
    alloc->large_pages = first_page;

    return (char*)first_page + sizeof(MKC_Page);
}

// Освобождение памяти в алгоритме McKusick-Karels
void mkc_free(MKCAllocator* alloc, void* ptr) {
    if (!alloc || !ptr)
        return;

    // Определяем, к какой странице принадлежит указатель
    size_t offset = (char*)ptr - (char*)alloc->base_data;
    size_t page_index = offset / PAGE_SIZE;
    
    if (page_index >= alloc->pages_count)
        return;

    MKC_Page* page = page_at(alloc, page_index);

    // Обработка большого блока
    if (page->size_class_index == MKC_PAGE_LARGE) {
        size_t pages_count = page->free_count;
        
        // Освобождаем все страницы большого блока
        for (size_t i = 0; i < pages_count; i++) {
            return_free_page(alloc, page_at(alloc, page_index + i));
        }
        
        // Удаляем страницу из списка больших блоков
        MKC_Page** prev_ptr = &alloc->large_pages;
        while (*prev_ptr) {
            if (*prev_ptr == page) {
                *prev_ptr = page->next;
                break;
            }
            prev_ptr = &(*prev_ptr)->next;
        }
        return;
    }

    // Обработка мелкого блока
    int class_idx = page->size_class_index;
    size_t class_size = mkc_classes[class_idx];
    
    // Вычисляем номер слота в странице
    size_t slot = ((char*)ptr - ((char*)page + sizeof(MKC_Page))) / class_size;

    // Проверяем, что слот действительно был занят
    if (!(page->bitmap[slot >> 5] & (1u << (slot & 31))))
        return;

    // Освобождаем слот (устанавливаем бит в 0)
    page->bitmap[slot >> 5] &= ~(1u << (slot & 31));
    page->free_count++;

    // Если страница полностью освободилась, возвращаем её в пул свободных
    if (page->free_count == max_slots_in_class(class_idx)) {
        // Удаляем страницу из списка её класса
        MKC_Page** prev_ptr = &alloc->class_pages[class_idx];
        while (*prev_ptr) {
            if (*prev_ptr == page) {
                *prev_ptr = page->next;
                break;
            }
            prev_ptr = &(*prev_ptr)->next;
        }
        
        // Возвращаем страницу в пул свободных
        return_free_page(alloc, page);
    }
}

// Получение количества свободной памяти
size_t mkc_free_memory(MKCAllocator* alloc) {
    if (!alloc) return 0;

    size_t free_memory = 0;
    
    // Считаем полностью свободные страницы
    for (MKC_Page* page = alloc->free_pages; page; page = page->next)
        free_memory += PAGE_SIZE;

    // Считаем свободные слоты в занятых страницах
    for (size_t i = 0; i < MKC_NUM_CLASSES; i++) {
        MKC_Page* page = alloc->class_pages[i];
        while (page) {
            free_memory += page->free_count * mkc_classes[i];
            page = page->next;
        }
    }
    
    return free_memory;
}