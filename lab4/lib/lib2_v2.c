#include "../include/contract.h"
#include <string.h>

// Находим размер массива (завершается 0)
static size_t array_size(int* array) {
    size_t size = 0;
    while (array[size] != 0) {
        size++;
    }
    return size;
}

static void swap(int *a, int *b) {
    int t = *a;
    *a = *b;
    *b = t;
}

static size_t partition(int *array, size_t low, size_t high) {
    int pivot = array[high]; 
    size_t i = low; 

    for (size_t j = low; j < high; j++) {
        if (array[j] <= pivot) {
            swap(&array[i], &array[j]);
            i++; 
        }
    }
    swap(&array[i], &array[high]);
    return i;
}

static void quick_sort_recursive(int *array, size_t low, size_t high) {
    if (low < high) {
        size_t pi = partition(array, low, high);

        if (pi > 0) quick_sort_recursive(array, low, pi - 1);
        quick_sort_recursive(array, pi + 1, high);
    }
}

int* sort(int* array) {
    size_t n = array_size(array);
    if (array == NULL || n < 2) return array;
    quick_sort_recursive(array, 0, n - 1);
    return array;
}