#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include <stdint.h>

// Opcode definitions
typedef enum {
    // Data movement
    OP_NOP  = 0x00,   // No operation
    OP_MOV  = 0x01,   // MOV reg, imm
    OP_MOVR = 0x02,   // MOV reg, reg

    // Arithmetic
    OP_ADD  = 0x10,   // ADD reg, reg
    OP_SUB  = 0x11,   // SUB reg, reg
    OP_MUL  = 0x12,   // MUL reg, reg
    OP_DIV  = 0x13,   // DIV reg, reg
    OP_INC  = 0x14,   // INC reg
    OP_DEC  = 0x15,   // DEC reg

    // Logic
    OP_AND  = 0x20,   // AND reg, reg
    OP_OR   = 0x21,   // OR  reg, reg
    OP_XOR  = 0x22,   // XOR reg, reg
    OP_NOT  = 0x23,   // NOT reg

    // Comparison
    OP_CMP  = 0x30,   // CMP reg, reg  (sets flags)

    // Jump
    OP_JMP  = 0x40,   // JMP address
    OP_JE   = 0x41,   // JMP if equal
    OP_JNE  = 0x42,   // JMP if not equal
    OP_JG   = 0x43,   // JMP if greater
    OP_JL   = 0x44,   // JMP if less

    // Stack
    OP_PUSH = 0x50,   // PUSH reg
    OP_POP  = 0x51,   // POP  reg

    // Functions
    OP_CALL = 0x60,   // CALL address
    OP_RET  = 0x61,   // RET

    // Memory
    OP_LOAD = 0x70,   // LOAD reg, [address]
    OP_STOR = 0x71,   // STOR [address], reg

    // I/O
    OP_PRINT = 0x80,  // PRINT reg  (print register value)
    OP_READ  = 0x81,  // READ  reg  (read input into register)

    // Control
    OP_HALT = 0xFF,   // Stop the VM
} Opcode;

const char *opcode_name(uint8_t opcode);

#endif
