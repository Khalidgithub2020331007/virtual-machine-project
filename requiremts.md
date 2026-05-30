# Virtual Machine Project

## Project Info
- **Language**: C / C++
- **Platform**: Linux
- **Deadline**: 1 month
- **Type**: University Assignment
- **Developer**: Khalid

---

## Functional Requirements

### 1. CPU (Core)
- [ ] Registers (general purpose, program counter, stack pointer, flags)
- [ ] Fetch → Decode → Execute cycle
- [ ] Arithmetic instructions (ADD, SUB, MUL, DIV)
- [ ] Logic instructions (AND, OR, XOR, NOT)
- [ ] Comparison instructions (CMP, TEST)
- [ ] Jump instructions (JMP, JE, JNE, JG, JL)
- [ ] PUSH / POP stack operations
- [ ] CALL / RET for function calls
- [ ] HALT instruction to stop the VM

### 2. Memory
- [ ] Simulated RAM (array in C/C++)
- [ ] Memory read / write operations
- [ ] Stack memory region
- [ ] Program memory region (where instructions load)

### 3. Instruction Set (ISA)
- [ ] Define your own instruction set (opcodes)
- [ ] Fixed or variable length instructions
- [ ] Instruction encoding/decoding

### 4. I/O / User Interface
- Primary: Graphical user interface (GUI) for interactive control and visualization
      - Use SDL2 (or a similar lightweight cross-platform library) for windowing and rendering
      - Visualize CPU registers, flags, program counter, stack pointer, and a memory hex/byte view
      - Controls: Load program, Run, Pause, Step, Reset, Set breakpoints
      - Console area for program stdout and keyboard input (fallback to terminal input when needed)
      - Optional: visual timeline / instruction trace and register change highlighting
- Secondary: Command-line interface (CLI) fallback
      - Support running the VM headless from the terminal for automated tests and grading
      - Accept program filename as a command-line argument and provide text-mode I/O

### 5. Program Loader
- [ ] Load a binary/text program file into VM memory
- [ ] Start execution from entry point

---

## Non-Functional Requirements
- Written in C or C++
- Runs on Linux (primary) — GUI should work on Linux; cross-platform is optional
- Allow one small external dependency for GUI: SDL2 (acceptable for the GUI mode)
- Clean modular code (separate files per component)
- Clear separation between emulator backend (CPU/memory/loader) and UI frontend
- Error handling (invalid opcode, memory overflow, etc.) and graceful UI error messages

---

## Deliverables
- [ ] Source code (well commented)
- [ ] A sample program that runs on your VM
- [ ] Documentation / report
- [ ] Demo (run a simple program: print numbers, factorial, fibonacci)

---

## Project Structure
```
vm-project/
├── src/
│   ├── main.c
│   ├── cpu.c / cpu.h
│   ├── memory.c / memory.h
│   ├── instructions.c / instructions.h
│   ├── loader.c / loader.h
│   └── io.c / io.h
├── programs/
│   └── hello.asm
├── Makefile
├── CLAUDE.md
└── README.md
```

---

## Bonus (if time allows)
- [ ] Simple assembler to write programs for your VM
- [ ] Extra GUI features: graphical memory map, hex editor, visual debugger
- [ ] Debugger (step through instructions with UI support)

---

## Architecture Overview
```
[ Program File ]
      ↓
[ Loader ] → loads into Memory
      ↓
[ CPU: Fetch → Decode → Execute ]
      ↓
[ Memory Read/Write ]
      ↓
[ I/O: Terminal Output/Input ]
```

---

## Notes
- VM type: Software-based system emulator (no hardware virtualization)
- Simulated RAM size: 1MB - 16MB (defined as array in C)
- Custom ISA (design our own instruction set / opcodes)
- Stack grows downward from top of memory

## LLM / AI usage (development assistance)
- Use of large language models (LLMs) or AI tools is permitted as a development aid (for example: code snippets, documentation, test-generation, debugging help, or design suggestions).
- These tools are for developer assistance only and are not required to be integrated into the delivered VM runtime. If you choose to integrate an LLM into the runtime, that must be explicitly documented and justified in the report.
- Document any substantive content produced with LLMs in the final report: which tool/model was used, prompts or queries (redact secrets), and how the output was validated or edited.
- Do not commit API keys or secrets to the repository; store them securely outside source control.

## Advanced / Optional Features (time-permitting)
These items are not required for the core deliverable but are good targets if you have extra time.

- Visual debugger: step/inspect with breakpoints, watch expressions, and change-history highlighting.
- Assembler IDE: an editor view to write assembly and assemble directly into memory inside the GUI.
- Graphical memory inspector with editable bytes and search/filter capabilities.
- Snapshot and restore of VM state (save/restore memory + registers).
- Peripheral simulations (simple disk, keyboard buffers, serial console) for richer demos.
- Assembler/disassembler pair with full ISA reference and annotations.
- JIT or optimized execution path for performance (experimental).
- Scripting for automated demos (record/replay UI actions or instruction traces).
- Unit test harness and CI integration for continuous verification.

