; Factorial: computes 5! = 120 using CALL/RET
    MOV  R0, 5
    CALL factorial
    PRINT R1
    HALT
factorial:
    MOV  R1, 1
floop:
    MUL  R1, R0
    DEC  R0
    MOV  R2, 0
    CMP  R0, R2
    JNE  floop
    RET
