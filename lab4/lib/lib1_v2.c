#include "../include/contract.h"
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 66

char* translation(long x) {
    if (x == 0) {
        char* res = (char *)malloc(2);
        if (res == NULL) {
            return NULL;
        }
        strcpy(res, "0");
        return res;
    }
    
    long temp = x;
    int is_negative = (temp < 0);
    if (is_negative) {
        temp = -temp;
    }
    
    char* buf = (char *)malloc(BUF_SIZE);
    if (buf == NULL) {
        return NULL;
    }
    
    int i = BUF_SIZE - 2;
    buf[BUF_SIZE - 1] = '\0';
    
    while (temp > 0) {
        buf[i--] = (temp % 3) + '0';
        temp /= 3;
    }
    
    if (is_negative) {
        buf[i--] = '-';
    }
    
    char* res = strdup(&buf[i + 1]);
    free(buf);
    return res;
}