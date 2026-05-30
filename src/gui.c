// GUI implementation — SDL2 + SDL2_ttf
// Layout: toolbar (top) | sidebar registers/flags (left) | memory hex view (main) | console (bottom)
#include "gui.h"
#include "cpu.h"
#include "memory.h"
#include "loader.h"
#include "io.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// ── Layout constants ──────────────────────────────────────────────────────────
#define WIN_W       1100
#define WIN_H        720
#define TOOLBAR_H     44
#define SIDEBAR_W    264
#define CONSOLE_H    180

// ── Console ring buffer ───────────────────────────────────────────────────────
#define CON_CAP  200
#define CON_COLS 128
static char con_buf[CON_CAP][CON_COLS];
static int  con_head  = 0;
static int  con_count = 0;

// ── Module state ──────────────────────────────────────────────────────────────
static SDL_Window   *g_win     = NULL;
static SDL_Renderer *g_ren     = NULL;
static TTF_Font     *g_font    = NULL;   // 13 px
static TTF_Font     *g_fsm     = NULL;   // 12 px
static bool          g_running = false;
static CPU          *g_cpu     = NULL;
static Memory       *g_mem     = NULL;

typedef enum { VM_STOPPED, VM_RUNNING, VM_PAUSED } VMState;
static VMState  g_vmstate   = VM_STOPPED;
static char     g_loaded[256] = "";
static uint32_t g_memscroll  = 0;

// ── Breakpoints ───────────────────────────────────────────────────────────────
#define MAX_BP 16
static uint32_t g_bp[MAX_BP];
static int      g_bp_count = 0;

// ── Buttons ───────────────────────────────────────────────────────────────────
typedef struct { SDL_Rect r; const char *label; } Btn;
#define NBTN 5
static Btn g_btn[NBTN];

// ── Forward declarations ──────────────────────────────────────────────────────
static void     render(void);
static void     on_event(const SDL_Event *e);
static void     draw_toolbar(void);
static void     draw_sidebar(void);
static void     draw_memview(void);
static void     draw_console(void);
static void     txt(TTF_Font *f, const char *s, int x, int y, SDL_Color c);
static void     fill(SDL_Rect r, SDL_Color c);
static void     bord(SDL_Rect r, SDL_Color c);
static void     act_load(void);
static void     act_run(void);
static void     act_step(void);
static void     act_pause(void);
static void     act_reset(void);
static void     act_toggle_breakpoint(void);
static TTF_Font *open_font(int sz);

// ── Font loader — tries common monospace paths ────────────────────────────────
static TTF_Font *open_font(int sz) {
    static const char *paths[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
        "/usr/share/fonts/truetype/freefont/FreeMono.ttf",
        "/usr/share/fonts/truetype/ubuntu/UbuntuMono-R.ttf",
        "/usr/share/fonts/truetype/hack/Hack-Regular.ttf",
        "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
        NULL
    };
    for (int i = 0; paths[i]; i++) {
        TTF_Font *f = TTF_OpenFont(paths[i], sz);
        if (f) return f;
    }
    return NULL;
}

// ── Init / shutdown ───────────────────────────────────────────────────────────

bool gui_init(CPU *cpu, Memory *mem) {
    g_cpu = cpu;
    g_mem = mem;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return false;
    }
    if (TTF_Init() != 0) {
        fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());
        SDL_Quit();
        return false;
    }

    g_font = open_font(13);
    g_fsm  = open_font(12);
    if (!g_font && !g_fsm)
        fprintf(stderr,
            "Warning: no monospace font found. "
            "Install fonts-dejavu-core for text rendering.\n");

    g_win = SDL_CreateWindow(
        "VM Emulator",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIN_W, WIN_H,
        SDL_WINDOW_RESIZABLE
    );
    if (!g_win) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        TTF_Quit(); SDL_Quit();
        return false;
    }

    g_ren = SDL_CreateRenderer(g_win, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!g_ren) {
        SDL_DestroyWindow(g_win);
        fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
        TTF_Quit(); SDL_Quit();
        return false;
    }

    // Build toolbar button strip (starts after sidebar)
    static const char *labels[NBTN] = {"LOAD","RUN","STEP","PAUSE","RESET"};
    int bx = SIDEBAR_W + 14;
    for (int i = 0; i < NBTN; i++)
        g_btn[i] = (Btn){ {bx + i * 100, 6, 88, 32}, labels[i] };

    g_running = true;
    gui_console_append("VM ready.  Click LOAD to open a program, then RUN.");
    return true;
}

void gui_shutdown(void) {
    if (g_font) TTF_CloseFont(g_font);
    if (g_fsm)  TTF_CloseFont(g_fsm);
    if (g_ren)  SDL_DestroyRenderer(g_ren);
    if (g_win)  SDL_DestroyWindow(g_win);
    TTF_Quit();
    SDL_Quit();
}

// ── Public API ────────────────────────────────────────────────────────────────

void gui_console_append(const char *line) {
    strncpy(con_buf[con_head], line, CON_COLS - 1);
    con_buf[con_head][CON_COLS - 1] = '\0';
    con_head = (con_head + 1) % CON_CAP;
    if (con_count < CON_CAP) con_count++;
    printf("%s\n", line);
}

uint32_t gui_request_input(void) {
    gui_console_append("[INPUT] Enter integer value in terminal:");
    printf("Input: ");
    fflush(stdout);
    uint32_t v = 0;
    scanf("%u", &v);
    char buf[40];
    snprintf(buf, sizeof(buf), "[INPUT] >> %u", v);
    gui_console_append(buf);
    return v;
}

// ── Main loop ─────────────────────────────────────────────────────────────────

void gui_run(void) {
    SDL_Event e;
    while (g_running) {
        while (SDL_PollEvent(&e))
            on_event(&e);

        // Step up to 500 instructions per frame (~30k/s at 60fps)
        if (g_vmstate == VM_RUNNING && g_cpu && g_cpu->running) {
            bool bp_hit = false;
            for (int i = 0; i < 500 && g_cpu->running && !bp_hit; i++) {
                cpu_step(g_cpu);
                for (int b = 0; b < g_bp_count; b++) {
                    if (g_cpu->pc == g_bp[b]) {
                        char msg[48];
                        snprintf(msg, sizeof(msg), "[BRKPT] Hit at 0x%08X", g_cpu->pc);
                        gui_console_append(msg);
                        g_vmstate = VM_PAUSED;
                        bp_hit = true;
                        break;
                    }
                }
            }
            if (!g_cpu->running && g_vmstate == VM_RUNNING) {
                g_vmstate = VM_STOPPED;
                gui_console_append("[VM] HALT — execution finished.");
            }
        }

        render();
    }
}

// ── Draw helpers ──────────────────────────────────────────────────────────────

static void fill(SDL_Rect r, SDL_Color c) {
    SDL_SetRenderDrawColor(g_ren, c.r, c.g, c.b, c.a);
    SDL_RenderFillRect(g_ren, &r);
}

static void bord(SDL_Rect r, SDL_Color c) {
    SDL_SetRenderDrawColor(g_ren, c.r, c.g, c.b, c.a);
    SDL_RenderDrawRect(g_ren, &r);
}

static void txt(TTF_Font *f, const char *s, int x, int y, SDL_Color c) {
    if (!f || !s || !s[0]) return;
    SDL_Surface *sur = TTF_RenderText_Blended(f, s, c);
    if (!sur) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(g_ren, sur);
    if (tex) {
        SDL_Rect dst = {x, y, sur->w, sur->h};
        SDL_RenderCopy(g_ren, tex, NULL, &dst);
        SDL_DestroyTexture(tex);
    }
    SDL_FreeSurface(sur);
}

// ── Render ────────────────────────────────────────────────────────────────────

static void render(void) {
    SDL_SetRenderDrawColor(g_ren, 24, 24, 32, 255);
    SDL_RenderClear(g_ren);
    draw_toolbar();
    draw_sidebar();
    draw_memview();
    draw_console();
    SDL_RenderPresent(g_ren);
}

// ── Toolbar ───────────────────────────────────────────────────────────────────

static void draw_toolbar(void) {
    fill((SDL_Rect){0, 0, WIN_W, TOOLBAR_H}, (SDL_Color){34, 34, 46, 255});

    TTF_Font *f = g_fsm ? g_fsm : g_font;

    // App title
    txt(f, "VM Emulator", 10, 13, (SDL_Color){90, 150, 255, 255});

    // VM state badge
    const char *s  = "STOPPED";
    SDL_Color   sc = {130, 130, 150, 255};
    if (g_vmstate == VM_RUNNING) { s = "RUNNING"; sc = (SDL_Color){70, 210, 90,  255}; }
    if (g_vmstate == VM_PAUSED)  { s = "PAUSED";  sc = (SDL_Color){255,175, 50,  255}; }
    txt(f, s, SIDEBAR_W - 90, 15, sc);

    // Buttons
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    for (int i = 0; i < NBTN; i++) {
        bool hov = SDL_PointInRect(&(SDL_Point){mx, my}, &g_btn[i].r);
        SDL_Color bg = hov ? (SDL_Color){60, 60, 80, 255} : (SDL_Color){44, 44, 58, 255};
        if (i == 1 && g_vmstate == VM_RUNNING)
            bg = (SDL_Color){30, 85, 160, 255};
        fill(g_btn[i].r, bg);
        bord(g_btn[i].r, hov ? (SDL_Color){90,140,255,255} : (SDL_Color){60,60,80,255});
        if (f) {
            int tw = 0, th = 0;
            TTF_SizeText(f, g_btn[i].label, &tw, &th);
            txt(f, g_btn[i].label,
                g_btn[i].r.x + (g_btn[i].r.w - tw) / 2,
                g_btn[i].r.y + (g_btn[i].r.h - th) / 2,
                (SDL_Color){215, 215, 228, 255});
        }
    }

    // Keyboard hints on the right
    txt(f, "F5 Run  F10 Step  F6 Pause  Esc Quit",
        SIDEBAR_W + NBTN * 100 + 24, 15,
        (SDL_Color){65, 65, 85, 255});

    // Bottom border
    SDL_SetRenderDrawColor(g_ren, 50, 50, 68, 255);
    SDL_RenderDrawLine(g_ren, 0, TOOLBAR_H - 1, WIN_W, TOOLBAR_H - 1);
}

// ── Sidebar — registers, flags, status ───────────────────────────────────────

static void draw_sidebar(void) {
    fill((SDL_Rect){0, TOOLBAR_H, SIDEBAR_W, WIN_H - TOOLBAR_H},
         (SDL_Color){30, 30, 40, 255});
    SDL_SetRenderDrawColor(g_ren, 50, 50, 68, 255);
    SDL_RenderDrawLine(g_ren, SIDEBAR_W - 1, TOOLBAR_H, SIDEBAR_W - 1, WIN_H);

    TTF_Font *f    = g_fsm ? g_fsm : g_font;
    SDL_Color head = {90, 150, 255, 255};
    SDL_Color lbl  = {110,110,135, 255};
    SDL_Color val  = {210,210,222, 255};
    SDL_Color pcc  = {110,200,255, 255};
    SDL_Color fon  = {70, 210, 90,  255};
    SDL_Color foff = {80,  80,100,  255};

    int x = 10, y = TOOLBAR_H + 10;

    // ── Registers ──────────────────────────────────────────────────────────
    txt(f, "REGISTERS", x, y, head); y += 22;
    if (g_cpu) {
        for (int i = 0; i < NUM_REGISTERS; i++) {
            char lb[4], vs[30];
            snprintf(lb, sizeof(lb), "R%d", i);
            snprintf(vs, sizeof(vs), "0x%08X  (%u)", g_cpu->reg[i], g_cpu->reg[i]);
            txt(f, lb, x,      y, lbl);
            txt(f, vs, x + 28, y, val);
            y += 17;
        }
        y += 4;
        char pc[12], sp[12];
        snprintf(pc, sizeof(pc), "0x%08X", g_cpu->pc);
        snprintf(sp, sizeof(sp), "0x%08X", g_cpu->sp);
        txt(f, "PC", x, y, lbl); txt(f, pc, x + 28, y, pcc); y += 17;
        txt(f, "SP", x, y, lbl); txt(f, sp, x + 28, y, val); y += 17;
    } else {
        txt(f, "(not initialized)", x, y, lbl); y += 17;
    }

    y += 12;

    // ── Flags ──────────────────────────────────────────────────────────────
    txt(f, "FLAGS", x, y, head); y += 22;
    if (g_cpu) {
        // Pill background for each flag
        struct { const char *name; bool on; int xoff; } flags[] = {
            {"ZERO",     g_cpu->flags.zero,     0},
            {"NEG",      g_cpu->flags.negative, 0},
            {"OVERFLOW", g_cpu->flags.overflow, 0},
        };
        for (int i = 0; i < 3; i++) {
            SDL_Color fc = flags[i].on ? fon : foff;
            char line[20];
            snprintf(line, sizeof(line), "%-9s  %c", flags[i].name, flags[i].on ? '1' : '0');
            txt(f, line, x, y, fc);
            y += 17;
        }
    }

    y += 12;

    // ── VM status ──────────────────────────────────────────────────────────
    txt(f, "STATUS", x, y, head); y += 22;
    if (g_cpu) {
        const char *ss = g_vmstate == VM_RUNNING ? "Running" :
                         g_vmstate == VM_PAUSED  ? "Paused"  : "Stopped";
        txt(f, "State:", x, y, lbl); txt(f, ss, x + 52, y, val); y += 17;
        txt(f, "CPU:",   x, y, lbl);
        txt(f, g_cpu->running ? "Active" : "Halted",
            x + 40, y,
            g_cpu->running ? fon : (SDL_Color){200, 70, 70, 255});
        y += 17;
    }

    y += 10;

    // ── Loaded file ────────────────────────────────────────────────────────
    txt(f, "PROGRAM", x, y, head); y += 22;
    if (g_loaded[0]) {
        const char *fname = strrchr(g_loaded, '/');
        fname = fname ? fname + 1 : g_loaded;
        txt(f, fname, x, y, (SDL_Color){145, 210, 120, 255});
    } else {
        txt(f, "(none loaded)", x, y, lbl);
    }

    // ── Disassembly trace ──────────────────────────────────────────────────────
    y += 8;
    txt(f, "DISASSEMBLY", x, y, head); y += 20;
    if (g_cpu && g_mem) {
        uint32_t a = g_cpu->pc;
        int max_lines = (WIN_H - 82 - y) / 15;
        if (max_lines > 9) max_lines = 9;
        for (int i = 0; i < max_lines && a < MEMORY_SIZE; i++) {
            bool is_bp = false;
            for (int b = 0; b < g_bp_count; b++)
                if (g_bp[b] == a) { is_bp = true; break; }

            if (i == 0) {
                SDL_Rect hl = {x-2, y-1, SIDEBAR_W-12, 14};
                fill(hl, (SDL_Color){35,55,85,200});
            }

            char dis[48], line[60];
            int  n = cpu_disasm(g_mem, a, dis, sizeof(dis));
            snprintf(line, sizeof(line), "%c%06X %s",
                     is_bp ? '*' : ' ', a, dis);

            SDL_Color lc = (i == 0)  ? pcc :
                           is_bp     ? (SDL_Color){255,80,80,255} : val;
            txt(f, line, x, y, lc);
            y += 15;
            a += (n > 0) ? (uint32_t)n : 1;
        }
    } else {
        txt(f, "(no program)", x, y, lbl);
    }

    // ── Keyboard hints at bottom ───────────────────────────────────────────
    int hy = WIN_H - 82;
    SDL_Color hint = {60, 60, 80, 255};
    txt(f, "F5   Run",          x, hy,      hint);
    txt(f, "F10  Step",         x, hy + 14, hint);
    txt(f, "F6   Pause/Resume", x, hy + 28, hint);
    txt(f, "F9   Toggle BRKPT", x, hy + 42, hint);
    txt(f, "Esc  Quit",         x, hy + 56, hint);
}

// ── Memory hex view ───────────────────────────────────────────────────────────

static void draw_memview(void) {
    int x0 = SIDEBAR_W + 1;
    int y0 = TOOLBAR_H;
    int w  = WIN_W - SIDEBAR_W;
    int h  = WIN_H - TOOLBAR_H - CONSOLE_H;

    fill((SDL_Rect){x0, y0, w, h}, (SDL_Color){24, 24, 32, 255});
    SDL_SetRenderDrawColor(g_ren, 48, 48, 64, 255);
    SDL_RenderDrawLine(g_ren, x0, y0 + h, WIN_W, y0 + h);

    TTF_Font *f    = g_fsm ? g_fsm : g_font;
    SDL_Color head = {90, 150, 255, 255};
    SDL_Color adrc = {95, 145, 190, 255};
    SDL_Color bytc = {190, 190, 205, 255};
    SDL_Color zerc = {58,  58,  75,  255};
    SDL_Color ascc = {110, 185, 110, 255};
    SDL_Color dimc = {62,  62,  80,  255};
    SDL_Color pchc = {255, 215, 60,  255};

    int tx = x0 + 10, ty = y0 + 8;

    txt(f, "MEMORY HEX VIEW", tx, ty, head); ty += 20;

    txt(f,
        "Address     00 01 02 03 04 05 06 07   08 09 0A 0B 0C 0D 0E 0F   ASCII",
        tx, ty, (SDL_Color){62, 105, 145, 255});
    ty += 14;
    SDL_SetRenderDrawColor(g_ren, 44, 44, 60, 255);
    SDL_RenderDrawLine(g_ren, tx, ty, tx + 650, ty);
    ty += 5;

    if (!g_mem) {
        txt(f, "(memory not attached)", tx, ty, dimc);
        return;
    }

    int lh   = 15;
    int rows = (h - (ty - y0) - 22) / lh;

    for (int row = 0; row < rows; row++) {
        uint32_t addr = (g_memscroll + (uint32_t)row) * 16;
        if (addr >= MEMORY_SIZE) break;

        bool pc_row = g_cpu && g_cpu->pc >= addr && g_cpu->pc < addr + 16;
        if (pc_row) {
            SDL_Rect hl = {tx - 4, ty - 1, w - 18, lh};
            fill(hl, (SDL_Color){35, 55, 85, 200});
        }

        // Address column
        char tmp[12];
        snprintf(tmp, sizeof(tmp), "%06X", addr);
        txt(f, tmp, tx, ty, pc_row ? pchc : adrc);

        // Hex bytes (two groups of 8, gap in the middle)
        int bx = tx + 76;
        for (int i = 0; i < 16; i++) {
            if (i == 8) bx += 8;
            uint8_t  b = memory_read_byte(g_mem, addr + i);
            SDL_Color bc = (b == 0) ? zerc : bytc;
            if (g_cpu && addr + (uint32_t)i == g_cpu->pc) bc = pchc;
            snprintf(tmp, sizeof(tmp), "%02X", b);
            txt(f, tmp, bx, ty, bc);
            bx += 22;
        }

        // ASCII column
        int ax = tx + 76 + 16 * 22 + 16;
        for (int i = 0; i < 16; i++) {
            uint8_t b = memory_read_byte(g_mem, addr + i);
            char ch[2] = {(b >= 32 && b < 127) ? (char)b : '.', '\0'};
            txt(f, ch, ax + i * 9, ty, ascc);
        }

        ty += lh;
    }

    // Scroll hint
    char sh[72];
    snprintf(sh, sizeof(sh), "Row %u / %u   (mouse wheel to scroll — yellow = PC)",
             g_memscroll, MEMORY_SIZE / 16);
    txt(f, sh, tx, y0 + h - 16, dimc);
}

// ── Console output panel ──────────────────────────────────────────────────────

static void draw_console(void) {
    int x0 = SIDEBAR_W + 1;
    int y0 = WIN_H - CONSOLE_H;
    int w  = WIN_W - SIDEBAR_W;

    fill((SDL_Rect){x0, y0, w, CONSOLE_H}, (SDL_Color){20, 20, 28, 255});
    SDL_SetRenderDrawColor(g_ren, 48, 48, 64, 255);
    SDL_RenderDrawLine(g_ren, x0, y0, WIN_W, y0);

    TTF_Font *f = g_fsm ? g_fsm : g_font;
    int tx = x0 + 10, ty = y0 + 6;

    txt(f, "CONSOLE OUTPUT", tx, ty, (SDL_Color){90, 150, 255, 255}); ty += 18;
    SDL_SetRenderDrawColor(g_ren, 38, 38, 52, 255);
    SDL_RenderDrawLine(g_ren, tx, ty, WIN_W - 10, ty); ty += 4;

    int lh  = 15;
    int vis = (CONSOLE_H - (ty - y0) - 6) / lh;
    if (vis > con_count) vis = con_count;

    SDL_Color tc = {155, 210, 155, 255};
    for (int i = 0; i < vis; i++) {
        int idx = (con_head - vis + i + CON_CAP) % CON_CAP;
        txt(f, con_buf[idx], tx, ty + i * lh, tc);
    }
}

// ── Event handling ────────────────────────────────────────────────────────────

static void on_event(const SDL_Event *e) {
    switch (e->type) {
        case SDL_QUIT:
            g_running = false;
            break;

        case SDL_KEYDOWN:
            switch (e->key.keysym.sym) {
                case SDLK_ESCAPE: g_running = false;           break;
                case SDLK_F5:     act_run();                   break;
                case SDLK_F10:    act_step();                  break;
                case SDLK_F6:     act_pause();                 break;
                case SDLK_F9:     act_toggle_breakpoint();     break;
            }
            break;

        case SDL_MOUSEBUTTONDOWN:
            if (e->button.button == SDL_BUTTON_LEFT) {
                SDL_Point pt = {e->button.x, e->button.y};
                if (SDL_PointInRect(&pt, &g_btn[0].r)) act_load();
                if (SDL_PointInRect(&pt, &g_btn[1].r)) act_run();
                if (SDL_PointInRect(&pt, &g_btn[2].r)) act_step();
                if (SDL_PointInRect(&pt, &g_btn[3].r)) act_pause();
                if (SDL_PointInRect(&pt, &g_btn[4].r)) act_reset();
            }
            break;

        case SDL_MOUSEWHEEL: {
            // Only scroll when cursor is over the memory view panel
            int mx, my;
            SDL_GetMouseState(&mx, &my);
            int mv_h = WIN_H - TOOLBAR_H - CONSOLE_H;
            if (mx > SIDEBAR_W && my > TOOLBAR_H && my < TOOLBAR_H + mv_h) {
                if (e->wheel.y < 0 && g_memscroll + 1 < MEMORY_SIZE / 16)
                    g_memscroll++;
                else if (e->wheel.y > 0 && g_memscroll > 0)
                    g_memscroll--;
            }
            break;
        }
    }
}

// ── Button actions ────────────────────────────────────────────────────────────

static void act_load(void) {
    // Terminal prompt (SDL has no built-in file dialog)
    gui_console_append("[LOAD] Type program path in terminal:");
    printf("Program path: ");
    fflush(stdout);
    char path[256];
    if (scanf("%255s", path) != 1) {
        gui_console_append("[LOAD] Cancelled.");
        return;
    }
    if (!g_cpu || !g_mem) return;
    memory_init(g_mem);
    cpu_init(g_cpu, g_mem);
    if (loader_load_file(g_mem, path, 0x0000) == 0) {
        strncpy(g_loaded, path, sizeof(g_loaded) - 1);
        g_vmstate   = VM_STOPPED;
        g_memscroll = 0;
        char msg[280];
        snprintf(msg, sizeof(msg), "[LOAD] OK: %s", path);
        gui_console_append(msg);
    } else {
        gui_console_append("[LOAD] Failed — check the file path.");
    }
}

static void act_run(void) {
    if (!g_cpu || !g_mem) return;
    if (!g_loaded[0]) { gui_console_append("[RUN] No program loaded."); return; }
    if (g_vmstate == VM_RUNNING) return;
    // If already halted, reload and restart
    if (!g_cpu->running) {
        memory_init(g_mem);
        cpu_init(g_cpu, g_mem);
        loader_load_file(g_mem, g_loaded, 0x0000);
    }
    g_vmstate = VM_RUNNING;
    gui_console_append("[RUN] Execution started.");
}

static void act_step(void) {
    if (!g_cpu || !g_mem) return;
    if (!g_loaded[0]) { gui_console_append("[STEP] Load a program first."); return; }
    if (g_vmstate == VM_RUNNING) {
        g_vmstate = VM_PAUSED;
        gui_console_append("[STEP] Paused for stepping.");
        return;
    }
    if (g_cpu->running) {
        char dbuf[64];
        uint32_t old_pc = g_cpu->pc;
        cpu_disasm(g_mem, old_pc, dbuf, sizeof(dbuf));
        cpu_step(g_cpu);
        char msg[96];
        snprintf(msg, sizeof(msg), "[STEP] 0x%08X: %s", old_pc, dbuf);
        gui_console_append(msg);
        if (!g_cpu->running) {
            g_vmstate = VM_STOPPED;
            gui_console_append("[VM] HALT.");
        }
    } else {
        gui_console_append("[STEP] CPU halted. Press RESET to restart.");
    }
}

static void act_pause(void) {
    if (g_vmstate == VM_RUNNING) {
        g_vmstate = VM_PAUSED;
        gui_console_append("[PAUSE] Execution paused.");
    } else if (g_vmstate == VM_PAUSED) {
        g_vmstate = VM_RUNNING;
        gui_console_append("[RUN] Resumed.");
    }
}

static void act_reset(void) {
    if (!g_cpu || !g_mem) return;
    memory_init(g_mem);
    cpu_init(g_cpu, g_mem);
    if (g_loaded[0]) loader_load_file(g_mem, g_loaded, 0x0000);
    g_vmstate   = VM_STOPPED;
    g_memscroll = 0;
    gui_console_append("[RESET] VM reset to initial state.");
}

static void act_toggle_breakpoint(void) {
    if (!g_cpu) return;
    uint32_t addr = g_cpu->pc;
    for (int i = 0; i < g_bp_count; i++) {
        if (g_bp[i] == addr) {
            g_bp[i] = g_bp[--g_bp_count];
            char msg[48];
            snprintf(msg, sizeof(msg), "[BRKPT] Removed at 0x%08X", addr);
            gui_console_append(msg);
            return;
        }
    }
    if (g_bp_count < MAX_BP) {
        g_bp[g_bp_count++] = addr;
        char msg[48];
        snprintf(msg, sizeof(msg), "[BRKPT] Set at 0x%08X", addr);
        gui_console_append(msg);
    } else {
        gui_console_append("[BRKPT] Max breakpoints reached (16).");
    }
}
