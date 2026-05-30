# Documentation — Phase 1: Foundation

**Steps covered:** Step 1, Step 2, Step 3

---

## Step 1: Project Setup & GUI Dependencies

### Goal
Establish the folder structure, build system, and dependency notes so every subsequent step has a clean base to build on.

### What was done

**Folder layout created:**
```
vm-project/
├── src/           # All C source and header files
├── programs/      # Binary and .asm test programs
├── docs/          # Project documentation
├── scripts/       # Helper shell scripts
├── Makefile       # Build entry point
├── README.md      # User-facing build/run guide
├── PLAN.md        # Step-by-step development plan
├── CLAUDE.md      # AI-assistant context file
└── LLM_USAGE.md   # Record of AI tool usage
```

**Makefile targets:**
| Target | Action |
|--------|--------|
| `make` | Build the CLI VM binary (`vm`) |
| `make gui` | Build the GUI VM binary (`vm_gui`) |
| `make asm` | Build the assembler binary (`asm`) |
| `make clean` | Remove all compiled artifacts |

**SDL2 dependency (GUI mode):**
- Required only when compiling the GUI target.
- Install on Ubuntu/Debian: `sudo apt install libsdl2-dev libsdl2-ttf-dev`
- The Makefile uses `pkg-config --cflags --libs sdl2 SDL2_ttf` to locate headers and libraries automatically.
- If SDL2 is absent, only the CLI (`vm`) and assembler (`asm`) targets are affected — the core emulator still builds.

### Key files
- [Makefile](../Makefile) — complete build rules
- [PLAN.md](../PLAN.md) — full step checklist
- [claude.md](../claude.md) — development rules and environment notes

---

## Step 2: Memory Module

### Goal
Implement a 1 MB flat address space with byte, word, and double-word access, plus out-of-bounds error detection.

### Design decisions
- **Size:** `MEMORY_SIZE = 1024 * 1024` (1 048 576 bytes). One flat `uint8_t` array covers the entire address space, keeping addressing simple.
- **Error flag:** Rather than aborting on a bad address, `Memory.error` is set to `true`. The caller (CPU, loader) checks this flag and decides whether to halt or log.
- **Endianness:** Words and dwords are stored **little-endian** (low byte at the lower address), matching x86 convention and making it easier to inspect raw binary dumps.

### API

```c
// memory.h
#define MEMORY_SIZE (1024 * 1024)

typedef struct {
    uint8_t data[MEMORY_SIZE];
    bool    error;   // set on any out-of-bounds access
} Memory;

void     memory_init(Memory *mem);
uint8_t  memory_read_byte (Memory *mem, uint32_t address);
uint16_t memory_read_word (Memory *mem, uint32_t address);
uint32_t memory_read_dword(Memory *mem, uint32_t address);
void     memory_write_byte (Memory *mem, uint32_t address, uint8_t  value);
void     memory_write_word (Memory *mem, uint32_t address, uint16_t value);
void     memory_write_dword(Memory *mem, uint32_t address, uint32_t value);
```

### Implementation details

**`memory_init()`** — calls `memset(mem->data, 0, MEMORY_SIZE)` and clears `mem->error`.

**Bounds check (used by every read/write):**
```c
if (address >= MEMORY_SIZE) {
    fprintf(stderr, "Memory error: address 0x%08X out of bounds\n", address);
    mem->error = true;
    return 0;  // safe default for reads; writes are silently dropped
}
```

**Word/dword reads** compose two or four `memory_read_byte()` calls, shifting and ORing each byte into the result:
```c
uint32_t memory_read_dword(Memory *mem, uint32_t address) {
    uint32_t b0 = memory_read_byte(mem, address);
    uint32_t b1 = memory_read_byte(mem, address + 1);
    uint32_t b2 = memory_read_byte(mem, address + 2);
    uint32_t b3 = memory_read_byte(mem, address + 3);
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}
```

**Word/dword writes** decompose the value into individual bytes:
```c
void memory_write_dword(Memory *mem, uint32_t address, uint32_t value) {
    memory_write_byte(mem, address,     value & 0xFF);
    memory_write_byte(mem, address + 1, (value >> 8)  & 0xFF);
    memory_write_byte(mem, address + 2, (value >> 16) & 0xFF);
    memory_write_byte(mem, address + 3, (value >> 24) & 0xFF);
}
```

### Key files
- [src/memory.h](../src/memory.h)
- [src/memory.c](../src/memory.c)

---

## Step 3: Instruction Set Design (ISA)

### Goal
Define the complete set of opcodes the VM understands, grouped by category, and provide a helper to convert opcode bytes to readable names.

### Opcode table

All opcodes are 1 byte wide. Operands follow inline in the instruction stream.

| Category | Mnemonic | Opcode | Operands | Description |
|----------|----------|--------|----------|-------------|
| Data move | `NOP` | `0x00` | — | No operation |
| Data move | `MOV` | `0x01` | reg, imm32 | Load immediate into register |
| Data move | `MOVR` | `0x02` | reg, reg | Copy register to register |
| Arithmetic | `ADD` | `0x10` | reg, reg | reg[0] += reg[1] |
| Arithmetic | `SUB` | `0x11` | reg, reg | reg[0] -= reg[1] |
| Arithmetic | `MUL` | `0x12` | reg, reg | reg[0] *= reg[1] |
| Arithmetic | `DIV` | `0x13` | reg, reg | reg[0] /= reg[1] |
| Arithmetic | `INC` | `0x14` | reg | reg++ |
| Arithmetic | `DEC` | `0x15` | reg | reg-- |
| Logic | `AND` | `0x20` | reg, reg | reg[0] &= reg[1] |
| Logic | `OR` | `0x21` | reg, reg | reg[0] \|= reg[1] |
| Logic | `XOR` | `0x22` | reg, reg | reg[0] ^= reg[1] |
| Logic | `NOT` | `0x23` | reg | reg = ~reg |
| Compare | `CMP` | `0x30` | reg, reg | Sets ZERO/NEGATIVE flags |
| Jump | `JMP` | `0x40` | addr32 | Unconditional jump |
| Jump | `JE` | `0x41` | addr32 | Jump if ZERO flag set |
| Jump | `JNE` | `0x42` | addr32 | Jump if ZERO flag clear |
| Jump | `JG` | `0x43` | addr32 | Jump if not ZERO and not NEGATIVE |
| Jump | `JL` | `0x44` | addr32 | Jump if NEGATIVE |
| Stack | `PUSH` | `0x50` | reg | Push register onto stack |
| Stack | `POP` | `0x51` | reg | Pop top of stack into register |
| Function | `CALL` | `0x60` | addr32 | Push return address, jump |
| Function | `RET` | `0x61` | — | Pop return address, jump |
| Memory | `LOAD` | `0x70` | reg, addr32 | reg = mem[addr] (dword) |
| Memory | `STOR` | `0x71` | addr32, reg | mem[addr] = reg (dword) |
| I/O | `PRINT` | `0x80` | reg | Print register value as integer |
| I/O | `READ` | `0x81` | reg | Read integer from input into reg |
| Control | `HALT` | `0xFF` | — | Stop the VM |

### Instruction encoding

Each instruction in the binary file is encoded sequentially:
- **1 byte** — opcode
- **1 byte** — register index (0–7) where needed
- **4 bytes** — 32-bit immediate / address (little-endian) where needed

Example encoding for `MOV R0, 42`:
```
01  00  2A 00 00 00
^   ^   ^---------^
op  R0  42 (LE dword)
```

### Flag semantics

After `CMP reg_a, reg_b` (which computes `reg_a - reg_b` without storing):

| Condition | Flags |
|-----------|-------|
| `reg_a == reg_b` | `zero = true`, `negative = false` |
| `reg_a < reg_b` | `zero = false`, `negative = true` |
| `reg_a > reg_b` | `zero = false`, `negative = false` |

Arithmetic instructions (`ADD`, `SUB`, `MUL`, `DIV`, `INC`, `DEC`) also update flags based on the result.

### Helper function

```c
const char *opcode_name(uint8_t opcode);
```

Returns a human-readable string (`"MOV"`, `"ADD"`, …) for debugging and the GUI disassembly view. Returns `"UNKNOWN"` for any undefined opcode byte.

### Key files
- [src/instructions.h](../src/instructions.h)
- [src/instructions.c](../src/instructions.c)
