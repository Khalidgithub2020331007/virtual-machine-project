#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpu.h"
#include "instructions.h"
#include "io.h"

static void cpu_error(CPU *cpu, const char *msg) {
    io_print_string(msg);
    cpu->running = false;
}

void cpu_init(CPU *cpu, Memory *mem) {
    if (!cpu || !mem) { fprintf(stderr, "[ERROR] cpu_init: NULL pointer\n"); return; }
    for (int i = 0; i < NUM_REGISTERS; i++)
        cpu->reg[i] = 0;
    cpu->pc      = 0x0000;
    cpu->sp      = STACK_START;
    cpu->flags.zero     = false;
    cpu->flags.negative = false;
    cpu->flags.overflow = false;
    cpu->running = true;
    cpu->mem     = mem;
}

// Update flags based on result
static void update_flags(CPU *cpu, int32_t result) {
    cpu->flags.zero     = (result == 0);
    cpu->flags.negative = (result < 0);
}

// Fetch next byte from memory and advance PC
static uint8_t fetch_byte(CPU *cpu) {
    uint8_t val = memory_read_byte(cpu->mem, cpu->pc);
    cpu->pc++;
    return val;
}

// Fetch next 32-bit value and advance PC
static uint32_t fetch_dword(CPU *cpu) {
    uint32_t val = memory_read_dword(cpu->mem, cpu->pc);
    cpu->pc += 4;
    return val;
}

void cpu_step(CPU *cpu) {
    if (!cpu || !cpu->mem) { fprintf(stderr, "[ERROR] cpu_step: NULL pointer\n"); return; }
    uint8_t opcode = fetch_byte(cpu);

    switch (opcode) {

        case OP_NOP:
            break;

        // MOV reg, immediate
        case OP_MOV: {
            uint8_t  reg = fetch_byte(cpu);
            uint32_t imm = fetch_dword(cpu);
            cpu->reg[reg & 0x07] = imm;
            break;
        }

        // MOV reg, reg
        case OP_MOVR: {
            uint8_t dst = fetch_byte(cpu);
            uint8_t src = fetch_byte(cpu);
            cpu->reg[dst & 0x07] = cpu->reg[src & 0x07];
            break;
        }

        // ADD dst, src
        case OP_ADD: {
            uint8_t dst = fetch_byte(cpu);
            uint8_t src = fetch_byte(cpu);
            int32_t result = (int32_t)cpu->reg[dst & 0x07] + (int32_t)cpu->reg[src & 0x07];
            cpu->reg[dst & 0x07] = (uint32_t)result;
            update_flags(cpu, result);
            break;
        }

        // SUB dst, src
        case OP_SUB: {
            uint8_t dst = fetch_byte(cpu);
            uint8_t src = fetch_byte(cpu);
            int32_t result = (int32_t)cpu->reg[dst & 0x07] - (int32_t)cpu->reg[src & 0x07];
            cpu->reg[dst & 0x07] = (uint32_t)result;
            update_flags(cpu, result);
            break;
        }

        // MUL dst, src
        case OP_MUL: {
            uint8_t dst = fetch_byte(cpu);
            uint8_t src = fetch_byte(cpu);
            int32_t result = (int32_t)cpu->reg[dst & 0x07] * (int32_t)cpu->reg[src & 0x07];
            cpu->reg[dst & 0x07] = (uint32_t)result;
            update_flags(cpu, result);
            break;
        }

        // DIV dst, src
        case OP_DIV: {
            uint8_t dst = fetch_byte(cpu);
            uint8_t src = fetch_byte(cpu);
            if (cpu->reg[src & 0x07] == 0) {
                char msg[64];
                snprintf(msg, sizeof(msg), "[ERROR] Division by zero at PC=0x%08X", cpu->pc);
                cpu_error(cpu, msg);
                break;
            }
            int32_t result = (int32_t)cpu->reg[dst & 0x07] / (int32_t)cpu->reg[src & 0x07];
            cpu->reg[dst & 0x07] = (uint32_t)result;
            update_flags(cpu, result);
            break;
        }

        // INC reg
        case OP_INC: {
            uint8_t reg = fetch_byte(cpu);
            cpu->reg[reg & 0x07]++;
            update_flags(cpu, (int32_t)cpu->reg[reg & 0x07]);
            break;
        }

        // DEC reg
        case OP_DEC: {
            uint8_t reg = fetch_byte(cpu);
            cpu->reg[reg & 0x07]--;
            update_flags(cpu, (int32_t)cpu->reg[reg & 0x07]);
            break;
        }

        // AND dst, src
        case OP_AND: {
            uint8_t dst = fetch_byte(cpu);
            uint8_t src = fetch_byte(cpu);
            cpu->reg[dst & 0x07] &= cpu->reg[src & 0x07];
            update_flags(cpu, (int32_t)cpu->reg[dst & 0x07]);
            break;
        }

        // OR dst, src
        case OP_OR: {
            uint8_t dst = fetch_byte(cpu);
            uint8_t src = fetch_byte(cpu);
            cpu->reg[dst & 0x07] |= cpu->reg[src & 0x07];
            update_flags(cpu, (int32_t)cpu->reg[dst & 0x07]);
            break;
        }

        // XOR dst, src
        case OP_XOR: {
            uint8_t dst = fetch_byte(cpu);
            uint8_t src = fetch_byte(cpu);
            cpu->reg[dst & 0x07] ^= cpu->reg[src & 0x07];
            update_flags(cpu, (int32_t)cpu->reg[dst & 0x07]);
            break;
        }

        // NOT reg
        case OP_NOT: {
            uint8_t reg = fetch_byte(cpu);
            cpu->reg[reg & 0x07] = ~cpu->reg[reg & 0x07];
            update_flags(cpu, (int32_t)cpu->reg[reg & 0x07]);
            break;
        }

        // CMP reg, reg  (sets flags, doesn't store result)
        case OP_CMP: {
            uint8_t a = fetch_byte(cpu);
            uint8_t b = fetch_byte(cpu);
            int32_t result = (int32_t)cpu->reg[a & 0x07] - (int32_t)cpu->reg[b & 0x07];
            update_flags(cpu, result);
            break;
        }

        // JMP address
        case OP_JMP: {
            uint32_t addr = fetch_dword(cpu);
            cpu->pc = addr;
            break;
        }

        // JE address (jump if zero flag set)
        case OP_JE: {
            uint32_t addr = fetch_dword(cpu);
            if (cpu->flags.zero) cpu->pc = addr;
            break;
        }

        // JNE address
        case OP_JNE: {
            uint32_t addr = fetch_dword(cpu);
            if (!cpu->flags.zero) cpu->pc = addr;
            break;
        }

        // JG address (jump if not zero and not negative)
        case OP_JG: {
            uint32_t addr = fetch_dword(cpu);
            if (!cpu->flags.zero && !cpu->flags.negative) cpu->pc = addr;
            break;
        }

        // JL address (jump if negative)
        case OP_JL: {
            uint32_t addr = fetch_dword(cpu);
            if (cpu->flags.negative) cpu->pc = addr;
            break;
        }

        // PUSH reg
        case OP_PUSH: {
            uint8_t reg = fetch_byte(cpu);
            if (cpu->sp < 4) {
                char msg[64];
                snprintf(msg, sizeof(msg), "[WARNING] Stack overflow at PC=0x%08X", cpu->pc - 2);
                cpu_error(cpu, msg);
                break;
            }
            cpu->sp -= 4;
            memory_write_dword(cpu->mem, cpu->sp, cpu->reg[reg & 0x07]);
            break;
        }

        // POP reg
        case OP_POP: {
            uint8_t reg = fetch_byte(cpu);
            if (cpu->sp >= STACK_START) {
                char msg[64];
                snprintf(msg, sizeof(msg), "[WARNING] Stack underflow at PC=0x%08X", cpu->pc - 2);
                cpu_error(cpu, msg);
                break;
            }
            cpu->reg[reg & 0x07] = memory_read_dword(cpu->mem, cpu->sp);
            cpu->sp += 4;
            break;
        }

        // CALL address
        case OP_CALL: {
            uint32_t addr = fetch_dword(cpu);
            if (cpu->sp < 4) {
                char msg[64];
                snprintf(msg, sizeof(msg), "[WARNING] Stack overflow (CALL) at PC=0x%08X", cpu->pc - 5);
                cpu_error(cpu, msg);
                break;
            }
            cpu->sp -= 4;
            memory_write_dword(cpu->mem, cpu->sp, cpu->pc);  // save return address
            cpu->pc = addr;
            break;
        }

        // RET
        case OP_RET: {
            if (cpu->sp >= STACK_START) {
                char msg[64];
                snprintf(msg, sizeof(msg), "[WARNING] Stack underflow (RET) at PC=0x%08X", cpu->pc - 1);
                cpu_error(cpu, msg);
                break;
            }
            cpu->pc = memory_read_dword(cpu->mem, cpu->sp);
            cpu->sp += 4;
            break;
        }

        // LOAD reg, [address]
        case OP_LOAD: {
            uint8_t  reg  = fetch_byte(cpu);
            uint32_t addr = fetch_dword(cpu);
            cpu->reg[reg & 0x07] = memory_read_dword(cpu->mem, addr);
            break;
        }

        // STOR [address], reg
        case OP_STOR: {
            uint32_t addr = fetch_dword(cpu);
            uint8_t  reg  = fetch_byte(cpu);
            memory_write_dword(cpu->mem, addr, cpu->reg[reg & 0x07]);
            break;
        }

        // PRINT reg
        case OP_PRINT: {
            uint8_t reg = fetch_byte(cpu);
            io_print_int(cpu->reg[reg & 0x07]);
            break;
        }

        // READ reg
        case OP_READ: {
            uint8_t reg = fetch_byte(cpu);
            cpu->reg[reg & 0x07] = io_read_int();
            break;
        }

        // HALT
        case OP_HALT:
            io_print_string("[VM] HALT — program finished.");
            cpu->running = false;
            break;

        default: {
            char msg[64];
            snprintf(msg, sizeof(msg), "[ERROR] Unknown opcode 0x%02X at PC=0x%08X", opcode, cpu->pc - 1);
            cpu_error(cpu, msg);
            break;
        }
    }

    if (cpu->mem->error) {
        cpu->mem->error = false;
        char msg[64];
        snprintf(msg, sizeof(msg), "[ERROR] Memory out-of-bounds — halting at PC=0x%08X", cpu->pc);
        cpu_error(cpu, msg);
    }
}

void cpu_run(CPU *cpu) {
    if (!cpu) return;
    while (cpu->running) {
        cpu_step(cpu);
    }
}

void cpu_dump(CPU *cpu) {
    if (!cpu) return;
    printf("\n=== CPU State ===\n");
    for (int i = 0; i < NUM_REGISTERS; i++)
        printf("  R%d = %u (0x%08X)\n", i, cpu->reg[i], cpu->reg[i]);
    printf("  PC = 0x%08X\n", cpu->pc);
    printf("  SP = 0x%08X\n", cpu->sp);
    printf("  Flags: ZERO=%d  NEG=%d  OVF=%d\n",
           cpu->flags.zero, cpu->flags.negative, cpu->flags.overflow);
    printf("=================\n\n");
}
