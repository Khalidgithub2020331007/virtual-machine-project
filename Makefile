CC     = gcc
CFLAGS = -Wall -Wextra -g -std=c11
SRC    = src/main.c src/cpu.c src/memory.c src/instructions.c src/loader.c src/io.c
GUI_SRC = src/gui.c
TARGET     = vm
GUI_TARGET = vm_gui

SDL_CFLAGS ?= $(shell pkg-config --cflags sdl2 SDL2_ttf 2>/dev/null)
SDL_LIBS   ?= $(shell pkg-config --libs   sdl2 SDL2_ttf 2>/dev/null)

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

$(GUI_TARGET): $(SRC) $(GUI_SRC)
	$(CC) $(CFLAGS) -DBUILD_GUI $(SDL_CFLAGS) -o $(GUI_TARGET) $(SRC) $(GUI_SRC) $(SDL_LIBS)

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
	rm -f $(TARGET) $(GUI_TARGET) src/*.o
	rm -f programs/make_programs programs/make_test
	rm -f programs/countdown.bin programs/factorial.bin \
	      programs/fibonacci.bin programs/input_test.bin

.PHONY: all programs test clean
