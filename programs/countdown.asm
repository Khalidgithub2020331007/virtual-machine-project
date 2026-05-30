; Countdown: prints 10, 9, 8 ... 0 then halts
    MOV  R0, 10
loop:
    PRINT R0
    MOV   R1, 0
    CMP   R0, R1
    JE    done
    DEC   R0
    JMP   loop
done:
    HALT
