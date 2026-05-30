// Generates all test binary programs for the VM.
// Run: ./make_programs  (creates countdown.bin, factorial.bin, fibonacci.bin, input_test.bin)
//
// Instruction encoding (little-endian immediates / addresses):
//   MOV  reg, imm32  : 0x01 reg  <4-byte imm>   (6 bytes)
//   MOVR dst, src    : 0x02 dst src              (3 bytes)
//   ADD  dst, src    : 0x10 dst src              (3 bytes)
//   MUL  dst, src    : 0x12 dst src              (3 bytes)
//   DEC  reg         : 0x15 reg                  (2 bytes)
//   CMP  a, b        : 0x30 a b                  (3 bytes)
//   JMP  addr        : 0x40 <4-byte addr>        (5 bytes)
//   JE   addr        : 0x41 <4-byte addr>        (5 bytes)
//   JNE  addr        : 0x42 <4-byte addr>        (5 bytes)
//   CALL addr        : 0x60 <4-byte addr>        (5 bytes)
//   RET              : 0x61                      (1 byte)
//   PRINT reg        : 0x80 reg                  (2 bytes)
//   READ  reg        : 0x81 reg                  (2 bytes)
//   HALT             : 0xFF                      (1 byte)

#include <stdio.h>
#include <stdint.h>

// Register names
#define R0 0
#define R1 1
#define R2 2
#define R3 3
#define R4 4

static FILE *g_f;

static void eb(uint8_t b) { fputc(b, g_f); }
static void ed(uint32_t v) {
    fputc( v        & 0xFF, g_f);
    fputc((v >>  8) & 0xFF, g_f);
    fputc((v >> 16) & 0xFF, g_f);
    fputc((v >> 24) & 0xFF, g_f);
}

static void MOV  (uint8_t r, uint32_t imm) { eb(0x01); eb(r); ed(imm); }
static void MOVR (uint8_t d, uint8_t s)    { eb(0x02); eb(d); eb(s);   }
static void ADD  (uint8_t d, uint8_t s)    { eb(0x10); eb(d); eb(s);   }
static void MUL  (uint8_t d, uint8_t s)    { eb(0x12); eb(d); eb(s);   }
static void DEC  (uint8_t r)               { eb(0x15); eb(r);           }
static void CMP  (uint8_t a, uint8_t b)    { eb(0x30); eb(a); eb(b);   }
static void JMP  (uint32_t a)              { eb(0x40); ed(a);           }
static void JE   (uint32_t a)              { eb(0x41); ed(a);           }
static void JNE  (uint32_t a)              { eb(0x42); ed(a);           }
static void CALL (uint32_t a)              { eb(0x60); ed(a);           }
static void RET  (void)                    { eb(0x61);                  }
static void PRINT(uint8_t r)               { eb(0x80); eb(r);           }
static void READ (uint8_t r)               { eb(0x81); eb(r);           }
static void HALT (void)                    { eb(0xFF);                  }

// ─────────────────────────────────────────────────────────────────────────────
// countdown.bin  —  prints 10, 9, 8, … 1, 0  then halts
//
// Assembly:
//   0x00  MOV  R0, 10      ; counter = 10
//   0x06  PRINT R0         ; << loop: print counter
//   0x08  MOV  R1, 0       ; R1 = 0  (zero reference)
//   0x0E  CMP  R0, R1      ; set ZERO flag if R0 == 0
//   0x11  JE   0x1D        ; if zero → done
//   0x16  DEC  R0          ; counter--
//   0x18  JMP  0x06        ; loop
//   0x1D  HALT             ; done
// ─────────────────────────────────────────────────────────────────────────────
static void make_countdown(void) {
    g_f = fopen("programs/countdown.bin", "wb");
    MOV  (R0, 10);      // 0x00  (6)
    PRINT(R0);          // 0x06  (2) << loop
    MOV  (R1, 0);       // 0x08  (6)
    CMP  (R0, R1);      // 0x0E  (3)
    JE   (0x1D);        // 0x11  (5)
    DEC  (R0);          // 0x16  (2)
    JMP  (0x06);        // 0x18  (5)
    HALT ();            // 0x1D  (1)  << done
    fclose(g_f);
    printf("  countdown.bin   30 bytes  prints 10 down to 0\n");
}

// ─────────────────────────────────────────────────────────────────────────────
// factorial.bin  —  computes 5! = 120 and prints the result
//
// Demonstrates CALL / RET.  Main calls factorial(R0=5); result in R1.
//
// Assembly:
//   ── main ──────────────────────────────────────────────────────
//   0x00  MOV  R0, 5        ; argument n = 5
//   0x06  CALL 0x0E         ; factorial(R0) → result in R1
//   0x0B  PRINT R1          ; print 120
//   0x0D  HALT
//   ── factorial(R0) ─────────────────────────────────────────────
//   0x0E  MOV  R1, 1        ; result = 1
//   0x14  MUL  R1, R0       ; << loop: result *= n
//   0x17  DEC  R0           ; n--
//   0x19  MOV  R2, 0        ; zero reference
//   0x1F  CMP  R0, R2       ; check n == 0
//   0x22  JNE  0x14         ; if not zero, loop
//   0x27  RET               ; return (result in R1)
// ─────────────────────────────────────────────────────────────────────────────
static void make_factorial(void) {
    g_f = fopen("programs/factorial.bin", "wb");
    // main
    MOV  (R0, 5);       // 0x00  (6)
    CALL (0x0E);        // 0x06  (5)
    PRINT(R1);          // 0x0B  (2)
    HALT ();            // 0x0D  (1)
    // factorial function
    MOV  (R1, 1);       // 0x0E  (6)
    MUL  (R1, R0);      // 0x14  (3) << loop
    DEC  (R0);          // 0x17  (2)
    MOV  (R2, 0);       // 0x19  (6)
    CMP  (R0, R2);      // 0x1F  (3)
    JNE  (0x14);        // 0x22  (5)
    RET  ();            // 0x27  (1)
    fclose(g_f);
    printf("  factorial.bin   40 bytes  computes 5! = 120  (uses CALL/RET)\n");
}

// ─────────────────────────────────────────────────────────────────────────────
// fibonacci.bin  —  prints first 10 Fibonacci numbers: 0 1 1 2 3 5 8 13 21 34
//
// Assembly:
//   0x00  MOV  R0, 0        ; a = 0
//   0x06  MOV  R1, 1        ; b = 1
//   0x0C  MOV  R2, 10       ; count = 10
//   0x12  PRINT R0          ; << loop: print a
//   0x14  MOVR R3, R0       ; tmp = a
//   0x17  MOVR R0, R1       ; a = b
//   0x1A  ADD  R1, R3       ; b = b + tmp  (old a)
//   0x1D  DEC  R2           ; count--
//   0x1F  MOV  R4, 0        ; zero reference
//   0x25  CMP  R2, R4       ; check count == 0
//   0x28  JNE  0x12         ; if not zero, loop
//   0x2D  HALT
// ─────────────────────────────────────────────────────────────────────────────
static void make_fibonacci(void) {
    g_f = fopen("programs/fibonacci.bin", "wb");
    MOV  (R0, 0);       // 0x00  (6)
    MOV  (R1, 1);       // 0x06  (6)
    MOV  (R2, 10);      // 0x0C  (6)
    PRINT(R0);          // 0x12  (2) << loop
    MOVR (R3, R0);      // 0x14  (3)
    MOVR (R0, R1);      // 0x17  (3)
    ADD  (R1, R3);      // 0x1A  (3)
    DEC  (R2);          // 0x1D  (2)
    MOV  (R4, 0);       // 0x1F  (6)
    CMP  (R2, R4);      // 0x25  (3)
    JNE  (0x12);        // 0x28  (5)
    HALT ();            // 0x2D  (1)
    fclose(g_f);
    printf("  fibonacci.bin   46 bytes  prints 0 1 1 2 3 5 8 13 21 34\n");
}

// ─────────────────────────────────────────────────────────────────────────────
// input_test.bin  —  reads an integer N from the user, prints N * 2
//
// Assembly:
//   0x00  READ  R0          ; R0 = user input
//   0x02  MOV   R1, 2       ; R1 = 2
//   0x08  MUL   R0, R1      ; R0 = R0 * 2
//   0x0B  PRINT R0          ; print result
//   0x0D  HALT
// ─────────────────────────────────────────────────────────────────────────────
static void make_input_test(void) {
    g_f = fopen("programs/input_test.bin", "wb");
    READ (R0);          // 0x00  (2)
    MOV  (R1, 2);       // 0x02  (6)
    MUL  (R0, R1);      // 0x08  (3)
    PRINT(R0);          // 0x0B  (2)
    HALT ();            // 0x0D  (1)
    fclose(g_f);
    printf("  input_test.bin  14 bytes  reads N, prints N*2\n");
}

// ─────────────────────────────────────────────────────────────────────────────

int main(void) {
    printf("Generating test programs in programs/:\n");
    make_countdown();
    make_factorial();
    make_fibonacci();
    make_input_test();
    printf("Done.\n");
    return 0;
}
