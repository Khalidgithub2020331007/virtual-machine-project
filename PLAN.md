# Virtual Machine Project — Step-by-Step Plan

**Developer**: Khalid
**Deadline**: 1 Month
**Language**: C / C++
**Platform**: Linux

---

## Overview

Build a GUI-first software Virtual Machine (System Emulator) from scratch in C/C++.
The VM backend (CPU, Memory, Loader, I/O) will be decoupled from a GUI frontend that provides visualization and controls. A CLI fallback mode will be kept for automated tests and grading.

### Use of LLM/AI tools during development
- LLMs or AI assistants may be used as development aids (code generation, docs, test generation, design suggestions).
- These tools are explicitly permitted to assist the developer, but they are not required to be integrated into the VM runtime.
- When LLMs are used, document the usage in the report and in a `LLM_USAGE.md` file: model/service, prompts, and how outputs were verified.

---

## Phase 1 — Foundation (Week 1)

### Step 1: Project Setup & GUI dependencies
- [x] Create project folder structure (`src/`, `programs/`)
- [x] Create `Makefile` for easy building
- [x] Create `CLAUDE.md` with all requirements
- [x] Create `PLAN.md` (this file)
- [x] Add build notes for SDL2 (pkg-config or distro package) and document how to install it on Linux

### Step 2: Memory Module
- [x] Define memory size (1MB RAM)
- [x] Implement `memory_init()` — zero out all memory
- [x] Implement `memory_read_byte()` — read 1 byte
- [x] Implement `memory_read_word()` — read 2 bytes
- [x] Implement `memory_read_dword()` — read 4 bytes
- [x] Implement `memory_write_byte()` — write 1 byte
- [x] Implement `memory_write_word()` — write 2 bytes
- [x] Implement `memory_write_dword()` — write 4 bytes
- [x] Add out-of-bounds error handling

### Step 3: Instruction Set Design (ISA)
- [x] Define all opcodes as enum
- [x] Data movement: `MOV`, `MOVR`
- [x] Arithmetic: `ADD`, `SUB`, `MUL`, `DIV`, `INC`, `DEC`
- [x] Logic: `AND`, `OR`, `XOR`, `NOT`
- [x] Comparison: `CMP`
- [x] Jumps: `JMP`, `JE`, `JNE`, `JG`, `JL`
- [x] Stack: `PUSH`, `POP`
- [x] Functions: `CALL`, `RET`
- [x] Memory: `LOAD`, `STOR`
- [x] I/O: `PRINT`, `READ` (UI integration planned)
- [x] Control: `HALT`, `NOP`
- [x] Implement `opcode_name()` for debugging

---

## Phase 2 — GUI Skeleton & CPU Core (Week 2)

### Step 4: GUI Skeleton
- [x] Create a minimal SDL2 window and event loop
- [x] Design basic UI layout: sidebar for registers/flags, main area for memory, bottom console and control toolbar
- [x] Implement placeholder rendering for register values and memory hex view
- [x] Add UI controls: Load, Run, Pause, Step, Reset

### Step 5: CPU Registers & State
- [x] Define 8 general purpose registers (R0 - R7)
- [x] Define Program Counter (PC)
- [x] Define Stack Pointer (SP)
- [x] Define CPU Flags (ZERO, NEGATIVE, OVERFLOW)
- [x] Implement `cpu_init()` — reset all registers and flags

### Step 6: Fetch → Decode → Execute Cycle
- [x] Implement `fetch_byte()` — read next instruction byte, advance PC
- [x] Implement `fetch_dword()` — read next 4 bytes, advance PC
- [x] Implement `cpu_step()` — one full fetch-decode-execute cycle
- [x] Implement `cpu_run()` — loop until HALT (support step mode for UI)
- [x] Implement `cpu_dump()` — provide programmatic register dump for UI

### Step 7: Implement All Instructions in CPU
- [x] Data movement (MOV, MOVR)
- [x] Arithmetic (ADD, SUB, MUL, DIV, INC, DEC)
- [x] Logic (AND, OR, XOR, NOT)
- [x] Comparison (CMP) with flag updates
- [x] Jumps (JMP, JE, JNE, JG, JL)
- [x] Stack (PUSH, POP)
- [x] Function calls (CALL, RET)
- [x] Memory access (LOAD, STOR)
- [x] I/O (PRINT, READ) — integrate with UI console
- [x] HALT and error for unknown opcodes

---

## Phase 3 — Loader, Backend I/O & UI Integration (Week 2-3)

### Step 8: Program Loader
- [x] Implement `loader_load_file()` — read binary file into memory
- [x] Load program at a specified start address
- [x] Show load confirmation in UI (address, size) and in CLI
- [x] Handle file not found error
- [x] Handle program too large error

### Step 9: Backend I/O Module
- [x] Implement `io_print_int()` — deliver output to UI console and terminal
- [x] Implement `io_print_string()` — deliver output to UI console and terminal
- [x] Implement `io_read_int()` — request input via UI modal or fallback to terminal

### Step 10: Main Entry Point & UI glue
- [x] Wire Memory + CPU + Loader together in `main.c`
- [x] Initialize GUI when `--gui` flag is provided; otherwise use CLI mode
- [x] Provide command-line flags: `--gui`, `--debug`, `--headless`, `--program <file>`
- [x] Provide UI callbacks for Run/Pause/Step that control the backend

---

## Phase 4 — Test Programs (Week 3)

### Step 11: Write Test Programs (Binary) and UI tests
- [x] Basic test: `R0 = 10 + 20`, print result (= 30)
- [x] Countdown: print numbers from 10 down to 0 (verify UI console)
- [x] Factorial: calculate 5! = 120 using CALL/RET (verify UI registers/memory)
- [x] Fibonacci: print first 10 fibonacci numbers (verify UI console)
- [x] Input test: read a number, multiply by 2, print result (verify UI input modal)

### Step 12: Build a Simple Assembler (Text → Binary)
- [x] Define assembly text format (e.g. `MOV R0, 10`)
- [x] Write assembler in C that reads `.asm` file
- [x] Convert each text instruction to binary opcode
- [x] Output `.bin` file ready for the VM
- [x] Test assembler with all sample programs

---

## Phase 5 — Polish, UI Enhancements & Report (Week 4)

### Step 13: Error Handling & Stability
- [x] Handle all invalid opcodes gracefully (show in UI and logs)
- [x] Handle stack overflow / underflow (show warnings in UI)
- [x] Handle division by zero (already done)
- [x] Handle memory out-of-bounds (already done)
- [x] Add input validation in loader

### Step 14: Debugging Tools (UI-enabled)
- [ ] Add `--debug` flag and GUI debug mode to step through instructions one by one
- [ ] Show disassembly trace in UI and allow clicking to jump to addresses
- [ ] Show register state and memory changes after each step with highlights
- [ ] Add breakpoints support via UI

### Step 15: Documentation & Report
- [ ] Write `README.md` (how to build and run, GUI vs CLI instructions)
- [ ] Document the instruction set (ISA reference table)
- [ ] Document UI features and keyboard shortcuts
- [ ] Draw architecture diagram
- [ ] Write project report:
  - Introduction
  - Architecture Design
  - Instruction Set Description
  - Memory Model
  - GUI design and screenshots
  - Test Results
  - Conclusion

### Step 16: Final Demo Preparation
- [ ] Prepare GUI demo showing: loading a program, running, stepping, and using debugger features
- [ ] Prepare CLI demo for automated grading
- [ ] Clean up code and comments

## Advanced Implementation (if time allows)

These tasks are optional and should only be attempted after the core features are stable.

- Add visual debugger features: conditional breakpoints, watchlists, and stepping over/into calls.
- Integrate an in-GUI assembler/editor with syntax highlighting and one-click assemble/load.
- Implement snapshot/save-state and restore features for demonstration and grading.
- Add peripheral simulations (simple file-backed disk, UART/serial) and example programs that use them.
- Build an automated demo recorder to capture UI interactions for reproducible demos.
- Prototype a simple JIT for hot paths (advanced, optional, risk: increases complexity).
- Add CI (GitHub Actions) to run build and core emulator tests on push.


---

## Summary Timeline

| Week | Phase | Goal |
|------|-------|------|
| Week 1 | Foundation | Memory, ISA, project setup |
| Week 2 | CPU Core | Registers, fetch-decode-execute, all instructions |
| Week 3 | Test Programs + Assembler | Run real programs, write assembler |
| Week 4 | Polish + Report | Error handling, debugger, documentation, demo |

---

## Evaluation, Reproducibility & Deliverables

- Evaluation criteria:
  - Correctness: emulator passes a suite of functional tests (arithmetic, memory access, stack/calls, jumps).
  - Stability: handles invalid opcodes and memory errors gracefully.
  - Performance: reasonable execution speed for simple programs (no strict real-time requirement).
  - Usability (GUI): user can load a program, run/pause/step, and view register/memory state.

- Reproducibility checklist:
  - Include exact dependency list and install commands (in `README.md`).
  - Provide sample programs in `programs/` and scripts to run tests (e.g. `scripts/run_tests.sh`).
  - Document any LLM/AI assistance in `LLM_USAGE.md`.

- Final deliverables:
  - Runnable repository with `Makefile`, `README.md`, and sample programs.
  - GUI demo (recorded video or screenshots) showing core features.
  - Project report with architecture, ISA, test results, and LLM usage notes.


## Current Status

| Module | Status |
|--------|--------|
| Project Setup | Done |
| Memory Module | Done |
| Instruction Set | Done |
| CPU Core | Done |
| Program Loader | Done |
| I/O Module | Done |
| Basic Test (10+20=30) | Done |
| Test Programs (countdown, factorial, fibonacci, input) | Done |
| Assembler | Done |
| Debugger | Pending |
| Report | Pending |
