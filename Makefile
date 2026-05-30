CC     = gcc
CFLAGS = -Wall -Wextra -g -std=c11
SRC    = src/main.c src/cpu.c src/memory.c src/instructions.c src/loader.c src/io.c
GUI_SRC = src/gui.c
ASM_SRC = src/assembler.c src/asm_main.c
TARGET     = vm
GUI_TARGET = vm_gui
ASM_TARGET = asm

SDL_CFLAGS ?= $(shell pkg-config --cflags sdl2 SDL2_ttf 2>/dev/null)
SDL_LIBS   ?= $(shell pkg-config --libs   sdl2 SDL2_ttf 2>/dev/null)

all: $(TARGET) $(ASM_TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

$(GUI_TARGET): $(SRC) $(GUI_SRC)
	$(CC) $(CFLAGS) -DBUILD_GUI $(SDL_CFLAGS) -o $(GUI_TARGET) $(SRC) $(GUI_SRC) $(SDL_LIBS)

$(ASM_TARGET): $(ASM_SRC)
	$(CC) $(CFLAGS) -o $(ASM_TARGET) $(ASM_SRC)

# Assemble all .asm source files to .bin
assemble: $(ASM_TARGET)
	./$(ASM_TARGET) programs/test.asm
	./$(ASM_TARGET) programs/countdown.asm
	./$(ASM_TARGET) programs/factorial.asm
	./$(ASM_TARGET) programs/fibonacci.asm
	./$(ASM_TARGET) programs/input_test.asm

# Build the program generator and produce all .bin files
programs/make_programs: programs/make_programs.c
	$(CC) $(CFLAGS) -o programs/make_programs programs/make_programs.c

programs: programs/make_programs
	programs/make_programs

# Run all programs through the CLI emulator and print results
test: $(TARGET) programs
	@echo ""
	@echo "=== countdown (expect 10..0) ==="
	./$(TARGET) programs/countdown.bin
	@echo ""
	@echo "=== factorial (expect 120) ==="
	./$(TARGET) programs/factorial.bin
	@echo ""
	@echo "=== fibonacci (expect 0 1 1 2 3 5 8 13 21 34) ==="
	./$(TARGET) programs/fibonacci.bin
	@echo ""
	@echo "=== basic add (expect 30) ==="
	./$(TARGET) programs/test.bin

clean:
	rm -f $(TARGET) $(GUI_TARGET) $(ASM_TARGET) src/*.o
	rm -f programs/make_programs programs/make_test
	rm -f programs/countdown.bin programs/factorial.bin \
	      programs/fibonacci.bin programs/input_test.bin programs/test.bin

.PHONY: all programs assemble test clean

