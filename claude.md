# CLAUDE

Project requirements have been moved to `requiremts.md`.

## Summary
This repository contains a student project to build a GUI-first virtual machine (VM) in C/C++ for a university assignment. The requirements and scope are in `requiremts.md`.

## System (development) Environment
- CPU: Intel i5
- RAM: 8 GB
- Storage: 512 GB SSD
- Platform: Linux

## Project Goals (GUI-first)
- Build an emulator backend (CPU, memory, loader) and a GUI frontend that visualizes state and provides interactive controls.
- Provide CLI fallback mode for automated tests and grading.
- Deliver a sample program and documentation including screenshots of the UI.

## Development Rules (must follow)
1. Do not break the existing repository structure.
2. Avoid introducing new bugs or regressions.
3. After any change, verify the codebase still builds and existing behavior is preserved.

## Use of LLM / AI tools
- You are allowed to use LLMs or AI tools to help write code, generate tests, or produce documentation during development.
- Any substantive LLM-produced material must be documented in the final report and in `LLM_USAGE.md` (model/service used, prompts, and verification steps). Do not include API keys in the repo.

## Where to find requirements
See `requiremts.md` for the full functional and non-functional requirements, deliverables, and architecture notes (now GUI-first).

## Next steps
- Update `PLAN.md` for GUI-first implementation tasks.
- Add SDL2 build notes to the `Makefile` or README and implement an initial GUI skeleton.
