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
                snprintf(msg, sizeof(msg), "[WARNING] Stack overflow at PC=0x%08X", cpu->pc - 5);
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
                snprintf(msg, sizeof(msg), "[WARNING] Stack underflow at PC=0x%08X", cpu->pc - 1);
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
        snprintf(msg, sizeof(msg), "[ERROR] Memory access out of bounds at PC=0x%08X", cpu->pc);
        cpu_error(cpu, msg);
    }
}

void cpu_run(CPU *cpu) {
    if (!cpu) return;
    while (cpu->running) {
        cpu_step(cpu);
    }
}

// ── Disassembler ──────────────────────────────────────────────────────────────
// Reads directly from mem->data so display calls never set mem->error.

static uint8_t  da_byte (Memory *m, uint32_t a)
    { return (a < MEMORY_SIZE) ? m->data[a] : 0; }
static uint32_t da_dword(Memory *m, uint32_t a)
    { return (a+3 < MEMORY_SIZE)
        ? ((uint32_t)m->data[a] | ((uint32_t)m->data[a+1]<<8)
          | ((uint32_t)m->data[a+2]<<16) | ((uint32_t)m->data[a+3]<<24)) : 0; }

int cpu_disasm(Memory *mem, uint32_t addr, char *buf, int bufsz) {
    if (!mem || !buf || bufsz <= 0) return 1;
    uint8_t op = da_byte(mem, addr);
    switch (op) {
        case OP_NOP:  snprintf(buf,bufsz,"NOP"); return 1;
        case OP_MOV: {
            uint8_t r = da_byte(mem,addr+1)&0x07; uint32_t v = da_dword(mem,addr+2);
            snprintf(buf,bufsz,"MOV  R%u, %u",r,v); return 6; }
        case OP_MOVR: {
            uint8_t d=da_byte(mem,addr+1)&7, s=da_byte(mem,addr+2)&7;
            snprintf(buf,bufsz,"MOVR R%u, R%u",d,s); return 3; }
        case OP_ADD: {
            uint8_t d=da_byte(mem,addr+1)&7, s=da_byte(mem,addr+2)&7;
            snprintf(buf,bufsz,"ADD  R%u, R%u",d,s); return 3; }
        case OP_SUB: {
            uint8_t d=da_byte(mem,addr+1)&7, s=da_byte(mem,addr+2)&7;
            snprintf(buf,bufsz,"SUB  R%u, R%u",d,s); return 3; }
        case OP_MUL: {
            uint8_t d=da_byte(mem,addr+1)&7, s=da_byte(mem,addr+2)&7;
            snprintf(buf,bufsz,"MUL  R%u, R%u",d,s); return 3; }
        case OP_DIV: {
            uint8_t d=da_byte(mem,addr+1)&7, s=da_byte(mem,addr+2)&7;
            snprintf(buf,bufsz,"DIV  R%u, R%u",d,s); return 3; }
        case OP_INC: {
            uint8_t r=da_byte(mem,addr+1)&7;
            snprintf(buf,bufsz,"INC  R%u",r); return 2; }
        case OP_DEC: {
            uint8_t r=da_byte(mem,addr+1)&7;
            snprintf(buf,bufsz,"DEC  R%u",r); return 2; }
        case OP_AND: {
            uint8_t d=da_byte(mem,addr+1)&7, s=da_byte(mem,addr+2)&7;
            snprintf(buf,bufsz,"AND  R%u, R%u",d,s); return 3; }
        case OP_OR: {
            uint8_t d=da_byte(mem,addr+1)&7, s=da_byte(mem,addr+2)&7;
            snprintf(buf,bufsz,"OR   R%u, R%u",d,s); return 3; }
        case OP_XOR: {
            uint8_t d=da_byte(mem,addr+1)&7, s=da_byte(mem,addr+2)&7;
            snprintf(buf,bufsz,"XOR  R%u, R%u",d,s); return 3; }
        case OP_NOT: {
            uint8_t r=da_byte(mem,addr+1)&7;
            snprintf(buf,bufsz,"NOT  R%u",r); return 2; }
        case OP_CMP: {
            uint8_t a2=da_byte(mem,addr+1)&7, b=da_byte(mem,addr+2)&7;
            snprintf(buf,bufsz,"CMP  R%u, R%u",a2,b); return 3; }
        case OP_JMP: {
            uint32_t t=da_dword(mem,addr+1);
            snprintf(buf,bufsz,"JMP  0x%X",t); return 5; }
        case OP_JE: {
            uint32_t t=da_dword(mem,addr+1);
            snprintf(buf,bufsz,"JE   0x%X",t); return 5; }
        case OP_JNE: {
            uint32_t t=da_dword(mem,addr+1);
            snprintf(buf,bufsz,"JNE  0x%X",t); return 5; }
        case OP_JG: {
            uint32_t t=da_dword(mem,addr+1);
            snprintf(buf,bufsz,"JG   0x%X",t); return 5; }
        case OP_JL: {
            uint32_t t=da_dword(mem,addr+1);
            snprintf(buf,bufsz,"JL   0x%X",t); return 5; }
        case OP_PUSH: {
            uint8_t r=da_byte(mem,addr+1)&7;
            snprintf(buf,bufsz,"PUSH R%u",r); return 2; }
        case OP_POP: {
            uint8_t r=da_byte(mem,addr+1)&7;
            snprintf(buf,bufsz,"POP  R%u",r); return 2; }
        case OP_CALL: {
            uint32_t t=da_dword(mem,addr+1);
            snprintf(buf,bufsz,"CALL 0x%X",t); return 5; }
        case OP_RET:  snprintf(buf,bufsz,"RET"); return 1;
        case OP_LOAD: {
            uint8_t r=da_byte(mem,addr+1)&7; uint32_t a2=da_dword(mem,addr+2);
            snprintf(buf,bufsz,"LOAD R%u, [0x%X]",r,a2); return 6; }
        case OP_STOR: {
            uint32_t a2=da_dword(mem,addr+1); uint8_t r=da_byte(mem,addr+5)&7;
            snprintf(buf,bufsz,"STOR [0x%X], R%u",a2,r); return 6; }
        case OP_PRINT: {
            uint8_t r=da_byte(mem,addr+1)&7;
            snprintf(buf,bufsz,"PRINT R%u",r); return 2; }
        case OP_READ: {
            uint8_t r=da_byte(mem,addr+1)&7;
            snprintf(buf,bufsz,"READ R%u",r); return 2; }
        case OP_HALT: snprintf(buf,bufsz,"HALT"); return 1;
        default:
            snprintf(buf,bufsz,"DB   0x%02X",op); return 1;
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
