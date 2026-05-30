#ifndef VM_GUI_H
#define VM_GUI_H

#include <stdbool.h>
#include <stdint.h>
#include "cpu.h"
#include "memory.h"

// Initialize GUI with CPU and memory pointers. Returns true on success.
bool     gui_init(CPU *cpu, Memory *mem);

// Run the GUI event loop (blocks until window is closed).
void     gui_run(void);

// Shutdown GUI and free resources.
void     gui_shutdown(void);

// Append a line to the GUI console output panel (also prints to stdout).
void     gui_console_append(const char *line);

// Request an integer from the user (falls back to terminal stdin).
uint32_t gui_request_input(void);

#endif // VM_GUI_H
