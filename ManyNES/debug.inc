DUMPB   MACRO   BYTE_VAL
        LOCAL   @@SKIP1, @@SKIP2
        IFDEF   DEBUG
        PUSHAD
        PUSHF
        MOV     BL, BYTE_VAL
        MOV     DL, BL
        SHR     DL, 4
        CMP     DL, 9
        JBE     @@SKIP1
        ADD     DL, 'A' - '0' - 10
@@SKIP1:
        ADD     DL, '0'
        MOV     AH, 2
        INT     21H
        MOV     DL, BL
        AND     DL, 0FH
        CMP     DL, 9
        JBE     @@SKIP2
        ADD     DL, 'A' - '0' - 10
@@SKIP2:
        ADD     DL, '0'
        MOV     AH, 2
        INT     21H
        POPF
        POPAD
        ENDIF
ENDM

DUMPW   MACRO   WORD_VAL
        IFDEF   DEBUG
        PUSH    EAX
        PUSHF
        MOV     AX, WORD_VAL
        DUMPB   AH
        DUMPB   AL
        POPF
        POP     EAX
        ENDIF
ENDM

DUMPD   MACRO   DWORD_VAL
        IFDEF   DEBUG
        PUSH    EAX
        PUSHF
        MOV     EAX, DWORD_VAL
        ROL     EAX, 16
        DUMPW   AX
        ROL     EAX, 16
        DUMPW   AX
        POPF
        POP     EAX
        ENDIF
ENDM

DUMP    MACRO   INPUT_STRING
        LOCAL   @@CODE_LABEL, @@DATA_LABEL
        IFDEF   DEBUG
        JMP     @@CODE_LABEL
@@DATA_LABEL    DB      INPUT_STRING, 13, 10, '$'
@@CODE_LABEL:
        PUSHAD
        PUSHF
        MOV     AH, 9
        MOV     EDX, OFFSET @@DATA_LABEL
        INT     21H
        ;MOV     AH, 0
        ;INT     16H
        POPF
        POPAD
        ENDIF
ENDM

GETCH   MACRO   INPUT_STRING
        IFDEF   DEBUG
        PUSHAD
        PUSHF
        XOR     AH, AH
        INT     16H
        POPF
        POPAD
        ENDIF
ENDM
