#include <stdio.h>
#include "io.h"

void io_print_int(uint32_t value) {
    printf("%u\n", value);
}

void io_print_string(const char *str) {
    printf("%s\n", str);
}

uint32_t io_read_int(void) {
    uint32_t val = 0;
    printf("Input: ");
    scanf("%u", &val);
    return val;
}
