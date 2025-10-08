#include "os_utils.h"
#include <ctype.h>

void remove_vowels(char* str) {
    if (!str) {
        return;
    }
    int write_idx = 0;
    for (int read_idx = 0; str[read_idx] != '\0'; read_idx++) {
        char c = str[read_idx];
        char lower_c = tolower((unsigned char) c);
        if (lower_c != 'a' && lower_c != 'e' && lower_c != 'i' && lower_c != 'o' && lower_c != 'u' && lower_c != 'y') {
            str[write_idx++] = c;
        }
    }
    str[write_idx] = '\0';
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        return INVALID_INPUT;
    }
    const char* output_name = argv[1];
    FILE* output = NULL;
    char buffer[1024];

    output = fopen(output_name, "w");
    if (output == NULL) {
        return FILE_OPEN_ERROR;
    }
    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        remove_vowels(buffer);
        if (fputs(buffer, output) == EOF) {
            return IO_ERROR;
        }
    }
    fclose(output);
    return STATUS_OK;
}