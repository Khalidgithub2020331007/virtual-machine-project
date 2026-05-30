# VM Project — Build & Run

Quick notes to build and run the emulator in CLI and GUI modes.

Prerequisites (Linux)
- C compiler (gcc)
- `make`
- SDL2 development libraries for GUI mode: install `libsdl2-dev` (Debian/Ubuntu) or `SDL2-devel` (Fedora).

Install SDL2 (Debian/Ubuntu):
```bash
sudo apt update
sudo apt install build-essential pkg-config libsdl2-dev
```

Build CLI emulator:
```bash
make
./vm --program programs/<yourprog>.bin
```

Build GUI emulator (requires SDL2):
```bash
make vm_gui
./vm_gui --gui --program programs/<yourprog>.bin
```

Notes
- If `pkg-config` is not available or SDL2 is in a custom location, set `SDL_CFLAGS` and `SDL_LIBS` environment variables before building, e.g.: `make vm_gui SDL_CFLAGS="-I/path/to/include" SDL_LIBS="-L/path/to/lib -lSDL2"`.
- See `requiremts.md` and `PLAN.md` for project scope and development plan.
