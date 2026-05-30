#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

#define MEMORY_SIZE (1024 * 1024)  // 1MB RAM

typedef struct {
    uint8_t data[MEMORY_SIZE];
} Memory;

void memory_init(Memory *mem);
uint8_t  memory_read_byte(Memory *mem, uint32_t address);
uint16_t memory_read_word(Memory *mem, uint32_t address);
uint32_t memory_read_dword(Memory *mem, uint32_t address);
void memory_write_byte(Memory *mem, uint32_t address, uint8_t value);
void memory_write_word(Memory *mem, uint32_t address, uint16_t value);
void memory_write_dword(Memory *mem, uint32_t address, uint32_t value);

#endif
