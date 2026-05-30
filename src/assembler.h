#ifndef ASSEMBLER_H
#define ASSEMBLER_H

// Assemble a .asm text file into a .bin binary for the VM.
// Returns 0 on success, non-zero if any errors were found.
int assemble(const char *src_path, const char *dst_path);

#endif
