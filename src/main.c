#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "memory.h"
#include "cpu.h"
#include "loader.h"
#include "io.h"

#ifdef BUILD_GUI
#include "gui.h"
#endif

static void usage(const char *prog) {
    fprintf(stderr,
        "Usage:\n"
        "  %s --program <file> [--gui] [--debug]\n"
        "  %s <program.bin>          (legacy CLI shorthand)\n"
        "\n"
        "Flags:\n"
        "  --program <file>  Binary program to load\n"
        "  --gui             Launch graphical interface (requires vm_gui build)\n"
        "  --headless        CLI mode (default)\n"
        "  --debug           Print each instruction as it executes\n",
        prog, prog);
}

int main(int argc, char *argv[]) {
    bool gui_mode = false;
    bool debug    = false;
    char program[256] = "";

    for (int i = 1; i < argc; i++) {
        if      (strcmp(argv[i], "--gui")      == 0) { gui_mode = true;  }
        else if (strcmp(argv[i], "--headless") == 0) { gui_mode = false; }
        else if (strcmp(argv[i], "--debug")    == 0) { debug    = true;  }
        else if (strcmp(argv[i], "--program")  == 0 && i + 1 < argc) {
            strncpy(program, argv[++i], sizeof(program) - 1);
        } else if (argv[i][0] != '-' && program[0] == '\0') {
            strncpy(program, argv[i], sizeof(program) - 1);
        } else {
            fprintf(stderr, "Unknown argument: %s\n", argv[i]);
            usage(argv[0]);
            return 1;
        }
    }


    Memory mem;
    memory_init(&mem);

    CPU cpu;
    cpu_init(&cpu, &mem);

#ifdef BUILD_GUI
    if (gui_mode) {
        if (!gui_init(&cpu, &mem)) return 1;

        // Route program I/O to the GUI console
        io_set_output_callback(gui_console_append);

        // Pre-load a program if one was given on the command line
        if (program[0]) {
            if (loader_load_file(&mem, program, 0x0000) != 0) {
                gui_console_append("[LOAD] Failed to load program.");
            } else {
                char msg[280];
                snprintf(msg, sizeof(msg), "[LOAD] OK: %s", program);
                gui_console_append(msg);
            }
        }

        gui_run();
        gui_shutdown();
        return 0;
    }
#else
    if (gui_mode) {
        fprintf(stderr,
            "Error: not built with GUI support.\n"
            "Run 'make vm_gui' and use ./vm_gui instead.\n");
        return 1;
    }
#endif

    // ── CLI / headless mode ───────────────────────────────────────────────
    if (program[0] == '\0') {
        usage(argv[0]);
        return 1;
    }

    if (loader_load_file(&mem, program, 0x0000) != 0) return 1;

    printf("[VM] Starting execution...\n\n");
    if (debug) {
        while (cpu.running) {
            char dbuf[80];
            cpu_disasm(&mem, cpu.pc, dbuf, sizeof(dbuf));
            printf("[DBG] 0x%08X: %s\n", cpu.pc, dbuf);
            cpu_step(&cpu);
        }
    } else {
        cpu_run(&cpu);
    }
    cpu_dump(&cpu);

    return 0;
}
