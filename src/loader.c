#include <stdio.h>
#include <stdbool.h>
#include "loader.h"
#include "io.h"

/* Write msg to stderr (CLI graders) and to the GUI console. */
static void loader_report(const char *msg) {
    fprintf(stderr, "%s\n", msg);
    io_print_string(msg);
}

/*
 * Operand byte count for each opcode (bytes that follow the opcode byte).
 * Returns -1 for unknown opcodes.
 *
 * Opcode map (matches instructions.h):
 *   0x00        NOP             0 extra
 *   0x01        MOV  reg, imm32 5 extra (1 reg + 4 imm)
 *   0x02        MOVR reg, reg   2 extra
 *   0x10–0x13   ADD SUB MUL DIV 2 extra (dst reg + src reg)
 *   0x14–0x15   INC DEC         1 extra
 *   0x20–0x22   AND OR XOR      2 extra
 *   0x23        NOT             1 extra
 *   0x30        CMP             2 extra
 *   0x40–0x44   JMP JE JNE JG JL  4 extra (addr32)
 *   0x50–0x51   PUSH POP        1 extra
 *   0x60        CALL            4 extra (addr32)
 *   0x61        RET             0 extra
 *   0x70        LOAD reg,[addr] 5 extra (1 reg + 4 addr)
 *   0x71        STOR [addr],reg 5 extra (4 addr + 1 reg)
 *   0x80–0x81   PRINT READ      1 extra
 *   0xFF        HALT            0 extra
 */
static int operand_bytes(uint8_t op) {
    switch (op) {
        case 0x00:                                    return 0;
        case 0x01:                                    return 5;
        case 0x02:                                    return 2;
        case 0x10: case 0x11: case 0x12: case 0x13:  return 2;
        case 0x14: case 0x15:                         return 1;
        case 0x20: case 0x21: case 0x22:              return 2;
        case 0x23:                                    return 1;
        case 0x30:                                    return 2;
        case 0x40: case 0x41: case 0x42:
        case 0x43: case 0x44:                         return 4;
        case 0x50: case 0x51:                         return 1;
        case 0x60:                                    return 4;
        case 0x61:                                    return 0;
        case 0x70:                                    return 5;
        case 0x71:                                    return 5;
        case 0x80: case 0x81:                         return 1;
        case 0xFF:                                    return 0;
        default:                                      return -1;
    }
}

/*
 * Walk the loaded binary and report structural problems as warnings.
 * Does not prevent loading — the CPU's own runtime checks remain the last line.
 */
static void validate_binary(const uint8_t *data, long size) {
    long    pos       = 0;
    uint8_t last_op   = 0;
    bool    truncated = false;

    while (pos < size) {
        uint8_t op    = data[pos];
        int     extra = operand_bytes(op);

        if (extra < 0) {
            char msg[72];
            snprintf(msg, sizeof(msg),
                     "[WARNING] Unknown opcode 0x%02X at binary offset %ld", op, pos);
            loader_report(msg);
            pos++;
            continue;
        }

        if (pos + 1 + extra > size) {
            char msg[128];
            snprintf(msg, sizeof(msg),
                     "[WARNING] Instruction 0x%02X at offset %ld is truncated (%ld byte(s) missing)",
                     op, pos, (pos + 1 + extra) - size);
            loader_report(msg);
            truncated = true;
            break;
        }

        last_op = op;
        pos += 1 + extra;
    }

    if (!truncated && last_op != 0xFF)
        loader_report("[WARNING] Program does not end with HALT");
}

int loader_load_file(Memory *mem, const char *filename, uint32_t start_address) {
    if (!mem) {
        fprintf(stderr, "[ERROR] Loader: NULL memory pointer\n");
        return -1;
    }
    if (!filename) {
        fprintf(stderr, "[ERROR] Loader: NULL filename\n");
        return -1;
    }
    if (start_address >= MEMORY_SIZE) {
        char msg[64];
        snprintf(msg, sizeof(msg),
                 "[ERROR] Load address 0x%08X is out of bounds", start_address);
        loader_report(msg);
        return -1;
    }

    FILE *f = fopen(filename, "rb");
    if (!f) {
        char msg[320];
        snprintf(msg, sizeof(msg), "[ERROR] Cannot open file: %s", filename);
        loader_report(msg);
        return -1;
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (fsize <= 0) {
        loader_report("[WARNING] Program file is empty");
        fclose(f);
        return -1;
    }

    uint32_t address = start_address;
    int byte;
    while ((byte = fgetc(f)) != EOF) {
        if (address >= MEMORY_SIZE) {
            loader_report("[ERROR] Program too large for memory");
            fclose(f);
            return -1;
        }
        memory_write_byte(mem, address++, (uint8_t)byte);
    }
    fclose(f);

    uint32_t loaded = address - start_address;
    validate_binary(mem->data + start_address, (long)loaded);

    char msg[80];
    snprintf(msg, sizeof(msg), "[Loader] %u bytes loaded at 0x%08X", loaded, start_address);
    io_print_string(msg);
    return 0;
}
