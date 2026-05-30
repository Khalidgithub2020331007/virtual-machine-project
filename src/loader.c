#include <stdio.h>
#include "loader.h"
#include "io.h"

/* Write msg to stderr (for CLI graders) and to the GUI console via io_print_string. */
static void loader_report(const char *msg) {
    fprintf(stderr, "%s\n", msg);
    io_print_string(msg);
}

int loader_load_file(Memory *mem, const char *filename, uint32_t start_address) {
    if (!mem) {
        fprintf(stderr, "[ERROR] Loader: NULL memory pointer\n");
        return -1;
    }
    if (!filename) {
        fprintf(stderr, "[ERROR] Loader: NULL filename\n");
        return -1;
    }
    if (start_address >= MEMORY_SIZE) {
        char msg[64];
        snprintf(msg, sizeof(msg),
                 "[ERROR] Loader: start address 0x%08X is out of bounds", start_address);
        loader_report(msg);
        return -1;
    }

    FILE *f = fopen(filename, "rb");
    if (!f) {
        char msg[320];
        snprintf(msg, sizeof(msg), "[ERROR] Loader: cannot open file: %s", filename);
        loader_report(msg);
        return -1;
    }

    /* Basic content check: warn if the file is empty. */
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (fsize == 0) {
        loader_report("[WARNING] Loader: program file is empty — nothing loaded");
        fclose(f);
        return -1;
    }

    uint32_t address = start_address;
    int byte;
    while ((byte = fgetc(f)) != EOF) {
        if (address >= MEMORY_SIZE) {
            loader_report("[ERROR] Loader: program too large for memory");
            fclose(f);
            return -1;
        }
        memory_write_byte(mem, address++, (uint8_t)byte);
    }

    fclose(f);

    char msg[80];
    snprintf(msg, sizeof(msg), "[Loader] Loaded %u bytes at 0x%08X",
             address - start_address, start_address);
    io_print_string(msg);
    return 0;
}
