# Documentation — Phase 5: Error Handling & Stability

**Steps covered:** Step 13

---

## Step 13: Error Handling & Stability

### Goal
Ensure the VM halts gracefully — never segfaults, never produces silent wrong results — when it encounters invalid opcodes, stack overflow/underflow, division by zero, memory out-of-bounds, or bad loader input.

---

### Overview: error propagation model

All errors in the VM follow the same pattern:

1. **Detect** — boundary check or validity test at the point of failure.
2. **Report** — `fprintf(stderr, ...)` for VM-level errors; `io_print_string()` routes the message to both the GUI console and stderr.
3. **Halt** — set `cpu->running = false` (or return an error code for the loader) so execution stops cleanly without crashing.

The GUI shows every error message in the console panel in red so the user can see exactly what went wrong without reading a terminal.

---

### 1. Invalid opcodes

**Where it happens:** `cpu_step()` — the `default:` case of the opcode `switch`.

**Implementation:**
```c
default: {
    char msg[64];
    snprintf(msg, sizeof(msg),
             "[ERROR] Unknown opcode 0x%02X at PC=0x%08X", opcode, cpu->pc - 1);
    cpu_error(cpu, msg);   // prints + sets cpu->running = false
    break;
}
```

**Effect:** VM halts immediately; the bad opcode byte and its address are reported. In GUI mode the message appears in the console panel.

---

### 2. Stack overflow

**Where it happens:** `OP_PUSH` and `OP_CALL` — before decrementing SP.

**Implementation:**
```c
// OP_PUSH
case OP_PUSH: {
    uint8_t reg = fetch_byte(cpu);
    if (cpu->sp < 4) {
        char msg[64];
        snprintf(msg, sizeof(msg),
                 "[WARNING] Stack overflow at PC=0x%08X", cpu->pc - 2);
        cpu_error(cpu, msg);
        break;
    }
    cpu->sp -= 4;
    memory_write_dword(cpu->mem, cpu->sp, cpu->reg[reg & 0x07]);
    break;
}
```

The same guard is applied in `OP_CALL` (which also pushes the return address).

**Why `sp < 4`:** SP decrements by 4 before the write; if `sp` is already less than 4 the subtraction would underflow the address space, potentially overwriting program code. The check prevents this.

---

### 3. Stack underflow

**Where it happens:** `OP_POP` and `OP_RET` — before reading from SP.

**Implementation:**
```c
case OP_POP: {
    uint8_t reg = fetch_byte(cpu);
    if (cpu->sp >= STACK_START) {
        char msg[64];
        snprintf(msg, sizeof(msg),
                 "[WARNING] Stack underflow at PC=0x%08X", cpu->pc - 2);
        cpu_error(cpu, msg);
        break;
    }
    cpu->reg[reg & 0x07] = memory_read_dword(cpu->mem, cpu->sp);
    cpu->sp += 4;
    break;
}
```

`STACK_START = 0xFFFF0` — if SP has never been decremented (i.e. nothing was ever pushed) or a mismatched number of pops has been executed, `sp >= STACK_START` detects the empty stack condition.

---

### 4. Division by zero

**Where it happens:** `OP_DIV`.

**Implementation:**
```c
case OP_DIV: {
    uint8_t dst = fetch_byte(cpu);
    uint8_t src = fetch_byte(cpu);
    if (cpu->reg[src & 0x07] == 0) {
        char msg[64];
        snprintf(msg, sizeof(msg),
                 "[ERROR] Division by zero at PC=0x%08X", cpu->pc);
        cpu_error(cpu, msg);
        break;
    }
    int32_t result = (int32_t)cpu->reg[dst & 0x07] / (int32_t)cpu->reg[src & 0x07];
    cpu->reg[dst & 0x07] = (uint32_t)result;
    update_flags(cpu, result);
    break;
}
```

**Why check before dividing:** On x86 Linux a divide-by-zero raises `SIGFPE` and crashes the process. The explicit check catches it before the C division and converts it to a clean VM halt with a meaningful error message.

---

### 5. Memory out-of-bounds

**Where it happens:** Every `memory_read_*` and `memory_write_*` call.

**Implementation (read_byte example):**
```c
uint8_t memory_read_byte(Memory *mem, uint32_t address) {
    if (!mem) {
        fprintf(stderr, "[ERROR] memory_read_byte: NULL pointer\n");
        return 0;
    }
    if (address >= MEMORY_SIZE) {
        fprintf(stderr, "[ERROR] Memory read out of bounds at 0x%08X\n", address);
        mem->error = true;
        return 0;
    }
    return mem->data[address];
}
```

After every `cpu_step()` the CPU checks `cpu->mem->error` and halts if it is set:

```c
// At the end of cpu_step():
if (cpu->mem->error) {
    cpu_error(cpu, "[ERROR] Memory error — halting");
}
```

The `error` flag approach (rather than immediate abort) lets the full error message be printed before the VM stops, and keeps the memory module free of CPU-level logic.

---

### 6. Loader input validation

**Where it happens:** `loader_load_file()`.

| Check | Condition | Message |
|-------|-----------|---------|
| NULL arguments | `!mem \|\| !filename` | `"Loader: null argument"` |
| File not found | `fopen()` returns NULL | `"Loader: cannot open '<file>': <errno>"` |
| Empty file | `ftell() <= 0` | `"Loader: file '<file>' is empty"` |
| Too large | `start + size > MEMORY_SIZE` | `"Loader: program too large (N bytes) for address 0x…"` |
| Partial read | `fread()` bytes < expected | `"Loader: partial read (X of Y bytes)"` |

All error paths close the file handle before returning `-1`, preventing resource leaks.

---

### 7. NULL pointer guards

Every public function in the VM checks its pointer arguments before dereferencing them:

```c
void cpu_init(CPU *cpu, Memory *mem) {
    if (!cpu || !mem) {
        fprintf(stderr, "[ERROR] cpu_init: NULL pointer\n");
        return;
    }
    // ...
}

void cpu_step(CPU *cpu) {
    if (!cpu || !cpu->mem) {
        fprintf(stderr, "[ERROR] cpu_step: NULL pointer\n");
        return;
    }
    // ...
}
```

These guards prevent segfaults when the VM is called from test harnesses or GUI code that constructs objects in a non-standard order.

---

### 8. Register index masking

Whenever a register index is fetched from the instruction stream it is masked with `0x07` before use:

```c
cpu->reg[reg & 0x07]
```

Even if a binary program contains an out-of-range register byte (e.g. `0x09`), the mask clamps it to `0x01`, avoiding an array-out-of-bounds access. The VM does not crash; instead it silently uses the clamped register. Future work could add an explicit warning here.

---

### Summary of error conditions and VM responses

| Error | Detection point | VM response |
|-------|----------------|-------------|
| Unknown opcode | `cpu_step()` default case | Print + halt |
| Stack overflow | `PUSH`, `CALL` before SP decrement | Print warning + halt |
| Stack underflow | `POP`, `RET` before SP increment | Print warning + halt |
| Division by zero | `DIV` before divide | Print error + halt |
| Memory OOB read | `memory_read_*` | Set `mem->error`, return 0 → halt on next step |
| Memory OOB write | `memory_write_*` | Set `mem->error`, drop write → halt on next step |
| File not found | `loader_load_file` | Print error, return -1 (no program loaded) |
| Program too large | `loader_load_file` | Print error, return -1 |
| NULL pointer | Any public API | `fprintf(stderr)`, early return |

### Key files
- [src/cpu.c](../src/cpu.c) — all CPU-level error checks
- [src/memory.c](../src/memory.c) — memory boundary checks
- [src/loader.c](../src/loader.c) — loader validation
