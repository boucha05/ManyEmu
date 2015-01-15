; ***************************************************************************
; *     A simple 6502 emulator                                              *
; ***************************************************************************

;DEBUG = 1

; === HEADER ================================================================

.586
.model  flat, c
;locals
include debug.inc
include r6502.inc


; === CONSTANTS =============================================================

IRQ_NMI         equ     0fffah          ; Address of the IRQ vectors
IRQ_RESET       equ     0fffch
IRQ_BRK         equ     0fffeh
IRQ_INT         equ     IRQ_BRK

DEFAULT_STACK   equ     0ffh            ; Default value for the stack
DEFAULT_FLAGS   equ     FL_R            ; Default value for the flags

REG_A           equ     bl              ; PC registers used by the 6502
REG_X           equ     cl
REG_Y           equ     ch
REG_FL          equ     bh
CLOCK           equ     esi             ; USE THE CLOCK FOR CYCLES COUNT
STACK_PTR       equ     edi             ; BASE OF THE STACK
REG_PC          equ     bp
REG_PCe         equ     ebp

FL_C            equ     001h            ; Carry Flag
FL_Z            equ     002h            ; Zero Flag
FL_I            equ     004h            ; Interrupt Flag
FL_D            equ     008h            ; Decimal Flag
FL_B            equ     010h            ; Break Flag
FL_R            equ     020h            ; Reserved Flag
FL_V            equ     040h            ; Overflow Flag
FL_S            equ     080h            ; Sign Flag


; === DATA ==================================================================

.data

align

; Global variables

cpu6502_Loop    dd      ?               ; Realtime label for the main loop
cpu6502_Done    dd      ?               ; Realtime end-of-loop/error label
cpu6502_NCycles dd      ?               ; Number of clock cycles to execute
cpu6502_Base    dd      ?               ; Base pointer for PC

; Local variables

cpu_reg_a       dd      0               ; Register: ACCUMULATOR
cpu_reg_x       dd      0               ; Register: X
cpu_reg_y       dd      0               ; Register: Y
cpu_reg_pc      dd      0               ; Register: PROGRAM COUNT
cpu_reg_fl      dd      0               ; Register: flags
cpu_reg_stack   dd      0               ; Register: STACK
flag_c          db      0
flag_z          db      0
flag_v          db      0
flag_s          db      0

clock_adjust    dd      ?               ; Clock adjust for an instruction
temp_fl         dd      0               ; Variable used for temporary flag

fl_lut          label   dword
        I = 0
        REPT    256
        db      I AND FL_C
        db      (I AND FL_Z) XOR FL_Z
        db      I AND FL_V
        db      I AND FL_S
        I = I + 1
        ENDM

rel_lut         label   dword           ; Relative jump look-up table
        I = 0
        REPT    128
        dd      I
        I = I + 1
        ENDM
        I = 0
        REPT    128
        dd      -I
        I = I + 1
        ENDM

;vflag_rflag_lut:                        ; Virtual flag to real flag lut
;rflag_vflag_lut:                        ; Real flag to virtual flag lut


; === CODE ==================================================================

.code

; ***** OPCODE FUNCTIONS ****************************************************

; Beginning of an opcode without incrementation of REG_PC
op_begin_x      macro   op
        align
        Opcode&op:
endm


; Beginning of an opcode with incrementation of REG_PC
op_begin        macro   op
        op_begin_x %op
        inc     REG_PC
endm


; End of an opcode
op_end          macro   clk
        local   DoneClock
        sub     CLOCK, clk
        jmp     cpu6502_Loop

        jle     DoneClock
        mov     al, [REG_PCe]
        and     eax, 0ffh
        jmp     dword ptr [instr_lut + eax*4]

        ;jmp     cpu6502_Loop
;DefaultLoop:
;        jle     DefaultDone
;FirstLoop:
;        mov     al, [REG_PCe]
;        jmp     dword ptr [instr_lut + eax*4]
DoneClock:
        jmp     cpu6502_Done
endm


; ***************************************************************************
; *** Architecture dependant functions **************************************
; ***************************************************************************

extern  NES_Read2007: proc
extern  NES_Write: proc

; ***** READ/WRITE FUNCTIONS ************************************************

; Read a byte
readb           macro
        local   Skip, Done
        cmp     edx, 2007h
        jne     Skip
        call    NES_Read2007
        jmp     Done
Skip:
        add     edx, cpu6502_Base
        mov     al, [edx]
        sub     edx, cpu6502_Base
Done:
endm

; Read a word
readw           macro
        add     edx, cpu6502_Base
        mov     ax, [edx]
        sub     edx, cpu6502_Base
endm

; Write a byte
writeb          macro
        push    edi
        call    NES_Write
        pop     edi
endm

GeneratePC      macro
        sub     REG_PCe, cpu6502_Base
endm

; Degenerate the value of PC in a 32 bit pointer
DegeneratePC    macro
        add     REG_PCe, cpu6502_Base
endm

; Fetch a byte
fetchb          macro   reg
        mov     reg, byte ptr [REG_PCe]
        inc     REG_PC
endm

; Fetch a word
fetchw          macro   reg
        mov     reg, word ptr [REG_PCe]
        add     REG_PC, 2
endm

; Push a byte to the stack
pushb           macro   reg        
        DUMPD   STACK_PTR
        DUMP    " = pushb"
        DUMPB   reg
        DUMP    " = value"
        add     STACK_PTR, cpu6502_Base
        mov     byte ptr [STACK_PTR + 100h + 0000h], reg
        mov     byte ptr [STACK_PTR + 100h + 0800h], reg
        mov     byte ptr [STACK_PTR + 100h + 1000h], reg
        mov     byte ptr [STACK_PTR + 100h + 1800h], reg
        sub     STACK_PTR, cpu6502_Base
        dec     STACK_PTR
endm

; Push a word to the stack
push16           macro   reg
        DUMPD   STACK_PTR
        DUMP    " = pushw"
        DUMPW   reg
        DUMP    " = value"
        add     STACK_PTR, cpu6502_Base
        mov     word ptr [STACK_PTR + 0ffh + 0000h], reg
        mov     word ptr [STACK_PTR + 0ffh + 0800h], reg
        mov     word ptr [STACK_PTR + 0ffh + 1000h], reg
        mov     word ptr [STACK_PTR + 0ffh + 1800h], reg
        sub     STACK_PTR, cpu6502_Base
        sub     STACK_PTR, 2
endm


; Pop a byte from the stack
popb            macro   reg
        DUMPD   STACK_PTR
        DUMP    " = popb"
        add     STACK_PTR, cpu6502_Base
        inc     STACK_PTR
        mov     reg, byte ptr [STACK_PTR + 100h]
        sub     STACK_PTR, cpu6502_Base
        DUMPB   reg
        DUMP    " = value"
endm


; Pop a word from the stack
popw            macro   reg        
        DUMPD   STACK_PTR
        DUMP    " = popw"
        add     STACK_PTR, cpu6502_Base
        add     STACK_PTR, 2
        mov     reg, word ptr [STACK_PTR + 0ffh]
        sub     STACK_PTR, cpu6502_Base
        DUMPW   reg
        DUMP    " = value"
endm


; ***************************************************************************


; Set a flag depending of the value of a 32 bits register (must be 0 or 1)
set_C           macro   r
        setc    flag_c
endm

set_Z           macro   r
        mov     flag_z, r
endm

set_V           macro   r
        seto    flag_v
endm

set_S           macro   r
        mov     flag_s, r
endm


; Test a flag
test_fl         macro   fl
        IFIDNI  <fl>, <flag_s>
        test    byte ptr flag_s, -1
        ELSE
        IFIDNI  <fl>, <flag_z>
        test    byte ptr flag_z, -1
        ELSE
        test    fl, -1
        ENDIF
        ENDIF
endm

; Generate a flag
GenerateFlags   macro
        local   SkipZero
        push    edx
        and     REG_FL, FL_R OR FL_D OR FL_I OR FL_B
        or      REG_FL, flag_c
        and     flag_s, FL_S
        or      REG_FL, flag_s
        test    flag_z, -1
        jnz     SkipZero
        or      REG_FL, FL_Z
SkipZero:
        mov     dl, flag_v
        shl     dl, 6
        or      REG_FL, dl
        pop     edx
endm

; Degenerate a flag
DegenerateFlags macro   fl
        push    edx
        xor     edx, edx
        mov     dl, REG_FL
        mov     edx, [fl_lut + edx]
        mov     dword ptr flag_c, edx
        pop     edx
endm


; Get the virtual carry flag in the real carry flag
get_c           macro
        bt      word ptr flag_c, FL_C
endm

get_cc          macro
        btc     word ptr flag_c, FL_C
endm


; ***** PUBLIC FUNCTIONS ****************************************************


align

; *** cpu6502_Reset *********************************************************
;       Executes a reset of the CPU.  This function clears all the registers
; and set FLAGS, STACK and PC to their initial value.
;
; Syntax:       void cpu6502_Reset(struct cpu6502 *CPU)
;
; In:           ebx =   Pointer on the CPU structure
;
; Out:          eax =   Error code (0 = okay)
; ***************************************************************************
cpu6502_Reset proc
        ; Fill the registers with the appropriates values
        xor     eax, eax
        mov     byte ptr cpu_reg_a, al
        mov     byte ptr cpu_reg_x, al
        mov     byte ptr cpu_reg_y, al
        mov     cpu_reg_stack, DEFAULT_STACK
        mov     cpu_reg_fl, DEFAULT_FLAGS
        mov     eax, [ebx + cpu6502s.cpu6502_pc_base]
        mov     cpu6502_Base, eax

        ; Get the entry point
        mov     cpu6502_Done, offset @@Done
        mov     cpu6502_Loop, offset @@Done
        add     eax, IRQ_RESET
        mov     eax, [eax]
        and     eax, 0ffffh
@@Done:
        mov     cpu_reg_pc, eax

        xor     eax, eax
@@EndFunc:
        ; Set the function pointers
        mov     cpu6502_Loop, offset DefaultLoop
        mov     cpu6502_Done, offset DefaultDone
        ret
@@Error:
        or      eax, -1
        jmp     @@EndFunc
cpu6502_Reset endp


; *** cpu6502_NMI ***********************************************************
;       Executes a non-maskable interrupt.
;
; In:           None
;
; Out:          None
; ***************************************************************************
cpu6502_NMI proc
        push    eax
        push    edx
        push    edi

        DUMP    "********** NMI **********"
        ;*;mov     REG_PCe, cpu_reg_pc
        ;*;DegeneratePC
        ;*;mov     STACK_PTR, cpu_reg_stack

        ; Save the value of PC on the stack
        mov     STACK_PTR, cpu_reg_stack
        mov     eax, cpu_reg_pc
        pushw   ax

        ; Clear the BREAK flag and push the flags on the stack
        mov     al, byte ptr cpu_reg_fl
        and     al, NOT FL_B
        pushb   al
        mov     cpu_reg_stack, STACK_PTR

        ; Set the INTERRUPT flag and save the flags
        or      al, FL_I
        mov     byte ptr cpu_reg_fl, al

        ; Set PC equal to the value of the NMI vector
        mov     edx, IRQ_NMI
        readw
        DUMPD   eax
        DUMP    " = NMI value for PC"
        mov     word ptr cpu_reg_pc, ax

        ; Update the clock count
        sub     cpu6502_NCycles, 7

        pop     edi
        pop     edx
        pop     eax
        ret
cpu6502_NMI endp


; *** cpu6502_INT ***********************************************************
;       Executes a maskable interrupt.
;
; In:           None
;
; Out:          None
; ***************************************************************************
cpu6502_INT proc
        ; Verify that we can call the interrupt
        test    cpu_reg_fl, FL_I
        jnz     @@Done
        push    eax
        push    edx
        push    edi

        ; Save the value of PC on the stack
        mov     STACK_PTR, cpu_reg_stack
        mov     eax, cpu_reg_pc
        pushw   ax

        ; Clear the BREAK flag and push the flags on the stack
        mov     al, byte ptr cpu_reg_fl
        and     al, NOT FL_B
        pushb   al
        mov     cpu_reg_stack, STACK_PTR

        ; Set the INTERRUPT flag and save the flags
        or      al, FL_I
        mov     byte ptr cpu_reg_fl, al

        ; Set PC equal to the value of the INT vector
        mov     edx, IRQ_INT
        readw
        mov     word ptr cpu_reg_pc, ax

        ; Update the clock count
        sub     cpu6502_NCycles, 7

        pop     edi
        pop     edx
        pop     eax
@@Done:
        ret
cpu6502_INT endp


; *** cpu6502_Execute *******************************************************
;       Executes a certain number of pseudo clock cycles of the 6502 cpu.  It
; use the cpu6502_readb function pointer to read a byte from memory and the
; cpu6502_writeb function pointer to write a byte to memory.  The number of
; clock cycles used is cpu6502_NCycles.
;
; In:           None
;
; Out:          eax =   Error code (0 = okay)
;
;       NOTE:   The number of clock cycles is a minimum value so a value of 1
;               will trace an instruction.
; ***************************************************************************
cpu6502_Execute proc
        push    ebx
        push    ecx
        push    edx
        push    esi
        push    edi
        push    ebp

        ; Read the current CPU values
        mov     REG_A, byte ptr cpu_reg_a
        mov     REG_X, byte ptr cpu_reg_x
        mov     REG_Y, byte ptr cpu_reg_y
        mov     REG_FL, byte ptr cpu_reg_fl
        DegenerateFlags
        mov     REG_PCe, cpu_reg_pc
        DegeneratePC
        mov     CLOCK, cpu6502_NCycles
        mov     STACK_PTR, cpu_reg_stack

        ; --- Use of the registers ---
        ; eax = general register & values for read/write
        ; bl =  accumulator (REG_A)
        ; bh =  flags
        ; cl =  x (REG_X)
        ; ch =  y (REG_Y)
        ; edx = memory address for read/write
        ; esi = CLOCK ;;;program count (no more REG_PC)
        ; edi = general register that can be modified by readb and writeb
        ; ebp = REG_PC (fetchb, fetchw, GeneratePC, DegeneratePC)

        ;xor     CLOCK, CLOCK
        ;jmp     DefaultDone

        ; Main loop for instruction processing
        xor     eax, eax                ; Clear eax
        jmp     FirstLoop
DefaultLoop label ptr
        jle     DefaultDoneLocal
FirstLoop:
        mov     al, [REG_PCe]
        ;DUMPD   REG_PCe
        ;DUMP    " = REG_PCe"
        ;DUMPD   eax
        ;DUMP    " = opcode in [REG_PCe]"
        ;GETCH
        jmp     dword ptr [instr_lut + eax*4]

        ; Save the cpu values
DefaultDone label ptr
DefaultDoneLocal:
        mov     cpu6502_NCycles, CLOCK
        mov     cpu_reg_stack, STACK_PTR
        mov     byte ptr cpu_reg_a, REG_A
        mov     byte ptr cpu_reg_x, REG_X
        mov     byte ptr cpu_reg_y, REG_Y
        GenerateFlags
        mov     byte ptr cpu_reg_fl, REG_FL
        GeneratePC
        mov     cpu_reg_pc, REG_PCe

        xor     eax, eax
        pop     ebp
        pop     edi
        pop     esi
        pop     edx
        pop     ecx
        pop     ebx
        ret
cpu6502_Execute endp


; ***************************************************************************
; ***** LOAD/STORE ADDRESSING MODES *****************************************
; ***************************************************************************

; --- Accumulator ---
lda_acc         macro
        mov     al, REG_A
endm

clr_acc         macro
endm


; --- Implied ---
lda_imp         macro
endm

clr_imp         macro
endm


; --- Immediate ---
lda_imm         macro
        fetchb  al
endm

clr_imm         macro
endm


; --- Zero-page ---
lea_zero        macro
        fetchb  dl                      ; Fetch a byte in dl then clear dh
        xor     dh, dh
endm

lda_zero        macro
        lea_zero
        add     edx, cpu6502_Base
        mov     al, [edx]
        sub     edx, cpu6502_Base
endm

clr_zero        macro
endm


; --- Zero-page, X ---
lea_zero_x      macro
        fetchb  dl                      ; Fetch a byte in dl and add REG_X
        add     dl, REG_X               ; then clear dh
        xor     dh, dh
endm

lda_zero_x      macro
        lea_zero_x
        add     edx, cpu6502_Base
        mov     al, [edx]
        sub     edx, cpu6502_Base
endm

clr_zero_x      macro
endm


; --- Zero-page, Y ---
lea_zero_y      macro
        fetchb  dl                      ; Fetch a byte in dl and add REG_Y
        add     dl, REG_Y               ; then clear dh
        xor     dh, dh
endm

lda_zero_y      macro
        lea_zero_y
        add     edx, cpu6502_Base
        mov     al, [edx]
        sub     edx, cpu6502_Base
endm

clr_zero_y      macro
endm


; --- Absolute ---
lea_abs         macro
        fetchw  dx                      ; Fetch a word in dx
endm

lda_abs         macro
        lea_abs
        readb
endm

clr_abs         macro
endm


; --- Absolute, X ---
lea_abs_x       macro
        fetchw  dx                      ; Fetch a word in dx then add the
        add     dl, REG_X               ; value of X with page boundary
        adc     dh, 0
endm

lda_abs_x       macro
        lea_abs_x
        readb
endm

clr_abs_x       macro
endm


; --- Absolute, X with page boundary verification ---
lda_abs_x_p     macro
        local   Skip

        fetchw  dx                      ; Fetch a word in dx then add the
        add     dl, REG_X               ; value of X with page boundary

        inc     CLOCK                   ; If the carry flag is set, recover
        jc      Skip                    ; a clock cycle then increase dh
        dec     CLOCK
        inc     dh
Skip:
        readb
endm

clr_abs_x_p     macro
endm


; --- Absolute, Y ---
lea_abs_y       macro
        fetchw  dx                      ; Fetch a word in dx then add the
        add     dl, REG_Y               ; value of Y without page boundary
        adc     dh, 0
endm

lda_abs_y       macro
        lea_abs_y
        readb
endm

clr_abs_y       macro
endm


; --- Absolute, Y with page boundary verification ---
lda_abs_y_p     macro
        local   Skip

        fetchw  dx                      ; Fetch a word in dx then add the
        add     dl, REG_Y               ; value of Y with page boundary

        inc     CLOCK                   ; If the carry flag is set, recover
        jc      Skip                    ; a clock cycle then increase dh
        dec     CLOCK
Skip:
        adc     dh, 0
        readb
endm

clr_abs_y_p     macro
endm


; --- (Indirect, X) ---
lea_ind_x       macro
        fetchb  dl                      ; Fetch a byte in dl and add X to it
        add     dl, REG_X               ; then clear dh
        xor     dh, dh

        add     edx, cpu6502_Base
        mov     ax, [edx]
        sub     edx, cpu6502_Base
        mov     dx, ax

        readw                           ; Read a word to get the address of
        mov     edx, eax                ; the byte to read
endm

lda_ind_x       macro
        lea_ind_x
        readb
endm

clr_ind_x       macro
        xor     eax, eax
endm


; --- (Indirect), Y ---
lea_ind_y       macro
        fetchb  dl                      ; Fetch a byte in dl and zero-page it
        xor     dh, dh

        readw                           ; Read the zero-page address then add
        mov     edx, eax
        add     dl, REG_Y               ; the value of y to it
        adc     dh, 0
endm

lda_ind_y       macro
        lea_ind_y
        readb
endm

clr_ind_y       macro
        xor     eax, eax
endm


; --- (Indirect), Y with page boundary ---
lda_ind_y_p     macro
        local   Skip
        fetchb  dl                      ; Fetch a byte in dl and zero-page it
        xor     dh, dh

        readw                           ; Read the zero-page address then add
        mov     edx, eax                ; the value of y to it
        add     dl, REG_Y

        inc     CLOCK                   ; If the carry flag is set, recover
        jc      Skip                    ; a clock cycle then increase dh
        dec     CLOCK
Skip:
        adc     dh, 0
        readb
endm

clr_ind_y_p     macro
        xor     eax, eax
endm



; ***************************************************************************
; ***** INSTRUCTION SET *****************************************************
; ***************************************************************************

; =====> Load, store and transfer instructions <=====

op_LDA          macro   store, etype, register, opcode, clk
op_begin        opcode
        IF      store
        lea_&etype
        mov     al, register
        set_S   al
        set_Z   al
        writeb
        ELSE
        lda_&etype
        mov     register, al
        set_S   al   
        set_Z   al
        ENDIF
        clr_&etype
op_end          clk
endm

op_LDA  0, imm,    REG_A, 0a9h, 2      ; --- LDA ---
op_LDA  0, zero,   REG_A, 0a5h, 3
op_LDA  0, zero_x, REG_A, 0b5h, 4
op_LDA  0, abs,    REG_A, 0adh, 4
op_LDA  0, abs_x_p,REG_A, 0bdh, 4
op_LDA  0, abs_y_p,REG_A, 0b9h, 4
op_LDA  0, ind_x,  REG_A, 0a1h, 6
op_LDA  0, ind_y_p,REG_A, 0b1h, 5

op_LDA  0, imm,    REG_X, 0a2h, 2      ; --- LDX ---
op_LDA  0, zero,   REG_X, 0a6h, 3
op_LDA  0, zero_y, REG_X, 0b6h, 4
op_LDA  0, abs,    REG_X, 0aeh, 4
op_LDA  0, abs_y_p,REG_X, 0beh, 4

op_LDA  0, imm,    REG_Y, 0a0h, 2      ; --- LDY ---
op_LDA  0, zero,   REG_Y, 0a4h, 3
op_LDA  0, zero_x, REG_Y, 0b4h, 4
op_LDA  0, abs,    REG_Y, 0ach, 4
op_LDA  0, abs_x_p,REG_Y, 0bch, 4

op_LDA  1, zero,   REG_A, 085h, 3      ; --- STA ---
op_LDA  1, zero_x, REG_A, 095h, 4
op_LDA  1, abs,    REG_A, 08dh, 4
op_LDA  1, abs_x,  REG_A, 09dh, 5
op_LDA  1, abs_y,  REG_A, 099h, 5
op_LDA  1, ind_x,  REG_A, 081h, 6
op_LDA  1, ind_y,  REG_A, 091h, 6

op_LDA  1, zero,   REG_X, 086h, 3      ; --- STX ---
op_LDA  1, zero_y, REG_X, 096h, 4
op_LDA  1, abs,    REG_X, 08eh, 4

op_LDA  1, zero,   REG_Y, 084h, 3      ; --- STY ---
op_LDA  1, zero_x, REG_Y, 094h, 4
op_LDA  1, abs,    REG_Y, 08ch, 4


op_Txx          macro   reg_s, reg_d, opcode, clk
op_begin        opcode
        mov     reg_d, reg_s
        IFE     clk AND 01h
        set_S   reg_d
        set_Z   reg_d
        ENDIF
op_end          clk AND 02h
endm

op_Txx  REG_A, REG_X, 0aah, 2                          ; --- TAX ---
op_Txx  REG_A, REG_Y, 0a8h, 2                          ; --- TAY ---
op_Txx  REG_X, REG_A, 08ah, 2                          ; --- TXA ---
op_Txx  REG_Y, REG_A, 098h, 2                          ; --- TYA ---

op_begin        %0bah                   ; --- TSX ---
        mov     REG_X, byte ptr cpu_reg_stack
        set_S   al
        set_Z   al
op_end          2

op_begin        %09ah                   ; --- TXS ---
        mov     byte ptr cpu_reg_stack, REG_X
op_end          2


; =====> Arithmetics operators <=====

op_ADC          macro   add_sub, etype, opcode, clk
        local   DecimalMode
op_begin        opcode
        lda_&etype
        test    REG_FL, FL_D
        jnz     DecimalMode
        get_c
        IF      add_sub
        xor     al, -1
        ENDIF
        adc     REG_A, al
        set_C   al
        set_V   al
        test    REG_A, REG_A
        set_S   al
        set_Z   al
        clr_&etype
op_end          clk
DecimalMode:
        get_cc
        IF      add_sub
        sbb     REG_A, al
        mov     al, REG_A
        das
        cmc
        ELSE
        adc     al, REG_A
        daa
        mov     REG_A, al
        ENDIF
        set_C   al
        set_V   al
        set_S   al
        set_Z   al
        clr_&etype
op_end          clk
endm

op_ADC  0, imm,    %069h, 2             ; --- ADC ---
op_ADC  0, zero,   %065h, 3
op_ADC  0, zero_x, %075h, 4
op_ADC  0, abs,    %06dh, 4
op_ADC  0, abs_x_p,%07dh, 4
op_ADC  0, abs_y_p,%079h, 4
op_ADC  0, ind_x,  %061h, 6
op_ADC  0, ind_y_p,%071h, 5

op_ADC  1, imm,    %0e9h, 2             ; --- SBC ---
op_ADC  1, zero,   %0e5h, 3
op_ADC  1, zero_x, %0f5h, 4
op_ADC  1, abs,    %0edh, 4
op_ADC  1, abs_x_p,%0fdh, 4
op_ADC  1, abs_y_p,%0f9h, 4
op_ADC  1, ind_x,  %0e1h, 6
op_ADC  1, ind_y_p,%0f1h, 5


op_ASL          macro   operate, rotate, dest, etype, opcode, clk
op_begin        opcode
        lda_&etype
        IF      rotate
        get_c
        ENDIF
        operate al, 1
        set_C   al
        set_S   al
        set_Z   al
        IFIDNI  <dest>,<REG_A>
        mov     REG_A, ah
        ELSE
        writeb
        ENDIF
        clr_&etype
op_end          clk
endm

op_ASL  shl, 0, REG_A, acc,    %00ah, 2 ; --- ASL ---
op_ASL  shl, 0, 0,     zero,   %006h, 5
op_ASL  shl, 0, 0,     zero_x, %016h, 6
op_ASL  shl, 0, 0,     abs,    %00eh, 6
op_ASL  shl, 0, 0,     abs_x,  %01eh, 7

op_ASL  shr, 0, REG_A, acc,    %04ah, 2 ; --- LSR ---
op_ASL  shr, 0, 0,     zero,   %046h, 5
op_ASL  shr, 0, 0,     zero_x, %056h, 6
op_ASL  shr, 0, 0,     abs,    %04eh, 6
op_ASL  shr, 0, 0,     abs_x,  %05eh, 7

op_ASL  rol, 1, REG_A, acc,    %02ah, 2 ; --- ROL ---
op_ASL  rol, 1, 0,     zero,   %026h, 5
op_ASL  rol, 1, 0,     zero_x, %036h, 6
op_ASL  rol, 1, 0,     abs,    %02eh, 6
op_ASL  rol, 1, 0,     abs_x,  %03eh, 7

op_ASL  ror, 1, REG_A, acc,    %06ah, 2 ; --- ROR ---
op_ASL  ror, 1, 0,     zero,   %066h, 5
op_ASL  ror, 1, 0,     zero_x, %076h, 6
op_ASL  ror, 1, 0,     abs,    %06eh, 6
op_ASL  ror, 1, 0,     abs_x,  %07eh, 7


op_DEC_INC      macro   operate, etype, mem_reg, opcode, clk
op_begin        opcode
        lda_&etype
        IFIDNI  <mem_reg>, <0>
        operate al
        ELSE
        operate mem_reg
        ENDIF
        IFIDNI  <mem_reg>, <0>
        writeb
        set_S   al
        set_Z   al
        ELSE
        set_S   mem_reg
        set_Z   mem_reg
        ENDIF
        clr_&etype
op_end          clk
endm

op_DEC_INC dec, zero,   0,     %0c6h, 5 ; --- DEC ---
op_DEC_INC dec, zero_x, 0,     %0d6h, 6
op_DEC_INC dec, abs,    0,     %0ceh, 6
op_DEC_INC dec, abs_x,  0,     %0deh, 7
op_DEC_INC dec, imp,    REG_X, %0cah, 2 ; --- DEX ---
op_DEC_INC dec, imp,    REG_Y, %088h, 2 ; --- DEY ---

op_DEC_INC inc, zero,   0,     %0e6h, 5 ; --- INC ---
op_DEC_INC inc, zero_x, 0,     %0f6h, 6
op_DEC_INC inc, abs,    0,     %0eeh, 6
op_DEC_INC inc, abs_x,  0,     %0feh, 7
op_DEC_INC inc, imp,    REG_X, %0e8h, 2 ; --- INX ---
op_DEC_INC inc, imp,    REG_Y, %0c8h, 2 ; --- INY ---



; =====> Logical operator <=====

op_LOGICAL      macro   operate, etype, opcode, clk
op_begin        opcode
        lda_&etype
        operate REG_A, al
        set_Z   al
        set_S   al
        clr_&etype
op_end          clk
endm

op_LOGICAL and, imm,    %029h, 2        ; --- AND ---
op_LOGICAL and, zero,   %025h, 3
op_LOGICAL and, zero_x, %035h, 4
op_LOGICAL and, abs,    %02dh, 4
op_LOGICAL and, abs_x_p,%03dh, 4
op_LOGICAL and, abs_y_p,%039h, 4
op_LOGICAL and, ind_x,  %021h, 6
op_LOGICAL and, ind_y_p,%031h, 5

op_LOGICAL xor, imm,    %049h, 2        ; --- EOR ---
op_LOGICAL xor, zero,   %045h, 3
op_LOGICAL xor, zero_x, %055h, 4
op_LOGICAL xor, abs,    %04dh, 4
op_LOGICAL xor, abs_x_p,%05dh, 4
op_LOGICAL xor, abs_y_p,%059h, 4
op_LOGICAL xor, ind_x,  %041h, 6
op_LOGICAL xor, ind_y_p,%051h, 5

op_LOGICAL or,  imm,    %009h, 2        ; --- ORA ---
op_LOGICAL or,  zero,   %005h, 3
op_LOGICAL or,  zero_x, %015h, 4
op_LOGICAL or,  abs,    %00dh, 4
op_LOGICAL or,  abs_x_p,%01dh, 4
op_LOGICAL or,  abs_y_p,%019h, 4
op_LOGICAL or,  ind_x,  %001h, 6
op_LOGICAL or,  ind_y_p,%011h, 5


op_BIT          macro   etype, opcode, clk
op_begin        opcode
        lda_&etype
        and     al, REG_A
        set_Z   al
        set_S   al
        shr     al, 6
        and     al, 1
        mov     flag_v, al
        clr_&etype
op_end          clk
endm

op_BIT  zero,   %024h, 3                ; --- BIT ---
op_BIT  abs,    %02ch, 4



; =====> Flags operator <=====

op_SEx          macro   fl, value, opcode
op_begin        opcode
        IFIDNI  <fl>, <FL_D>
        IF      VALUE
        or      REG_FL, FL_D
        ELSE
        and     REG_FL, NOT FL_D
        ENDIF
        ELSE
        IFIDNI  <fl>, <FL_I>
        IF      VALUE
        or      REG_FL, FL_I
        ELSE
        and     REG_FL, NOT FL_I
        ENDIF
        ELSE
        mov     fl, value
        ENDIF
        ENDIF
op_end          2
endm

op_SEx  flag_c, 0  %018h                ; --- CLC ---
op_SEx  FL_D,   0, %0d8h                ; --- CLD ---
op_SEx  FL_I,   0, %058h                ; --- CLI ---
op_SEx  flag_v, 0, %0b8h                ; --- CLV ---

op_SEx  flag_c, 1, %038h                ; --- SEC ---
op_SEx  FL_D,   1, %0f8h                ; --- SED ---
op_SEx  FL_I,   1, %078h                ; --- SEI ---


; =====> Comparison instructions <=====

op_CMP          macro   etype, register, opcode, clk
op_begin        opcode
        lda_&etype
        cmp     register, al
        set_C   al
        set_S   al
        set_Z   al
        clr_&etype
op_end          clk
endm

op_CMP  imm,    REG_A, %0c9h, 2         ; --- CMP ---
op_CMP  zero,   REG_A, %0c5h, 3
op_CMP  zero_x, REG_A, %0d5h, 4
op_CMP  abs,    REG_A, %0cdh, 4
op_CMP  abs_x_p,REG_A, %0ddh, 4
op_CMP  abs_y_p,REG_A, %0d9h, 4
op_CMP  ind_x,  REG_A, %0c1h, 6
op_CMP  ind_y_p,REG_A, %0d1h, 5

op_CMP  imm,    REG_X, %0e0h, 2         ; --- CPX ---
op_CMP  zero,   REG_X, %0e4h, 3
op_CMP  abs,    REG_X, %0ech, 4

op_CMP  imm,    REG_Y, %0c0h, 2         ; --- CPY ---
op_CMP  zero,   REG_Y, %0c4h, 3
op_CMP  abs,    REG_Y, %0cch, 4



; =====> Branch and jump instructions <=====

op_Bxx          macro   fl, z_nz, opcode
        local   Done, SkipAddCycle
op_begin_x      %opcode
        test_fl fl
        j&z_nz  Done

        inc     REG_PC
        fetchb  al
        movsx   eax, al         ; 4 cycles (check page!!!)
        add     REG_PCe, eax    ; IF PAGE IS THE SAME, DON'T DEGENERATE

;        GeneratePC
;        lea     edx, [esi + eax]
;        and     esi, 0ff00h
;        xor     dl, dl
;        cmp     si, dx
;        je      SkipAddCycle
;        dec     CLOCK
;SkipAddCycle:
;        add     REG_PC, ax
        xor     eax, eax
op_end          4
Done:
        add     REG_PC, 2
op_end          2
endm

op_Bxx  flag_c, z, %090h                ; --- BCC ---
op_Bxx  flag_c, nz,%0b0h                ; --- BCS ---
op_Bxx  flag_z, nz,%0f0h                ; --- BEQ ---
op_Bxx  flag_s, ns,%030h                ; --- BMI ---
op_Bxx  flag_z, z, %0d0h                ; --- BNE ---
op_Bxx  flag_s, s, %010h                ; --- BPL ---
op_Bxx  flag_v, z, %050h                ; --- BVC ---
op_Bxx  flag_v, nz,%070h                ; --- BVS ---


op_JMP          macro   ind, opcode, clk
op_begin        opcode
        IF      ind
        mov     dx, [REG_PCe]
        readw
        mov     REG_PCe, eax
        ELSE
        mov     REG_PC, [REG_PCe]
        ENDIF
        and     REG_PCe, 0ffffh
        DegeneratePC
op_end          clk
endm

op_JMP  0, %04ch, 3                     ; --- JMP ---
op_JMP  1, %06ch, 5

op_begin        %020h                   ; --- JSR ---
        mov     ax, [REG_PCe]
        add     REG_PC, 2
        GeneratePC
        pushw   REG_PC
        mov     REG_PC, ax

        and     REG_PCe, 0ffffh
        DegeneratePC
        xor     eax, eax
op_end          6

op_begin        %040h                   ; --- RTI ---
        DUMP    "********** RTI **********"
        popb    REG_FL
        DegenerateFlags
        popw    REG_PC
        and     REG_PCe, 0ffffh
        DegeneratePC
op_end          6

op_begin        %060h                   ; --- RTS ---
        popw    REG_PC
        and     REG_PCe, 0ffffh
        DegeneratePC
op_end          6



; =====> Push/pos <=====

op_PSH          macro   reg, opcode, clk
op_begin        opcode
        IF      clk EQ 3
        IFIDNI  <reg>, <REG_FL>
        GenerateFlags
        ENDIF
        pushb   reg
        ELSE
        popb    reg
        IFIDNI  <reg>, <REG_FL>
        DegenerateFlags
        ENDIF
        ENDIF
op_end          clk
endm

op_PSH  REG_A,  %048h, 3                ; --- PHA ---
op_PSH  REG_FL, %008h, 3                ; --- PHP ---
op_PSH  REG_A,  %068h, 4                ; --- PLA ---
op_PSH  REG_FL, %028h, 4                ; --- PLP ---



; =====> Misc opcodes <=====

op_begin        %0eah                   ; --- NOP ---
op_end          2


op_begin        %000h                   ; --- BRK ---
        GeneratePC
        add     REG_PC, 2
        pushw   REG_PC
        GenerateFlags
        pushb   REG_FL
        or      REG_FL, FL_B OR FL_I
        mov     dx, IRQ_BRK
        readw
        mov     REG_PC, ax

        and     REG_PC, 0ffffh
        DegeneratePC
        xor     eax, eax
op_end          7


; =====> INVALID OPCODE <=====

align                                   ; --- ??? ---
OpcodeXX:
        DUMPB   al
        DUMP    ": INVALID OPCODE!!!"
        push    REG_PCe
        GeneratePC
        DUMPD   REG_PCe
        DUMP    ": PC"
        GETCH
        pop     REG_PCe
        sub     CLOCK, 2

        jmp     cpu6502_Loop

        jle     @@DoneClock
        mov     al, [REG_PCe]
        jmp     dword ptr [instr_lut + eax*4]
@@DoneClock:
        jmp     cpu6502_Done



; ***************************************************************************
; ***** INSTRUCTION SET LOOKUP-TABLE ****************************************
; ***************************************************************************

.data

MAKEOP  macro   R
        IFDEF   Opcode&R
                dd      offset Opcode&R ; Store the address of the opcode
        ELSE                            ; function
                dd      offset OpcodeXX ; Generate a Bad Opcode Error
        ENDIF
endm

instr_lut label dword                             ; Instruction set look-up table
        I = 0
        REPT    256
        MAKEOP  %I
        I = I + 1
        ENDM


end
