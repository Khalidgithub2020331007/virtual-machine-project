#include <stdio.h>
#include <string.h>
#include "assembler.h"

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: %s <input.asm> [output.bin]\n", argv[0]);
        return 1;
    }
    const char *src = argv[1];

    char dst[256];
    if (argc == 3) {
        strncpy(dst, argv[2], sizeof(dst) - 1);
        dst[sizeof(dst) - 1] = '\0';
    } else {
        strncpy(dst, src, sizeof(dst) - 1);
        dst[sizeof(dst) - 1] = '\0';
        char *dot = strrchr(dst, '.');
        if (dot) strcpy(dot, ".bin");
        else     strncat(dst, ".bin", sizeof(dst) - strlen(dst) - 1);
    }

    return assemble(src, dst);
}
