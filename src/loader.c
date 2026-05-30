#include <stdio.h>
#include "loader.h"

int loader_load_file(Memory *mem, const char *filename, uint32_t start_address) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "[ERROR] Cannot open program file: %s\n", filename);
        return -1;
    }

    uint32_t address = start_address;
    int byte;
    while ((byte = fgetc(f)) != EOF) {
        if (address >= MEMORY_SIZE) {
            fprintf(stderr, "[ERROR] Program too large for memory\n");
            fclose(f);
            return -1;
        }
        memory_write_byte(mem, address++, (uint8_t)byte);
    }

    fclose(f);
    printf("[Loader] Program loaded at 0x%08X (%u bytes)\n", start_address, address - start_address);
    return 0;
}
