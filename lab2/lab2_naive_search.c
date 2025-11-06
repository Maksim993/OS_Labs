#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int MAX_THREADS = 4;

typedef struct {
    int id;
    char *text;
    char *pattern;
    int start;
    int end;
    int pattern_len;
    int *results;
    int *result_count;
    pthread_mutex_t *mutex;
} ThreadData;

pthread_t* threads;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void naive_search(const char *text, const char *pattern, int start, int end, 
                  int pattern_len, int *positions, int *count) {
    for (int i = start; i <= end - pattern_len; i++) {
        int j;
        for (j = 0; j < pattern_len; j++) {
            if (text[i + j] != pattern[j]) break;
        }
        if (j == pattern_len) {
            positions[*count] = i;
            (*count)++;
        }
    }
}

void* thread_function(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    
    // Локальные переменные (у каждого потока свои)
    int local_count = 0;
    int *local_positions = malloc(sizeof(int) * (data->end - data->start));
    
    // Ищем в своей области
    naive_search(data->text, data->pattern, data->start, data->end, 
                data->pattern_len, local_positions, &local_count);
    
    pthread_mutex_lock(data->mutex);
    for (int i = 0; i < local_count; i++) {
        data->results[*data->result_count] = local_positions[i];
        (*data->result_count)++;
    }
    pthread_mutex_unlock(data->mutex);
    
    free(local_positions);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <max_threads> <text> <pattern>\n", argv[0]);
        return 1;
    }
    
    MAX_THREADS = atoi(argv[1]);
    char *text = argv[2];
    char *pattern = argv[3];
    
    int text_len = strlen(text);
    int pattern_len = strlen(pattern);
    
    if (pattern_len > text_len) {
        printf("Pattern longer than text\n");
        return 1;
    }
    
    if (MAX_THREADS <= 0) {
        printf("Thread count must be positive\n");
        return 1;
    }
    
    int actual_threads = (text_len - pattern_len + 1 < MAX_THREADS) ? 
                        text_len - pattern_len + 1 : MAX_THREADS;
    
    threads = malloc(sizeof(pthread_t) * actual_threads);
    ThreadData *thread_data = malloc(sizeof(ThreadData) * actual_threads);
    
    int *results = malloc(sizeof(int) * text_len);
    int result_count = 0;
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // Распределение работы между потоками
    // находим базовый размер порции каждого потока
    // и остаток, который распределяется первыми потоками
    int chunk_size = (text_len - pattern_len + 1) / actual_threads;
    int remainder = (text_len - pattern_len + 1) % actual_threads;
    
    int current_start = 0;
    for (int i = 0; i < actual_threads; i++) {
        thread_data[i].id = i;
        thread_data[i].text = text;
        thread_data[i].pattern = pattern;
        thread_data[i].start = current_start;
        
        int chunk = chunk_size + (i < remainder ? 1 : 0);
        thread_data[i].end = current_start + chunk + pattern_len - 1;
        if (thread_data[i].end > text_len) thread_data[i].end = text_len;
        
        thread_data[i].pattern_len = pattern_len;
        thread_data[i].results = results;
        thread_data[i].result_count = &result_count;
        thread_data[i].mutex = &mutex;
        
        current_start += chunk;
        
        pthread_create(&threads[i], NULL, thread_function, &thread_data[i]);
    }
    
    for (int i = 0; i < actual_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_sec = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    
    printf("Found: %d\n", result_count);
    for (int i = 0; i < result_count; i++) {
        printf("%d ", results[i]);
    }
    printf("\n");
    
    printf("Time: %lf seconds\n", time_sec);
    printf("Threads used: %d (max: %d)\n", actual_threads, MAX_THREADS);
    printf("Process PID: %d\n", getpid());
    
    free(threads);
    free(thread_data);
    free(results);
    return 0;
}