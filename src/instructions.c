#include "instructions.h"

const char *opcode_name(uint8_t opcode) {
    switch (opcode) {
        case OP_NOP:   return "NOP";
        case OP_MOV:   return "MOV";
        case OP_MOVR:  return "MOVR";
        case OP_ADD:   return "ADD";
        case OP_SUB:   return "SUB";
        case OP_MUL:   return "MUL";
        case OP_DIV:   return "DIV";
        case OP_INC:   return "INC";
        case OP_DEC:   return "DEC";
        case OP_AND:   return "AND";
        case OP_OR:    return "OR";
        case OP_XOR:   return "XOR";
        case OP_NOT:   return "NOT";
        case OP_CMP:   return "CMP";
        case OP_JMP:   return "JMP";
        case OP_JE:    return "JE";
        case OP_JNE:   return "JNE";
        case OP_JG:    return "JG";
        case OP_JL:    return "JL";
        case OP_PUSH:  return "PUSH";
        case OP_POP:   return "POP";
        case OP_CALL:  return "CALL";
        case OP_RET:   return "RET";
        case OP_LOAD:  return "LOAD";
        case OP_STOR:  return "STOR";
        case OP_PRINT: return "PRINT";
        case OP_READ:  return "READ";
        case OP_HALT:  return "HALT";
        default:       return "UNKNOWN";
    }
}
