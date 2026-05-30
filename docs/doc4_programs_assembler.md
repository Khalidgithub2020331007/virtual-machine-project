# Documentation — Phase 4: Test Programs & Assembler

**Steps covered:** Step 11, Step 12

---

## Step 11: Write Test Programs

### Goal
Write five programs that exercise every major part of the VM — arithmetic, loops, function calls, memory, and I/O — and verify them against known expected outputs.

### Programs overview

| File | Purpose | Expected output |
|------|---------|----------------|
| `programs/test.asm` | Basic arithmetic | `30` |
| `programs/countdown.asm` | Loop + conditional jump | `10 9 8 7 6 5 4 3 2 1 0` |
| `programs/factorial.asm` | CALL/RET + MUL loop | `120` |
| `programs/fibonacci.asm` | Two-variable loop | `0 1 1 2 3 5 8 13 21 34` |
| `programs/input_test.asm` | READ + MUL | `2 * N` (user supplies N) |

---

### Program 1 — Basic arithmetic (`test.asm`)

**Source:**
```asm
; Basic add: R0 = 10 + 20, print result (expect 30)
    MOV   R0, 10
    MOV   R1, 20
    ADD   R0, R1
    PRINT R0
    HALT
```

**How it works:**
1. Load immediate `10` into R0, `20` into R1.
2. `ADD R0, R1` — R0 = R0 + R1 = 30; zero/negative flags updated.
3. `PRINT R0` — output `30` to console/stdout.
4. `HALT` — stops the VM.

**Verification:**  
Run `./vm --program programs/test.bin` → output must be `30`.

---

### Program 2 — Countdown (`countdown.asm`)

**Source:**
```asm
; Countdown: prints 10, 9, 8 ... 0 then halts
    MOV  R0, 10
loop:
    PRINT R0
    MOV   R1, 0
    CMP   R0, R1
    JE    done
    DEC   R0
    JMP   loop
done:
    HALT
```

**How it works:**
- R0 starts at 10.
- Each iteration prints R0, then compares R0 to 0.
- If R0 == 0, jump to `done` (HALT). Otherwise decrement and loop.
- The `PRINT` happens **before** the compare so 0 is also printed.

**Verification:**  
11 lines of output: `10` down to `0`.

---

### Program 3 — Factorial (`factorial.asm`)

**Source:**
```asm
; Factorial: computes 5! = 120 using CALL/RET
    MOV  R0, 5
    CALL factorial
    PRINT R1
    HALT
factorial:
    MOV  R1, 1
floop:
    MUL  R1, R0
    DEC  R0
    MOV  R2, 0
    CMP  R0, R2
    JNE  floop
    RET
```

**How it works:**
- Main code loads `n=5` into R0 and calls `factorial`.
- The subroutine initialises R1 (the accumulator) to 1, then loops: `R1 *= R0`, `R0--` until R0 reaches 0.
- `RET` pops the return address pushed by `CALL` and jumps back.
- Main prints R1 (= 120) and halts.

**Verification:**  
Output: `120`. Demonstrates `CALL`/`RET` and stack usage.

---

### Program 4 — Fibonacci (`fibonacci.asm`)

**Source:**
```asm
; Fibonacci: prints first 10 numbers (0 1 1 2 3 5 8 13 21 34)
    MOV  R0, 0      ; a
    MOV  R1, 1      ; b
    MOV  R2, 10     ; count
loop:
    PRINT R0
    MOV   R3, R0    ; tmp = a  (emits MOVR)
    MOV   R0, R1    ; a = b    (emits MOVR)
    ADD   R1, R3    ; b = b + old_a
    DEC   R2
    MOV   R4, 0
    CMP   R2, R4
    JNE   loop
    HALT
```

**How it works:**
- R0=a, R1=b, R2=countdown, R3=temp.
- Each iteration: print `a`, save `a` in R3, set `a = b`, then `b = b + R3 (old a)`.
- Loop 10 times. Also exercises `MOVR` (register-to-register move).

**Verification:**  
10 lines: `0 1 1 2 3 5 8 13 21 34`.

---

### Program 5 — Input test (`input_test.asm`)

**Source:**
```asm
; Input test: read N from user, print N * 2
    READ  R0
    MOV   R1, 2
    MUL   R0, R1
    PRINT R0
    HALT
```

**How it works:**
- `READ R0` — in CLI mode reads an integer from stdin; in GUI mode opens an input modal.
- Multiplies by 2, prints the result.

**Verification:**  
Enter `5` → output `10`. Enter `0` → output `0`.

---

### Running the test suite

```bash
bash scripts/run_tests.sh
```

The script assembles each `.asm` file, runs the resulting `.bin`, and checks output with `diff`.

---

## Step 12: Build a Simple Assembler (Text → Binary)

### Goal
Write a single-pass assembler with a fixup table for forward label references, converting human-readable `.asm` files into `.bin` files the VM can execute directly.

### API

```c
// assembler.h
int assemble(const char *src_path, const char *dst_path);
// Returns 0 on success, 1 if any errors were found (no output written).
```

### Assembler architecture

```
.asm text file
     │
     ▼
┌───────────────────────────────┐
│  Line-by-line pass            │
│  • strip comments (;)         │
│  • detect label definitions   │  ─► label table [ name → address ]
│  • split mnemonic + operands  │
│  • emit opcode bytes + fixups │  ─► fixup table [ offset → label ]
└───────────────────────────────┘
     │
     ▼  (after last line)
┌───────────────────────────────┐
│  Fixup resolution pass        │
│  for each fixup:              │
│    look up label address      │
│    patch 4 bytes in outbuf    │
└───────────────────────────────┘
     │
     ▼
  .bin file  (fwrite outbuf)
```

### Key internal structures

```c
typedef struct { char name[64]; uint32_t addr; } Label;
typedef struct { uint32_t offset; char label[64]; int line; } Fixup;

static Label   labels[256];   // defined labels
static int     nlabels;
static Fixup   fixups[256];   // unresolved forward references
static int     nfixups;
static uint8_t outbuf[1<<20]; // output bytes (1 MB max)
static int     outlen;
```

### Emit helpers

```c
static void eb(uint8_t b)  { outbuf[outlen++] = b; }           // emit byte
static void ed(uint32_t v) { eb(v); eb(v>>8); eb(v>>16); eb(v>>24); } // emit LE dword
static void patch_dword(int off, uint32_t v) {                  // patch 4 bytes in-place
    outbuf[off]=v; outbuf[off+1]=v>>8; outbuf[off+2]=v>>16; outbuf[off+3]=v>>24;
}
```

### Label handling

**Definition** — a line whose last non-whitespace character is `:`:
```c
if (line ends with ':') {
    strip ':';
    to_upper(name);
    add_label(name, outlen);   // current output offset IS the label address
    return;
}
```

**Reference** — when a jump/call target cannot be resolved immediately:
```c
static void emit_addr(const char *tok, int ln) {
    uint32_t addr;
    if (parse_imm(tok, &addr)) { ed(addr); return; }    // numeric literal
    if (find_label(tok, &addr)) { ed(addr); return; }   // already defined
    add_fixup(outlen, tok, ln);                          // forward ref — patch later
    ed(0);                                               // placeholder
}
```

**Fixup resolution** (after the full file is parsed):
```c
for (int i = 0; i < nfixups; i++) {
    uint32_t addr;
    if (find_label(fixups[i].label, &addr))
        patch_dword(fixups[i].offset, addr);
    else
        error("undefined label");
}
```

### Instruction encoding examples

| Assembly | Emitted bytes | Explanation |
|----------|--------------|-------------|
| `MOV R0, 10` | `01 00 0A 00 00 00` | opcode=0x01, dst=0, imm=10 LE |
| `MOVR R1, R2` | `02 01 02` | opcode=0x02, dst=1, src=2 |
| `ADD R0, R1` | `10 00 01` | opcode=0x10, a=0, b=1 |
| `CMP R0, R2` | `30 00 02` | opcode=0x30, a=0, b=2 |
| `JNE loop` | `42 XX XX XX XX` | opcode=0x42, addr=label offset (LE) |
| `CALL factorial` | `60 XX XX XX XX` | opcode=0x60, addr=label offset (LE) |
| `LOAD R3, [0x100]` | `70 03 00 01 00 00` | opcode=0x70, reg=3, addr=0x100 LE |
| `HALT` | `FF` | single byte |

### Error reporting

The assembler collects all errors before exiting (so multiple errors are shown at once):

```
[ASM] programs/test.asm:3: MOV: invalid immediate
[ASM] programs/test.asm:7: undefined label 'done'
[ASM] 2 error(s) — no output written.
```

If `g_errors > 0`, the output file is **not** written — the binary is only created when the source is clean.

### Usage

```bash
# Build the assembler
make asm

# Assemble a single file
./asm programs/factorial.asm programs/factorial.bin

# Output
[ASM] programs/factorial.asm -> programs/factorial.bin (42 bytes)
```

### Key files
- [src/assembler.h](../src/assembler.h)
- [src/assembler.c](../src/assembler.c)
- [src/asm_main.c](../src/asm_main.c) — CLI entry point for the assembler binary
- [programs/](../programs/) — all `.asm` source files and their `.bin` outputs
