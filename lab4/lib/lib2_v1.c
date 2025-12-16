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

int* sort(int* array) {
    size_t n = array_size(array);
    if (array == NULL || n == 0) {
        return array;
    }

    for (size_t i = 0; i < n - 1; i++) {
        for (size_t j = 0; j < n - i - 1; j++) {
            if (array[j] > array[j + 1]) {
                int temp = array[j];
                array[j] = array[j + 1];
                array[j + 1] = temp;
            }
        }
    }
    return array;
}