#ifndef CONTRACT_H
#define CONTRACT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Контракт для функции перевода числа
char* translation(long x);

// Контракт для функции сортировки
int* sort(int* array);

#ifdef __cplusplus
}
#endif

#endif