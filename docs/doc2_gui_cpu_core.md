# Documentation — Phase 2: GUI Skeleton & CPU Core

**Steps covered:** Step 4, Step 5, Step 6, Step 7

---

## Step 4: GUI Skeleton

### Goal
Create a minimal SDL2 window with an event loop and placeholder rendering panels so that all later UI integration has a concrete shell to fill in.

### Layout design

```
+----------------------------------------------------------+
|  [Load]  [Run]  [Pause]  [Step]  [Reset]    (toolbar)   |
+------------------+---------------------------------------+
|  Registers       |  Memory Hex View                     |
|  R0: 0x00000000  |  0x000000: 00 00 00 00  00 00 00 00  |
|  R1: 0x00000000  |  0x000008: 00 00 00 00  00 00 00 00  |
|  ...             |  ...                                 |
|  PC: 0x00000000  |                                      |
|  SP: 0xFFFF0000  |                                      |
|  Flags: Z=0 N=0  |                                      |
+------------------+---------------------------------------+
|  Console output                                          |
|  > Program loaded at 0x0000 (128 bytes)                  |
+----------------------------------------------------------+
```

### SDL2 initialisation

```c
SDL_Init(SDL_INIT_VIDEO);
TTF_Init();
SDL_Window   *window   = SDL_CreateWindow("VM", SDL_WINDOWPOS_CENTERED,
                           SDL_WINDOWPOS_CENTERED, 1024, 768, SDL_WINDOW_RESIZABLE);
SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
```

### Event loop structure

```c
SDL_Event event;
while (gui_running) {
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) gui_running = false;
        gui_handle_event(&event);   // button clicks, keyboard shortcuts
    }
    gui_render(renderer);           // clear → draw panels → present
    SDL_Delay(16);                  // ~60 fps cap
}
```

### UI panels

| Panel | Position | Content |
|-------|----------|---------|
| Toolbar | Top strip | 5 button rectangles with text labels |
| Register sidebar | Left column | Register name + hex value per row |
| Memory view | Main area | Scrollable hex dump (16 bytes per row) |
| Console | Bottom strip | Scrollable text output from the VM |

### Key files
- [src/gui.h](../src/gui.h)
- [src/gui.c](../src/gui.c)

---

## Step 5: CPU Registers & State

### Goal
Define the full CPU state structure — registers, program counter, stack pointer, flags — and the initialisation function that resets everything to a known-good state.

### CPU state structure

```c
// cpu.h
#define NUM_REGISTERS 8
#define STACK_START   0xFFFF0  // Stack grows downward from here

typedef enum { R0=0, R1, R2, R3, R4, R5, R6, R7 } Register;

typedef struct {
    bool zero;      // result == 0
    bool negative;  // result < 0 (or reg_a < reg_b after CMP)
    bool overflow;  // arithmetic overflow
} Flags;

typedef struct {
    uint32_t reg[NUM_REGISTERS]; // R0–R7
    uint32_t pc;                 // Program Counter — next instruction address
    uint32_t sp;                 // Stack Pointer — grows downward
    Flags    flags;
    bool     running;            // false → VM halted
    Memory  *mem;                // shared memory, not owned
} CPU;
```

### Initialisation

```c
void cpu_init(CPU *cpu, Memory *mem) {
    memset(cpu->reg, 0, sizeof(cpu->reg));
    cpu->pc      = 0;
    cpu->sp      = STACK_START;
    cpu->flags   = (Flags){false, false, false};
    cpu->running = false;
    cpu->mem     = mem;
}
```

Key points:
- All registers start at **0**.
- `pc` starts at **0x000000** — programs are loaded there by default.
- `sp` starts at **0xFFFF0** and decrements on each `PUSH`.
- `running` is `false` until `cpu_run()` sets it; calling `cpu_step()` before loading a program is safe.

### Key files
- [src/cpu.h](../src/cpu.h)
- [src/cpu.c](../src/cpu.c)

---

## Step 6: Fetch → Decode → Execute Cycle

### Goal
Implement the three core CPU functions: `cpu_step()` (one instruction cycle), `cpu_run()` (run until HALT), and `cpu_dump()` (state snapshot for debugging/UI).

### Fetch helpers

```c
// Read 1 byte from pc, advance pc by 1
static uint8_t fetch_byte(CPU *cpu) {
    uint8_t b = memory_read_byte(cpu->mem, cpu->pc);
    cpu->pc++;
    return b;
}

// Read 4 bytes from pc, advance pc by 4
static uint32_t fetch_dword(CPU *cpu) {
    uint32_t d = memory_read_dword(cpu->mem, cpu->pc);
    cpu->pc += 4;
    return d;
}
```

### `cpu_step()` — one fetch-decode-execute cycle

```c
void cpu_step(CPU *cpu) {
    if (!cpu->running) return;

    uint8_t opcode = fetch_byte(cpu);   // FETCH

    switch (opcode) {                   // DECODE + EXECUTE
        case OP_MOV: {
            uint8_t  reg = fetch_byte(cpu);
            uint32_t val = fetch_dword(cpu);
            cpu->reg[reg] = val;
            break;
        }
        // ... all other opcodes
        case OP_HALT:
            cpu->running = false;
            break;
        default:
            fprintf(stderr, "Unknown opcode 0x%02X at PC=0x%08X\n",
                    opcode, cpu->pc - 1);
            cpu->running = false;
    }

    if (cpu->mem->error) {
        fprintf(stderr, "Memory error — halting\n");
        cpu->running = false;
    }
}
```

### `cpu_run()` — loop until HALT

```c
void cpu_run(CPU *cpu) {
    cpu->running = true;
    while (cpu->running) {
        cpu_step(cpu);
    }
}
```

In **GUI mode** the run loop is driven by the UI event loop instead: each press of the Step button calls `cpu_step()` once; Run mode calls `cpu_step()` on every frame until `cpu->running` becomes false.

### `cpu_dump()` — register snapshot

```c
void cpu_dump(CPU *cpu) {
    printf("=== CPU Dump ===\n");
    for (int i = 0; i < NUM_REGISTERS; i++)
        printf("  R%d = 0x%08X (%u)\n", i, cpu->reg[i], cpu->reg[i]);
    printf("  PC = 0x%08X\n", cpu->pc);
    printf("  SP = 0x%08X\n", cpu->sp);
    printf("  Flags: ZERO=%d NEGATIVE=%d OVERFLOW=%d\n",
           cpu->flags.zero, cpu->flags.negative, cpu->flags.overflow);
}
```

The GUI calls a similar function to refresh the register sidebar after each step.

### Key files
- [src/cpu.h](../src/cpu.h)
- [src/cpu.c](../src/cpu.c)

---

## Step 7: Implement All Instructions in the CPU

### Goal
Fill in every `case` in `cpu_step()` so the VM can execute the full ISA defined in Step 3.

### Data movement

```c
case OP_MOV: {           // MOV reg, imm32
    uint8_t  dst = fetch_byte(cpu);
    uint32_t val = fetch_dword(cpu);
    cpu->reg[dst] = val;
    break;
}
case OP_MOVR: {          // MOVR reg, reg
    uint8_t dst = fetch_byte(cpu);
    uint8_t src = fetch_byte(cpu);
    cpu->reg[dst] = cpu->reg[src];
    break;
}
```

### Arithmetic — common pattern with flag update

```c
static void update_flags(CPU *cpu, uint32_t result) {
    cpu->flags.zero     = (result == 0);
    cpu->flags.negative = ((int32_t)result < 0);
    // overflow is set individually by MUL/DIV
}

case OP_ADD: {
    uint8_t dst = fetch_byte(cpu);
    uint8_t src = fetch_byte(cpu);
    cpu->reg[dst] += cpu->reg[src];
    update_flags(cpu, cpu->reg[dst]);
    break;
}
// SUB, MUL follow the same pattern; DIV checks for divide-by-zero
case OP_DIV: {
    uint8_t dst = fetch_byte(cpu);
    uint8_t src = fetch_byte(cpu);
    if (cpu->reg[src] == 0) {
        fprintf(stderr, "Division by zero at PC=0x%08X\n", cpu->pc);
        cpu->running = false;
        break;
    }
    cpu->reg[dst] /= cpu->reg[src];
    update_flags(cpu, cpu->reg[dst]);
    break;
}
```

### Comparison

```c
case OP_CMP: {
    uint8_t a = fetch_byte(cpu);
    uint8_t b = fetch_byte(cpu);
    int32_t diff = (int32_t)cpu->reg[a] - (int32_t)cpu->reg[b];
    cpu->flags.zero     = (diff == 0);
    cpu->flags.negative = (diff < 0);
    break;
}
```

### Jumps — read address, set PC conditionally

```c
case OP_JMP: { uint32_t addr = fetch_dword(cpu); cpu->pc = addr; break; }
case OP_JE:  { uint32_t addr = fetch_dword(cpu); if (cpu->flags.zero)     cpu->pc = addr; break; }
case OP_JNE: { uint32_t addr = fetch_dword(cpu); if (!cpu->flags.zero)    cpu->pc = addr; break; }
case OP_JG:  { uint32_t addr = fetch_dword(cpu); if (!cpu->flags.zero && !cpu->flags.negative) cpu->pc = addr; break; }
case OP_JL:  { uint32_t addr = fetch_dword(cpu); if (cpu->flags.negative) cpu->pc = addr; break; }
```

### Stack

```c
case OP_PUSH: {
    uint8_t src = fetch_byte(cpu);
    cpu->sp -= 4;
    if (cpu->sp < 0x1000) { /* stack overflow */ cpu->running = false; break; }
    memory_write_dword(cpu->mem, cpu->sp, cpu->reg[src]);
    break;
}
case OP_POP: {
    uint8_t dst = fetch_byte(cpu);
    if (cpu->sp >= STACK_START) { /* stack underflow */ cpu->running = false; break; }
    cpu->reg[dst] = memory_read_dword(cpu->mem, cpu->sp);
    cpu->sp += 4;
    break;
}
```

### Function calls

```c
case OP_CALL: {
    uint32_t addr = fetch_dword(cpu);
    cpu->sp -= 4;
    memory_write_dword(cpu->mem, cpu->sp, cpu->pc);   // push return address
    cpu->pc = addr;
    break;
}
case OP_RET: {
    cpu->pc = memory_read_dword(cpu->mem, cpu->sp);   // pop return address
    cpu->sp += 4;
    break;
}
```

### Memory access

```c
case OP_LOAD: {
    uint8_t  dst  = fetch_byte(cpu);
    uint32_t addr = fetch_dword(cpu);
    cpu->reg[dst] = memory_read_dword(cpu->mem, addr);
    break;
}
case OP_STOR: {
    uint32_t addr = fetch_dword(cpu);
    uint8_t  src  = fetch_byte(cpu);
    memory_write_dword(cpu->mem, addr, cpu->reg[src]);
    break;
}
```

### I/O

```c
case OP_PRINT: {
    uint8_t src = fetch_byte(cpu);
    io_print_int(cpu->reg[src]);    // routes to GUI console or stdout
    break;
}
case OP_READ: {
    uint8_t dst = fetch_byte(cpu);
    cpu->reg[dst] = io_read_int();  // GUI modal or stdin
    break;
}
```

### Key files
- [src/cpu.c](../src/cpu.c) — all instruction implementations
- [src/instructions.h](../src/instructions.h) — opcode enum
- [src/io.h](../src/io.h) — I/O callback interface
