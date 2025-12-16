#include "os_utils.h"

int create_shared_memory(const char* name) {
    int fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        return -1;
    }
    
    if (ftruncate(fd, sizeof(shared_data_t)) == -1) {
        perror("ftruncate");
        close(fd);
        return -1;
    }
    
    return fd;
}

int main() {
    char shm_name1[MAX_FILENAME_LENGTH];
    char shm_name2[MAX_FILENAME_LENGTH];
    char output_name1[MAX_FILENAME_LENGTH];
    char output_name2[MAX_FILENAME_LENGTH];
    
    shared_data_t *shared1, *shared2;
    int shm_fd1, shm_fd2;
    pid_t pid1, pid2;
    
    StatusCode status = STATUS_OK;
    
    srand(time(NULL));
    
    // Ввод имен для разделяемой памяти и выходных файлов
    printf("Введите имя shared memory для child1: ");
    if (fgets(shm_name1, sizeof(shm_name1), stdin) == NULL) {
        return INVALID_INPUT;
    }
    shm_name1[strcspn(shm_name1, "\n")] = '\0';
    
    printf("Введите имя файла вывода для child1: ");
    if (fgets(output_name1, sizeof(output_name1), stdin) == NULL) {
        return INVALID_INPUT;
    }
    output_name1[strcspn(output_name1, "\n")] = '\0';
    
    printf("Введите имя shared memory для child2: ");
    if (fgets(shm_name2, sizeof(shm_name2), stdin) == NULL) {
        return INVALID_INPUT;
    }
    shm_name2[strcspn(shm_name2, "\n")] = '\0';
    
    printf("Введите имя файла вывода для child2: ");
    if (fgets(output_name2, sizeof(output_name2), stdin) == NULL) {
        return INVALID_INPUT;
    }
    output_name2[strcspn(output_name2, "\n")] = '\0';
    
    // Создание разделяемой памяти
    shm_fd1 = create_shared_memory(shm_name1);
    shm_fd2 = create_shared_memory(shm_name2);
    
    if (shm_fd1 == -1 || shm_fd2 == -1) {
        return MMAP_ERROR;
    }
    
    // Отображение разделяемой памяти
    shared1 = mmap(NULL, sizeof(shared_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd1, 0);
    shared2 = mmap(NULL, sizeof(shared_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd2, 0);
    
    if (shared1 == MAP_FAILED || shared2 == MAP_FAILED) {
        perror("mmap");
        return MMAP_ERROR;
    }
    
    // Инициализация разделяемой памяти
    memset(shared1, 0, sizeof(shared_data_t));
    memset(shared2, 0, sizeof(shared_data_t));
    
    // Создание первого дочернего процесса
    pid1 = fork();
    if (pid1 == -1) {
        perror("fork");
        status = FORK_ERROR;
        goto cleanup;
    }
    
    if (pid1 == 0) {
        // Дочерний процесс 1
        execl("./child", "child", shm_name1, output_name1, NULL);
        perror("execl");
        exit(3);  // EXEC_ERROR = 3
    }
    
    // Создание второго дочернего процесса
    pid2 = fork();
    if (pid2 == -1) {
        perror("fork");
        status = FORK_ERROR;
        goto cleanup;
    }
    
    if (pid2 == 0) {
        // Дочерний процесс 2
        execl("./child", "child", shm_name2, output_name2, NULL);
        perror("execl");
        exit(3);  // EXEC_ERROR = 3
    }
    
    printf("Родительский процесс начался\n");
    printf("Введите строки (для выхода введите QUIT):\n");
    
    char buffer[MAX_LINE_LENGTH];
    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        int len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            len--;
        }
        
        if (strcmp(buffer, "QUIT") == 0) {
            break;
        }
        
        // Вероятностная отправка (80% - child1, 20% - child2)
        int random_percent = rand() % 100;
        shared_data_t* target_shared;
        const char* target_name;
        
        if (random_percent < 80) {
            target_shared = shared1;
            target_name = "Child1";
        } else {
            target_shared = shared2;
            target_name = "Child2";
        }
        
        printf("Отправлено в %s (вероятность: %d%%)\n", target_name, random_percent);
        
        // Копирование данных в разделяемую память
        strncpy(target_shared->data, buffer, MAX_LINE_LENGTH - 1);
        target_shared->data_ready = 1; // поднимает флаг "данные готовы"
        
        // Ожидание обработки дочерним процессом
        while (target_shared->data_ready == 1) {
            // usleep(1000); // Небольшая пауза
        }
    }
    
    // Сигнал дочерним процессам о завершении
    shared1->process_complete = 1;
    shared2->process_complete = 1;
    
    // Ожидание завершения дочерних процессов
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    
    printf("Родительский и дочерние процессы успешно завершены\n");

cleanup:
    // Освобождение ресурсов
    if (shared1 != MAP_FAILED) munmap(shared1, sizeof(shared_data_t));
    if (shared2 != MAP_FAILED) munmap(shared2, sizeof(shared_data_t));
    if (shm_fd1 != -1) {
        close(shm_fd1);
        shm_unlink(shm_name1); // удаление объекта
    }
    if (shm_fd2 != -1) {
        close(shm_fd2);
        shm_unlink(shm_name2);
    }
    
    return status;
}