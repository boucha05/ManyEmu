;************
;* 6502.inc *
;************


; === STRUCTURES ============================================================

; The 6502 CPU structure
cpu6502s    struct
        cpu6502_error   dd      ?       ; Pointer on the cpu error function
        cpu6502_a       dd      ?       ; Accumulator (byte)
        cpu6502_x       dd      ?       ; X index register (byte)
        cpu6502_y       dd      ?       ; Y index register (byte)
        cpu6502_stack   dd      ?       ; Stack pointer (byte)
        cpu6502_flags   dd      ?       ; Flags (byte)
        cpu6502_pc      dd      ?       ; Program counter (word)
        cpu6502_pc_base dd      ?       ; Base address for PC
; NOTE: All the datas that are byte or word sized must not exceed their
;       limit($FF for bytes and $FFFF for speed reason).  The read function
;       must preserve the content of AH and set to zero the upper part of
;       EAX.  The write function must use the value in AL as the index in
;       the memory.  The address is assumed to be in the range of $0000 and
;       $FFFF.  If an error occurs then set the CF.  If all is okay, just
;       clear it.
cpu6502s    ends


; === MACROS ================================================================

cpu6502_OKAY            equ     0
cpu6502_ERR_BADOPCODE   equ     1
cpu6502_ERR_READ        equ     2
cpu6502_ERR_WRITE       equ     3


; === DATA ==================================================================

externdef   cpu6502_Loop:       dword
externdef   cpu6502_Done:       dword
externdef   cpu6502_NCycles:    dword
externdef   cpu6502_Base:       dword


; === CODE ==================================================================

externdef   cpu6502_Reset:      proc
externdef   cpu6502_NMI:        proc
externdef   cpu6502_INT:        proc
externdef   cpu6502_Execute:    proc
