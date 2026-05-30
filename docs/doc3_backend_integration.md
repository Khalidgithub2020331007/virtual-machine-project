# Documentation — Phase 3: Loader, Backend I/O & UI Integration

**Steps covered:** Step 8, Step 9, Step 10

---

## Step 8: Program Loader

### Goal
Read a raw binary file from disk and copy its bytes into VM memory at a specified start address, with clear error messages for common failure modes.

### API

```c
// loader.h
int loader_load_file(Memory *mem, const char *filename, uint32_t start_address);
```

Returns `0` on success, `-1` on any error.

### Implementation

```c
int loader_load_file(Memory *mem, const char *filename, uint32_t start_address) {
    if (!mem || !filename) {
        fprintf(stderr, "Loader: null argument\n");
        return -1;
    }

    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Loader: cannot open '%s': %s\n", filename, strerror(errno));
        return -1;
    }

    // Determine file size
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    if (size <= 0) {
        fprintf(stderr, "Loader: file '%s' is empty\n", filename);
        fclose(f);
        return -1;
    }

    // Check that the program fits in memory
    if ((uint64_t)start_address + (uint64_t)size > MEMORY_SIZE) {
        fprintf(stderr, "Loader: program too large (%ld bytes) for address 0x%08X\n",
                size, start_address);
        fclose(f);
        return -1;
    }

    // Copy bytes into memory
    size_t read = fread(&mem->data[start_address], 1, (size_t)size, f);
    fclose(f);

    if ((long)read != size) {
        fprintf(stderr, "Loader: partial read (%zu of %ld bytes)\n", read, size);
        return -1;
    }

    // Confirm load — message goes to GUI console and stdout
    char msg[128];
    snprintf(msg, sizeof(msg),
             "Loaded '%s' at 0x%08X (%ld bytes)", filename, start_address, size);
    io_print_string(msg);

    return 0;
}
```

### Error cases handled

| Condition | Error message |
|-----------|--------------|
| `filename` is NULL | `"Loader: null argument"` |
| File not found | `"Loader: cannot open '<file>': No such file or directory"` |
| File is empty | `"Loader: file '<file>' is empty"` |
| Program too large | `"Loader: program too large (N bytes) for address 0x…"` |
| Partial read | `"Loader: partial read (X of Y bytes)"` |

### GUI integration
After a successful load the GUI file-picker dialog calls `loader_load_file()` and the status message appears in the console panel. The memory hex view scrolls to `start_address` to show the first bytes of the program.

### Key files
- [src/loader.h](../src/loader.h)
- [src/loader.c](../src/loader.c)

---

## Step 9: Backend I/O Module

### Goal
Provide a thin I/O layer that decouples the CPU from its output destination — in CLI mode output goes to `stdout`/`stdin`; in GUI mode it is routed to the on-screen console panel and input modal.

### API

```c
// io.h
typedef void (*IOOutputFn)(const char *line);
void     io_set_output_callback(IOOutputFn fn);

void     io_print_int   (uint32_t value);
void     io_print_string(const char *str);
uint32_t io_read_int    (void);
```

### Callback mechanism

```c
// io.c
static IOOutputFn output_callback = NULL;

void io_set_output_callback(IOOutputFn fn) {
    output_callback = fn;
}

static void emit(const char *line) {
    if (output_callback)
        output_callback(line);   // GUI: append to console buffer
    else
        printf("%s\n", line);    // CLI: stdout
}
```

### `io_print_int()`

```c
void io_print_int(uint32_t value) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%u", value);
    emit(buf);
}
```

### `io_print_string()`

```c
void io_print_string(const char *str) {
    if (str) emit(str);
}
```

### `io_read_int()`

```c
uint32_t io_read_int(void) {
    // GUI mode: the GUI sets a flag to open an input modal and blocks
    // until the user submits a value via gui_get_pending_input().
    // CLI mode: plain scanf.
    uint32_t value = 0;
    if (gui_is_active()) {
        value = gui_read_int_modal();   // blocks until user presses Enter in modal
    } else {
        printf("Input: ");
        fflush(stdout);
        scanf("%u", &value);
    }
    return value;
}
```

### GUI console buffer
The GUI maintains a `char console_lines[256][256]` ring buffer. `io_set_output_callback()` is called during GUI init with a function that appends to this buffer; the rendering loop draws the last N lines that fit in the console panel.

### Key files
- [src/io.h](../src/io.h)
- [src/io.c](../src/io.c)

---

## Step 10: Main Entry Point & UI Glue

### Goal
Wire Memory, CPU, Loader, and I/O together in `main.c`; parse command-line flags; initialise the GUI or fall back to CLI; and connect UI button callbacks to backend operations.

### Command-line flags

| Flag | Effect |
|------|--------|
| `--program <file>` | Load the given `.bin` file before running |
| `--gui` | Open the SDL2 window (default: CLI mode) |
| `--headless` | Run without any output (for automated grading) |
| `--debug` | Enable verbose instruction trace to stderr |

### Startup sequence

```c
int main(int argc, char *argv[]) {
    // 1. Parse arguments
    const char *program_file = NULL;
    bool use_gui = false, headless = false, debug = false;
    for (int i = 1; i < argc; i++) {
        if      (!strcmp(argv[i], "--gui"))           use_gui = true;
        else if (!strcmp(argv[i], "--headless"))      headless = true;
        else if (!strcmp(argv[i], "--debug"))         debug = true;
        else if (!strcmp(argv[i], "--program") && i+1 < argc)
            program_file = argv[++i];
    }

    // 2. Initialise subsystems
    Memory mem;
    CPU    cpu;
    memory_init(&mem);
    cpu_init(&cpu, &mem);

    // 3. Route I/O to GUI console (if GUI mode)
    if (use_gui)
        io_set_output_callback(gui_append_console);

    // 4. Load program (if provided)
    if (program_file) {
        if (loader_load_file(&mem, program_file, 0) != 0)
            return 1;
        cpu.running = true;
    }

    // 5. Run
    if (use_gui) {
        gui_run(&cpu);     // GUI event loop drives cpu_step() / cpu_run()
    } else {
        cpu_run(&cpu);     // CLI: run to HALT
        if (!headless) cpu_dump(&cpu);
    }

    return 0;
}
```

### GUI button callbacks

Each toolbar button is wired to a backend operation:

| Button | Action |
|--------|--------|
| **Load** | Open file-picker dialog → `loader_load_file()` → refresh GUI |
| **Run** | Set `cpu.running = true`; GUI loop calls `cpu_step()` each frame |
| **Pause** | Set `gui_paused = true`; GUI loop stops calling `cpu_step()` |
| **Step** | Call `cpu_step()` once regardless of `running` state |
| **Reset** | Call `memory_init()` + `cpu_init()` + clear console buffer |

### GUI state refresh
After every `cpu_step()` call the GUI re-reads `cpu.reg[]`, `cpu.pc`, `cpu.sp`, and `cpu.flags` to update the register sidebar. The memory hex view redraws the region around the current PC.

### Key files
- [src/main.c](../src/main.c)
- [src/gui.c](../src/gui.c) — `gui_run()`, button handlers, console buffer
