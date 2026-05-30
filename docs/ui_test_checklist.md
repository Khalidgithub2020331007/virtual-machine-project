# GUI Manual Test Checklist

Build and launch before running these checks:

```bash
make vm_gui
./vm_gui --gui --program programs/countdown.bin
```

---

## 1. Startup

- [ ] Window opens (1100 × 720, resizable), title "VM Emulator"
- [ ] Toolbar shows five buttons: **LOAD · RUN · STEP · PAUSE · RESET**
- [ ] Status badge (top-left) shows **STOPPED**
- [ ] Sidebar registers R0–R7 all `0x00000000`, PC = `0x00000000`, SP = `0x000FFFF0`
- [ ] Sidebar flags ZERO / NEG / OVERFLOW all `0`
- [ ] Console shows: `VM ready.  Click LOAD to open a program, then RUN.`

---

## 2. LOAD

Steps: click **LOAD**, then type the path in the terminal prompt.

- [ ] Console shows: `[LOAD] Type program path in terminal:`
- [ ] After entering `programs/countdown.bin`: console shows `[LOAD] OK: programs/countdown.bin`
- [ ] PROGRAM section in sidebar shows the filename
- [ ] Memory hex view shows program bytes starting at address `000000`

Error case — type a nonexistent path:
- [ ] Console shows: `[LOAD] Failed — check the file path.`

---

## 3. RUN (F5 or RUN button)

Load `programs/countdown.bin`, then click **RUN**.

- [ ] Status badge turns green and reads **RUNNING**
- [ ] Console shows: `[RUN] Execution started.`
- [ ] Console appends numbers `10 9 8 7 6 5 4 3 2 1 0` as the program runs
- [ ] Console shows: `[VM] HALT — execution finished.`
- [ ] Status badge returns to **STOPPED**; sidebar CPU field shows `Halted`

---

## 4. STEP (F10 or STEP button)

Click **RESET**, then click **STEP** repeatedly.

- [ ] First step: console shows `[STEP] PC=0x00000006` (after the first MOV)
- [ ] Each step advances PC and updates registers in the sidebar in real time
- [ ] Yellow highlight in the memory hex view tracks the current PC byte
- [ ] After all instructions: console shows `[VM] HALT.`
- [ ] Subsequent STEP clicks: `[STEP] CPU halted. Press RESET to restart.`

Shortcut: clicking **STEP** while running pauses and prints `[STEP] Paused for stepping.`

---

## 5. PAUSE / RESUME (F6 or PAUSE button)

Load and click **RUN** on `programs/fibonacci.bin`.

- [ ] Click **PAUSE**: status badge turns orange **PAUSED**, console shows `[PAUSE] Execution paused.`
- [ ] Registers freeze; memory view stops scrolling
- [ ] Click **PAUSE** again (or F6): status returns to green **RUNNING**, console shows `[RUN] Resumed.`

---

## 6. RESET

While any program is loaded or partially run, click **RESET**.

- [ ] Console shows: `[RESET] VM reset to initial state.`
- [ ] All registers return to `0`, PC = `0x00000000`, SP = `0x000FFFF0`
- [ ] Status badge shows **STOPPED**; CPU field shows `Active` (ready to run again)
- [ ] Memory scroll resets to row 0

---

## 7. Memory Hex View

- [ ] Move the mouse over the hex view; scroll wheel scrolls row by row
- [ ] Row counter at the bottom updates: `Row N / 65536`
- [ ] Scrolling away from PC: yellow PC highlight moves off-screen; highlight reappears when scrolling back
- [ ] Mouse wheel over the sidebar or console does **not** scroll the memory view

---

## 8. Console Output Per Program

Run each binary with **LOAD → RUN** and verify console output.

| Program | Expected console output |
|---|---|
| `test.bin` | `30` |
| `countdown.bin` | `10  9  8  7  6  5  4  3  2  1  0` |
| `factorial.bin` | `120` |
| `fibonacci.bin` | `0  1  1  2  3  5  8  13  21  34` |

---

## 9. Input Modal (input_test.bin)

Load and run `programs/input_test.bin`.

- [ ] Console shows: `[INPUT] Enter integer value in terminal:`
- [ ] Terminal shows: `Input: ` — type `7` and press Enter
- [ ] Console shows: `[INPUT] >> 7`
- [ ] Console shows: `14` (= 7 × 2)
- [ ] Console shows: `[VM] HALT — execution finished.`

---

## 10. Keyboard Shortcuts

| Key | Expected action |
|---|---|
| **F5** | Same as clicking RUN |
| **F6** | Same as clicking PAUSE (toggles pause/resume) |
| **F10** | Same as clicking STEP |
| **Esc** | Window closes, process exits cleanly |
