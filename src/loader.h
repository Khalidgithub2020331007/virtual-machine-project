#ifndef LOADER_H
#define LOADER_H

#include <stdint.h>
#include "memory.h"

int loader_load_file(Memory *mem, const char *filename, uint32_t start_address);

#endif
