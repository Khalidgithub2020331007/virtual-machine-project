#include <stdio.h>
#include <stdlib.h>
#include "memory.h"
#include "cpu.h"
#include "loader.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <program.bin>\n", argv[0]);
        return 1;
    }

    // Initialize memory
    Memory mem;
    memory_init(&mem);

    // Load program into memory starting at address 0
    if (loader_load_file(&mem, argv[1], 0x0000) != 0) {
        return 1;
    }

    // Initialize CPU
    CPU cpu;
    cpu_init(&cpu, &mem);

    printf("[VM] Starting execution...\n\n");

    // Run the VM
    cpu_run(&cpu);

    // Dump final CPU state
    cpu_dump(&cpu);

    return 0;
}
