# LLM Usage Log

Use this file to record any substantive outputs you obtain from LLMs or AI tools during development. This project permits LLMs as development aids; record-keeping helps with reproducibility and academic integrity.

Template entry:

- Date: YYYY-MM-DD
- Tool / Model: (e.g., ChatGPT, Claude, local LLM)
- Purpose: (code snippet, test generation, documentation, design idea, bug fix)
- Prompt / Query: (brief or redacted — do not include secrets)
- Response summary: (short description of useful output)
- Files changed / created: (list file paths)
- Validation / edits performed: (how you verified or changed the output)

Example:

- Date: 2026-05-30
- Tool / Model: ChatGPT (developer assistant)
- Purpose: generate SDL2 GUI skeleton
- Prompt: "Create a minimal C SDL2 program that opens a window and handles quit"
- Response summary: Generated `src/gui.c` and `src/gui.h` skeleton which was adapted and added to repo.
- Files changed / created: `src/gui.c`, `src/gui.h`, `Makefile`, `README.md`
- Validation / edits performed: Compiled locally; trimmed comments and adjusted include paths.
