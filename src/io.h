#ifndef IO_H
#define IO_H

#include <stdint.h>

void io_print_int(uint32_t value);
void io_print_string(const char *str);
uint32_t io_read_int(void);

#endif
