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

clean:
	rm -f $(TARGET) $(GUI_TARGET) src/*.o

.PHONY: all clean
