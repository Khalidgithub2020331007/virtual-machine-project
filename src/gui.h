// Minimal GUI header for VM (SDL2)
#ifndef VM_GUI_H
#define VM_GUI_H

#include <stdbool.h>

// Initialize GUI. Returns true on success.
bool gui_init(int argc, char **argv);

// Run the GUI event loop (non-blocking control handled by backend callbacks).
void gui_run(void);

// Shutdown GUI and free resources.
void gui_shutdown(void);

#endif // VM_GUI_H
