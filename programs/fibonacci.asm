; Fibonacci: prints first 10 numbers (0 1 1 2 3 5 8 13 21 34)
    MOV  R0, 0      ; a
    MOV  R1, 1      ; b
    MOV  R2, 10     ; count
loop:
    PRINT R0
    MOV   R3, R0    ; tmp = a  (emits MOVR)
    MOV   R0, R1    ; a = b    (emits MOVR)
    ADD   R1, R3    ; b = b + old_a
    DEC   R2
    MOV   R4, 0
    CMP   R2, R4
    JNE   loop
    HALT
