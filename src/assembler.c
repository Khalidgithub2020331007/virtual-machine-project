#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include "assembler.h"

// ── Limits ────────────────────────────────────────────────────────────────────
#define MAX_LABELS  256
#define MAX_FIXUPS  256
#define MAX_OUTPUT  (1024 * 1024)
#define MAX_LINE    256

// ── Internal state ────────────────────────────────────────────────────────────
typedef struct { char name[64]; uint32_t addr; }           Label;
typedef struct { uint32_t offset; char label[64]; int ln; } Fixup;

static Label   labels[MAX_LABELS];
static int     nlabels;
static Fixup   fixups[MAX_FIXUPS];
static int     nfixups;
static uint8_t outbuf[MAX_OUTPUT];
static int     outlen;
static int     g_errors;
static const char *g_src;

// ── Output helpers ────────────────────────────────────────────────────────────

static void eb(uint8_t b) {
    if (outlen < MAX_OUTPUT) outbuf[outlen++] = b;
}

static void ed(uint32_t v) {
    eb(v & 0xFF); eb((v >> 8) & 0xFF);
    eb((v >> 16) & 0xFF); eb((v >> 24) & 0xFF);
}

static void patch_dword(int off, uint32_t v) {
    outbuf[off]   =  v        & 0xFF;
    outbuf[off+1] = (v >>  8) & 0xFF;
    outbuf[off+2] = (v >> 16) & 0xFF;
    outbuf[off+3] = (v >> 24) & 0xFF;
}

// ── Error reporting ───────────────────────────────────────────────────────────

static void err(int ln, const char *msg) {
    fprintf(stderr, "[ASM] %s:%d: %s\n", g_src, ln, msg);
    g_errors++;
}

// ── String utilities ──────────────────────────────────────────────────────────

static void rtrim(char *s) {
    int n = (int)strlen(s);
    while (n > 0 && (unsigned char)s[n-1] <= ' ') s[--n] = '\0';
}

static char *ltrim(char *s) {
    while (*s && (unsigned char)*s <= ' ') s++;
    return s;
}

static void strip_comment(char *s) {
    char *p = strchr(s, ';');
    if (p) *p = '\0';
    rtrim(s);
}

static void to_upper(char *s) {
    for (; *s; s++) *s = (char)toupper((unsigned char)*s);
}

// ── Register parse: "R0"–"R7" → 0–7, else -1 ─────────────────────────────────

static int parse_reg(const char *s) {
    if (s[0] == 'R' && s[1] >= '0' && s[1] <= '7' && s[2] == '\0')
        return s[1] - '0';
    return -1;
}

// ── Immediate parse: decimal or 0x hex ───────────────────────────────────────

static int parse_imm(const char *s, uint32_t *out) {
    char *end;
    long long v;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        v = strtoll(s, &end, 16);
    else
        v = strtoll(s, &end, 10);
    if (end == s || *end != '\0') return 0;
    *out = (uint32_t)v;
    return 1;
}

// ── Label table ───────────────────────────────────────────────────────────────

static void add_label(const char *name, uint32_t addr) {
    if (nlabels >= MAX_LABELS) return;
    strncpy(labels[nlabels].name, name, 63);
    labels[nlabels].addr = addr;
    nlabels++;
}

static int find_label(const char *name, uint32_t *addr) {
    for (int i = 0; i < nlabels; i++)
        if (strcmp(labels[i].name, name) == 0) { *addr = labels[i].addr; return 1; }
    return 0;
}

// ── Fixup table (forward label references) ────────────────────────────────────

static void add_fixup(int offset, const char *label, int ln) {
    if (nfixups >= MAX_FIXUPS) return;
    fixups[nfixups].offset = (uint32_t)offset;
    strncpy(fixups[nfixups].label, label, 63);
    fixups[nfixups].ln = ln;
    nfixups++;
}

// ── Emit a 4-byte address operand (number or label) ───────────────────────────

static void emit_addr(const char *tok, int ln) {
    uint32_t addr;
    if (parse_imm(tok, &addr)) { ed(addr); return; }
    if (find_label(tok, &addr)) { ed(addr); return; }
    add_fixup(outlen, tok, ln);  // forward reference — patch later
    ed(0);
}

// ── Strip [ ] brackets in-place ───────────────────────────────────────────────

static void strip_brackets(char *s) {
    int n = (int)strlen(s);
    if (n >= 2 && s[0] == '[' && s[n-1] == ']') {
        memmove(s, s + 1, (size_t)(n - 2));
        s[n - 2] = '\0';
    }
}

// ── Split "MNEM op1, op2" into parts; returns operand count ──────────────────

static int split_ops(char *line, char *mnem, char *op1, char *op2) {
    mnem[0] = op1[0] = op2[0] = '\0';
    char *p = ltrim(line);

    int i = 0;
    while (*p && (unsigned char)*p > ' ') mnem[i++] = *p++;
    mnem[i] = '\0';
    if (!mnem[0]) return 0;

    p = ltrim(p);
    if (!*p) return 1;

    i = 0;
    while (*p && *p != ',') op1[i++] = *p++;
    op1[i] = '\0';
    while (i > 0 && (unsigned char)op1[i-1] <= ' ') op1[--i] = '\0';

    if (*p != ',') return 2;
    p = ltrim(p + 1);

    i = 0;
    while (*p) op2[i++] = *p++;
    op2[i] = '\0';
    while (i > 0 && (unsigned char)op2[i-1] <= ' ') op2[--i] = '\0';
    return 3;
}

// ── Assemble one source line ──────────────────────────────────────────────────

static void assemble_line(char *raw, int ln) {
    rtrim(raw);
    char *p = ltrim(raw);
    strip_comment(p);
    p = ltrim(p);
    if (!*p) return;

    // Label definition ("name:")
    int len = (int)strlen(p);
    if (p[len - 1] == ':') {
        char name[64];
        p[len - 1] = '\0';
        strncpy(name, p, 63); name[63] = '\0';
        to_upper(name);
        add_label(name, (uint32_t)outlen);
        return;
    }

    char mnem[32], op1[64], op2[64];
    int nops = split_ops(p, mnem, op1, op2);
    to_upper(mnem); to_upper(op1); to_upper(op2);

    // ── NOP / HALT / RET ──────────────────────────────────────────────────────
    if      (strcmp(mnem, "NOP")  == 0) { eb(0x00); }
    else if (strcmp(mnem, "HALT") == 0) { eb(0xFF); }
    else if (strcmp(mnem, "RET")  == 0) { eb(0x61); }

    // ── MOV reg, imm  or  MOV/MOVR reg, reg ─────────────────────────────────
    else if (strcmp(mnem, "MOV") == 0 || strcmp(mnem, "MOVR") == 0) {
        if (nops < 3) { err(ln, "MOV needs 2 operands"); return; }
        int dst = parse_reg(op1);
        if (dst < 0) { err(ln, "MOV: invalid dst register"); return; }
        int src = parse_reg(op2);
        if (src >= 0) {
            eb(0x02); eb((uint8_t)dst); eb((uint8_t)src);   // MOVR
        } else {
            uint32_t imm;
            if (!parse_imm(op2, &imm)) { err(ln, "MOV: invalid immediate"); return; }
            eb(0x01); eb((uint8_t)dst); ed(imm);             // MOV imm
        }
    }

    // ── Two-reg arithmetic / logic / compare ─────────────────────────────────
    else if (strcmp(mnem,"ADD")==0 || strcmp(mnem,"SUB")==0 ||
             strcmp(mnem,"MUL")==0 || strcmp(mnem,"DIV")==0 ||
             strcmp(mnem,"AND")==0 || strcmp(mnem,"OR") ==0 ||
             strcmp(mnem,"XOR")==0 || strcmp(mnem,"CMP")==0) {
        if (nops < 3) { err(ln, "needs 2 register operands"); return; }
        int dst = parse_reg(op1), src = parse_reg(op2);
        if (dst < 0 || src < 0) { err(ln, "invalid register"); return; }
        uint8_t opc = (strcmp(mnem,"ADD")==0) ? 0x10 :
                      (strcmp(mnem,"SUB")==0) ? 0x11 :
                      (strcmp(mnem,"MUL")==0) ? 0x12 :
                      (strcmp(mnem,"DIV")==0) ? 0x13 :
                      (strcmp(mnem,"AND")==0) ? 0x20 :
                      (strcmp(mnem,"OR") ==0) ? 0x21 :
                      (strcmp(mnem,"XOR")==0) ? 0x22 : 0x30; /* CMP */
        eb(opc); eb((uint8_t)dst); eb((uint8_t)src);
    }

    // ── One-reg instructions ──────────────────────────────────────────────────
    else if (strcmp(mnem,"INC")  ==0 || strcmp(mnem,"DEC")  ==0 ||
             strcmp(mnem,"NOT")  ==0 || strcmp(mnem,"PUSH") ==0 ||
             strcmp(mnem,"POP")  ==0 || strcmp(mnem,"PRINT")==0 ||
             strcmp(mnem,"READ") ==0) {
        if (nops < 2) { err(ln, "needs 1 register operand"); return; }
        int reg = parse_reg(op1);
        if (reg < 0) { err(ln, "invalid register"); return; }
        uint8_t opc = (strcmp(mnem,"INC")  ==0) ? 0x14 :
                      (strcmp(mnem,"DEC")  ==0) ? 0x15 :
                      (strcmp(mnem,"NOT")  ==0) ? 0x23 :
                      (strcmp(mnem,"PUSH") ==0) ? 0x50 :
                      (strcmp(mnem,"POP")  ==0) ? 0x51 :
                      (strcmp(mnem,"PRINT")==0) ? 0x80 : 0x81; /* READ */
        eb(opc); eb((uint8_t)reg);
    }

    // ── Jump / CALL — address or label operand ────────────────────────────────
    else if (strcmp(mnem,"JMP") ==0 || strcmp(mnem,"JE")  ==0 ||
             strcmp(mnem,"JNE") ==0 || strcmp(mnem,"JG")  ==0 ||
             strcmp(mnem,"JL")  ==0 || strcmp(mnem,"CALL")==0) {
        if (nops < 2) { err(ln, "needs address operand"); return; }
        uint8_t opc = (strcmp(mnem,"JMP") ==0) ? 0x40 :
                      (strcmp(mnem,"JE")  ==0) ? 0x41 :
                      (strcmp(mnem,"JNE") ==0) ? 0x42 :
                      (strcmp(mnem,"JG")  ==0) ? 0x43 :
                      (strcmp(mnem,"JL")  ==0) ? 0x44 : 0x60; /* CALL */
        eb(opc); emit_addr(op1, ln);
    }

    // ── LOAD reg, [addr] ─────────────────────────────────────────────────────
    else if (strcmp(mnem, "LOAD") == 0) {
        if (nops < 3) { err(ln, "LOAD needs reg and [addr]"); return; }
        int reg = parse_reg(op1);
        if (reg < 0) { err(ln, "LOAD: invalid register"); return; }
        strip_brackets(op2);
        eb(0x70); eb((uint8_t)reg); emit_addr(op2, ln);
    }

    // ── STOR [addr], reg ─────────────────────────────────────────────────────
    else if (strcmp(mnem, "STOR") == 0) {
        if (nops < 3) { err(ln, "STOR needs [addr] and reg"); return; }
        int reg = parse_reg(op2);
        if (reg < 0) { err(ln, "STOR: invalid register"); return; }
        strip_brackets(op1);
        eb(0x71); emit_addr(op1, ln); eb((uint8_t)reg);
    }

    else {
        char msg[80];
        snprintf(msg, sizeof(msg), "unknown mnemonic '%s'", mnem);
        err(ln, msg);
    }
}

// ── Public API ────────────────────────────────────────────────────────────────

int assemble(const char *src_path, const char *dst_path) {
    g_src    = src_path;
    g_errors = nlabels = nfixups = outlen = 0;

    FILE *in = fopen(src_path, "r");
    if (!in) {
        fprintf(stderr, "[ASM] cannot open '%s'\n", src_path);
        return 1;
    }

    char line[MAX_LINE];
    int  ln = 0;
    while (fgets(line, sizeof(line), in))
        assemble_line(line, ++ln);
    fclose(in);

    // Resolve forward-reference fixups
    for (int i = 0; i < nfixups; i++) {
        uint32_t addr;
        if (find_label(fixups[i].label, &addr)) {
            patch_dword((int)fixups[i].offset, addr);
        } else {
            fprintf(stderr, "[ASM] %s:%d: undefined label '%s'\n",
                    src_path, fixups[i].ln, fixups[i].label);
            g_errors++;
        }
    }

    if (g_errors) {
        fprintf(stderr, "[ASM] %d error(s) — no output written.\n", g_errors);
        return 1;
    }

    FILE *out = fopen(dst_path, "wb");
    if (!out) {
        fprintf(stderr, "[ASM] cannot write '%s'\n", dst_path);
        return 1;
    }
    fwrite(outbuf, 1, (size_t)outlen, out);
    fclose(out);

    printf("[ASM] %-30s -> %-30s (%d bytes)\n", src_path, dst_path, outlen);
    return 0;
}
