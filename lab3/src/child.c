#include "os_utils.h"

void remove_vowels(char* str) {
    if (!str) {
        return;
    }
    int write_idx = 0;
    for (int read_idx = 0; str[read_idx] != '\0'; read_idx++) {
        char c = str[read_idx];
        char lower_c = tolower((unsigned char)c);
        if (lower_c != 'a' && lower_c != 'e' && lower_c != 'i' && 
            lower_c != 'o' && lower_c != 'u' && lower_c != 'y') {
            str[write_idx++] = c;
        }
    }
    str[write_idx] = '\0';
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Использование: %s <shm_name> <output_file>\n", argv[0]);
        return INVALID_INPUT;
    }
    
    const char* shm_name = argv[1];
    const char* output_name = argv[2];
    
    // Открытие разделяемой памяти
    int shm_fd = shm_open(shm_name, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return MMAP_ERROR;
    }
    
    // Отображение разделяемой памяти
    shared_data_t* shared = mmap(NULL, sizeof(shared_data_t), 
                                PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared == MAP_FAILED) {
        perror("mmap");
        close(shm_fd);
        return MMAP_ERROR;
    }
    
    // Открытие выходного файла
    FILE* output = fopen(output_name, "w");
    if (output == NULL) {
        perror("fopen");
        munmap(shared, sizeof(shared_data_t));
        close(shm_fd);
        return FILE_OPEN_ERROR;
    }
    
    printf("Дочерний процесс начал работу (SHM: %s, Output: %s)\n", shm_name, output_name);
    
    // Основной цикл обработки
    while (1) {
        if (shared->process_complete) {
            break;
        }
        
        if (shared->data_ready) {
            // Копируем данные для обработки
            char buffer[MAX_LINE_LENGTH];
            strncpy(buffer, shared->data, MAX_LINE_LENGTH - 1);
            buffer[MAX_LINE_LENGTH - 1] = '\0';
            
            // Обрабатываем строку
            remove_vowels(buffer);
            
            // Записываем результат в файл
            if (fprintf(output, "%s\n", buffer) < 0) {
                perror("fprintf");
                fclose(output);
                munmap(shared, sizeof(shared_data_t));
                close(shm_fd);
                return IO_ERROR;
            }
            fflush(output);
            
            // Сбрасываем флаг готовности данных
            shared->data_ready = 0;
        }
        
        // usleep(1000); // Небольшая пауза для уменьшения нагрузки на CPU
    }
    
    printf("Дочерний процесс завершил работу\n");
    
    // Освобождение ресурсов
    fclose(output);
    munmap(shared, sizeof(shared_data_t)); // удаление отображения
    close(shm_fd);
    
    return STATUS_OK;
}