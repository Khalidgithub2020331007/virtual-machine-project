// Helper: generates a test binary program for the VM
// Program: R0 = 10, R1 = 20, R0 = R0 + R1, PRINT R0, HALT
#include <stdio.h>
#include <stdint.h>

int main() {
    FILE *f = fopen("test.bin", "wb");

    // MOV R0, 10
    fputc(0x01, f); fputc(0x00, f);
    fputc(10, f); fputc(0, f); fputc(0, f); fputc(0, f);

    // MOV R1, 20
    fputc(0x01, f); fputc(0x01, f);
    fputc(20, f); fputc(0, f); fputc(0, f); fputc(0, f);

    // ADD R0, R1
    fputc(0x10, f); fputc(0x00, f); fputc(0x01, f);

    // PRINT R0
    fputc(0x80, f); fputc(0x00, f);

    // HALT
    fputc(0xFF, f);

    fclose(f);
    printf("test.bin created.\n");
    return 0;
}
