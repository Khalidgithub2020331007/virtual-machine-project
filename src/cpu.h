#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>
#include "memory.h"

#define NUM_REGISTERS 8
#define STACK_START   0xFFFF0  // Stack starts near top of memory

// General purpose registers: R0 - R7
typedef enum {
    R0 = 0, R1, R2, R3, R4, R5, R6, R7
} Register;

// CPU Flags
typedef struct {
    bool zero;      // Set if result == 0
    bool negative;  // Set if result < 0
    bool overflow;  // Set on arithmetic overflow
} Flags;

// CPU State
typedef struct {
    uint32_t reg[NUM_REGISTERS];  // General purpose registers R0-R7
    uint32_t pc;                  // Program counter
    uint32_t sp;                  // Stack pointer
    Flags    flags;               // CPU flags
    bool     running;             // VM running state
    Memory  *mem;                 // Pointer to memory
} CPU;

void cpu_init(CPU *cpu, Memory *mem);
void cpu_run(CPU *cpu);
void cpu_step(CPU *cpu);
void cpu_dump(CPU *cpu);

// Decode one instruction at addr into buf; returns number of bytes consumed.
int  cpu_disasm(Memory *mem, uint32_t addr, char *buf, int bufsz);

#endif
