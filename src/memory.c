#include <stdio.h>
#include <string.h>
#include "memory.h"

void memory_init(Memory *mem) {
    if (!mem) return;
    memset(mem->data, 0, MEMORY_SIZE);
    mem->error = false;
}

uint8_t memory_read_byte(Memory *mem, uint32_t address) {
    if (!mem) { fprintf(stderr, "[ERROR] memory_read_byte: NULL pointer\n"); return 0; }
    if (address >= MEMORY_SIZE) {
        fprintf(stderr, "[ERROR] Memory read out of bounds: 0x%08X\n", address);
        mem->error = true;
        return 0;
    }
    return mem->data[address];
}

uint16_t memory_read_word(Memory *mem, uint32_t address) {
    if (!mem) { fprintf(stderr, "[ERROR] memory_read_word: NULL pointer\n"); return 0; }
    if (address + 1 >= MEMORY_SIZE) {
        fprintf(stderr, "[ERROR] Memory read out of bounds: 0x%08X\n", address);
        mem->error = true;
        return 0;
    }
    return (uint16_t)(mem->data[address]) | ((uint16_t)(mem->data[address + 1]) << 8);
}

uint32_t memory_read_dword(Memory *mem, uint32_t address) {
    if (!mem) { fprintf(stderr, "[ERROR] memory_read_dword: NULL pointer\n"); return 0; }
    if (address + 3 >= MEMORY_SIZE) {
        fprintf(stderr, "[ERROR] Memory read out of bounds: 0x%08X\n", address);
        mem->error = true;
        return 0;
    }
    return (uint32_t)(mem->data[address])
         | ((uint32_t)(mem->data[address + 1]) << 8)
         | ((uint32_t)(mem->data[address + 2]) << 16)
         | ((uint32_t)(mem->data[address + 3]) << 24);
}

void memory_write_byte(Memory *mem, uint32_t address, uint8_t value) {
    if (!mem) { fprintf(stderr, "[ERROR] memory_write_byte: NULL pointer\n"); return; }
    if (address >= MEMORY_SIZE) {
        fprintf(stderr, "[ERROR] Memory write out of bounds: 0x%08X\n", address);
        mem->error = true;
        return;
    }
    mem->data[address] = value;
}

void memory_write_word(Memory *mem, uint32_t address, uint16_t value) {
    if (!mem) { fprintf(stderr, "[ERROR] memory_write_word: NULL pointer\n"); return; }
    if (address + 1 >= MEMORY_SIZE) {
        fprintf(stderr, "[ERROR] Memory write out of bounds: 0x%08X\n", address);
        mem->error = true;
        return;
    }
    mem->data[address]     = (uint8_t)(value & 0xFF);
    mem->data[address + 1] = (uint8_t)((value >> 8) & 0xFF);
}

void memory_write_dword(Memory *mem, uint32_t address, uint32_t value) {
    if (!mem) { fprintf(stderr, "[ERROR] memory_write_dword: NULL pointer\n"); return; }
    if (address + 3 >= MEMORY_SIZE) {
        fprintf(stderr, "[ERROR] Memory write out of bounds: 0x%08X\n", address);
        mem->error = true;
        return;
    }
    mem->data[address]     = (uint8_t)(value & 0xFF);
    mem->data[address + 1] = (uint8_t)((value >> 8)  & 0xFF);
    mem->data[address + 2] = (uint8_t)((value >> 16) & 0xFF);
    mem->data[address + 3] = (uint8_t)((value >> 24) & 0xFF);
}
