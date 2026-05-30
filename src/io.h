#ifndef IO_H
#define IO_H

#include <stdint.h>

// Optional callback — when set, io_print_* sends output here instead of stdout.
// Used by the GUI to route program output to the console panel.
typedef void (*IOOutputFn)(const char *line);
void io_set_output_callback(IOOutputFn fn);

void     io_print_int(uint32_t value);
void     io_print_string(const char *str);
uint32_t io_read_int(void);

#endif
