; ÕÍÍ NES.ASM ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ¸
; ³     Multitarget interface for the NES                                   ³
; ÔÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ¾

;DEBUG   equ     1


; °°° HEADER °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°

.386
.model  flat, c
include debug.inc
include nes.inc
include r6502.inc



; °°° DATA °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°

.data

align

START_DATAS     =       $
; --- Global variables ------------------------------------------------------

conNES_NCycles          dd      ?       ; Number of cycles to execute


; --- Local variables -------------------------------------------------------

; --- NES structure ---
CPU                     dd      ?
ROM                     dd      ?
RAM                     dd      ?

; --- CPU variables ---
CPU_RAM                 dd      ?
CPU_Regs                dd      ?
CPU_Exp                 dd      ?
CPU_WRAM                dd      ?
CPU_Trainer             dd      ?
CPU_PRG_ROM             dd      ?

; --- PPU variables ---
PPU_Pattern_0           dd      ?
PPU_Pattern_1           dd      ?
PPU_Name                dd      ?
PPU_Attribute           dd      ?
PPU_ImagePal            dd      ?
PPU_SpritePal           dd      ?
PPU_SpriteRAM           dd      ?

; --- Misc. variables (needed for savegames) ---
MiscPtr         label   dword

reg2000                 dd      0
reg2001                 dd      0
reg2002                 dd      0
reg2003                 dd      0
reg2004                 dd      0
reg2005a                dd      0
reg2005b                dd      0
reg2006b                db      0
reg2006a                db      0
                        dw      0
reg2007                 dd      0

MiscSize                equ     $ - offset MiscPtr

ScreenPatternTable      dd      ?
SpritePatternTable      dd      ?

NameTable               dd      ?
AttributeTable          dd      ?

IncrementVRAM           dd      1
CurRowNumber            dd      0
FirstHiddenRow          dd      225
RowsPerFrame            dd      262
RowNCycles              dd      114
RowCurCycle             dd      0

END_DATAS       =       $

LAST_MEM_MAPPER equ     0fh
MemoryMapper    dd      offset m00, offset mXX, offset mXX, offset mXX
                dd      offset mXX, offset mXX, offset mXX, offset mXX
                dd      offset mXX, offset mXX, offset mXX, offset mXX
                dd      offset mXX, offset mXX, offset mXX, offset mXX
CallFunc        dd      0


; °°° CODE °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°

.code

align

include NES_IO.ASM

; ÄÄÄ conNES_Reset ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;       Executes a reset of the console.  This function initialize the ROM
; and RAM infos and mappers and send a RESET signal to the CPU.  All the
; variables of the RAM must be set to 0 or to the wanted value.
;
; Syntax:       void conNES_Reset(struct conNES *NES)
;
; In:           eax =   Pointer on the NES structure
;
; Out:          eax =   Error code (0 = okay)
; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
conNES_Reset:
        push    ebx ecx edx

        ; Save the structure first
        mov     ebx, [eax + conNESs.conNES_CPU]
        mov     CPU, ebx
        mov     ecx, [eax + conNESs.conNES_ROM]
        mov     ROM, ecx
        mov     edx, [eax + conNESs.conNES_RAM]
        mov     RAM, edx

        ; Load the RAM's CPU pointers
        lea     eax, [edx + NES_CPU_RAM]
        mov     CPU_RAM, eax
        lea     eax, [edx + NES_CPU_REGS]
        mov     CPU_Regs, eax
        lea     eax, [edx + NES_CPU_EXP]
        mov     CPU_Exp, eax
        lea     eax, [edx + NES_CPU_WRAM]
        mov     CPU_WRAM, eax
        lea     eax, [edx + NES_CPU_TRAINER]
        mov     CPU_Trainer, eax
        lea     eax, [edx + NES_CPU_PRG_ROM]
        mov     CPU_PRG_ROM, eax

        ; Load the RAM's PPU variables
        lea     eax, [edx + NES_PPU_PATTERN_0]
        mov     PPU_Pattern_0, eax
        mov     ScreenPatternTable, eax
        mov     SpritePatternTable, eax
        lea     eax, [edx + NES_PPU_PATTERN_1]
        mov     PPU_Pattern_1, eax
        lea     eax, [edx + NES_PPU_NAME]
        mov     PPU_Name, eax
        lea     eax, [edx + NES_PPU_ATTRIBUTE]
        mov     PPU_Attribute, eax
        lea     eax, [edx + NES_PPU_IMAGEPAL]
        mov     PPU_ImagePal, eax
        lea     eax, [edx + NES_PPU_SPRITEPAL]
        mov     PPU_SpritePal, eax
        lea     eax, [edx + NES_PPU_SPRITERAM]
        mov     PPU_SpriteRAM, eax

        ; Read the ROM header and interpret it
        cmp     dword ptr [ecx], 1a53454eh
        jne     @@Error2
        xor     eax, eax
        mov     al, [ecx + 4]           ; Get the number of 16K PRG-ROM banks
        mov     [edx + NES_H_N_PRG_ROM], eax
        mov     al, [ecx + 5]           ; Get the number of 8K CHR-RAM banks
        mov     [edx + NES_H_N_CHR_RAM], eax
        mov     ax, [ecx + 6]           ; Get the mapper number
        and     ah, 0f0h
        shr     al, 4
        or      al, ah
        xor     ah, ah
        mov     [edx + NES_H_MAPPER], eax
        mov     al, [ecx + 6]           ; Get the cartridge attributes
        and     al, 0fh
        mov     [edx + NES_H_ATTRIB], eax
        lea     eax, [ecx + 16]                 ; Get the address of the
        mov     [edx + NES_H_PRG_ROM], eax      ; first PRG-ROM bank
        mov     eax, [edx + NES_H_N_PRG_ROM]    ; Get the address of the
        shl     eax, 14                         ; first CHR-RAM bank
        lea     eax, [eax + ecx + 16]
        mov     [edx + NES_H_CHR_RAM], eax

        ; Initialize the memory mapper
        mov     eax, [edx + NES_H_MAPPER]
        cmp     eax, LAST_MEM_MAPPER
        ja      @@Error3
        call    dword ptr MemoryMapper[eax*4]
        test    eax, eax
        jnz     @@Error3

        ; Reset any other value needed
        IO_ResetCPU

        ; Do a CPU reset
        mov     eax, CPU_RAM
        mov     [ebx + cpu6502_pc_base], eax
        call    cpu6502_Reset
        test    eax, eax
        jnz     @@Error1

        xor     eax, eax
@@Done:
        pop     edx ecx ebx
        ret

@@Error1:
        mov     eax, NES_ERR_RESET
        jmp     @@Done
@@Error2:
        mov     eax, NES_ERR_NOT_NES
        jmp     @@Done
@@Error3:
        mov     eax, NES_ERR_MAPPER
        jmp     @@Done


; ÄÄÄ conNES_Execute ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;       Execute ecx clock cycles.
;
; Syntax:       void conNES_Execute(void)
;
; In:           None
;
; Out:          eax =   Error code (0 = okay)
; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
conNES_Execute:
        push    edi ebx ecx edx esi ebp

        ; Get a pointer on the RAM structure and on the number of cycles then
        ; keep the number of cycles per row in a register
        mov     ebp, RAM
        mov     edi, conNES_NCycles
        mov     edx, RowNCycles
        mov     esi, RowCurCycle

@@MainLoop:
        ; Get the maximum number of cycles executables
        mov     ecx, edx
        sub     ecx, esi
        cmp     ecx, edi
        jnge    @@ExecuteCode
        mov     ecx, edi

        ; Execute ecx clock cycles
@@ExecuteCode:
        mov     cpu6502_NCycles, ecx
        call    cpu6502_Execute
        test    eax, eax
        jnz     @@Error1

        ; Add the number of clock cycles executed to the counter
        sub     ecx, cpu6502_NCycles
        add     esi, ecx

        ; Check if we must change the display row
        cmp     esi, edx
        jnge    @@UpdateCounter

        ; Change the display row
        sub     esi, edx
        mov     RowCurCycle, esi
        mov     eax, CurRowNumber
        inc     eax

        ; Check if we can draw the done row
        cmp     eax, FirstHiddenRow
        jge     @@SkipDrawCurRow
        IO_DrawCurRow
        jmp     @@DoneIncreaseRow

@@SkipDrawCurRow:
        je      @@BeginVertRetrace
@@DoneVRet:
        cmp     eax, RowsPerFrame
        jnge    @@DoneIncreaseRow
        IO_UpdateFrame
        and     reg2001, not 0c0h
        xor     eax, eax

        ; If necessary, do some post-draw operations on the done row
@@DoneIncreaseRow:
        mov     CurRowNumber, eax

        ; Update the number of clock cycles executed
@@UpdateCounter:
        sub     edi, ecx
        test    edi, edi
        jg      @@MainLoop

        ; esi = RowCurCycle
        ; edx = RowNCycles
        ; ecx = MaxCurNCycles / OldNCycles
        ; ebx = NCycles
        ; edi = ncycles

        xor     eax, eax
@@Done:
        mov     conNES_NCycles, edi
        pop     ebp esi edx ecx ebx edi
        ret

        ; Time for a NMI
@@BeginVertRetrace:
        or      reg2002, 80h
        test    reg2000, 80h
        jnz     @@SkipVRet
        call    cpu6502_NMI
@@SkipVRet:
        jmp     @@DoneVRet

@@Error1:
        DUMP    "@@Error1"
        mov     eax, NES_ERR_EXECUTE
        jmp     @@Done



; ***************************************************************************
; ***** conNES_error ********************************************************
; ***************************************************************************

conNES_error:
        DUMP    "ERROR!"
        ret

conNES_void:
        ret


; ***************************************************************************
; ***** MEMORY MAPPERS ******************************************************
; ***************************************************************************


; --- $0000-$1FFF -----------------------------------------------------------
map_RAM:
        ; Set the RAM write function to the offset of writeb_RAM
        mov     Write_JMP_Table[00h], offset Write_RAM
        mov     Write_JMP_Table[04h], offset Write_RAM
        ret

Write_RAM:
        mov     edi, edx
        and     edi, 07ffh
        add     edi, CPU_RAM
        mov     [edi], al
        mov     [edi + 0800h], al
        mov     [edi + 1000h], al
        mov     [edi + 1800h], al
        ret


; --- $2000-$4FFF -----------------------------------------------------------
map_Registers:
        ; Set the write functions.
        mov     Write_JMP_Table[08h], offset Write_PPU_Register
        mov     Write_JMP_Table[0ch], offset Write_PPU_Register
        mov     Write_JMP_Table[10h], offset Write_IO_Register

        ; Reset the w2005x and w2006x functions
        mov     dword ptr PPU_w + 4*5, offset w2005a
        mov     dword ptr PPU_w + 4*6, offset w2006a

        ; Initialize the registers
        xor     eax, eax
        mov     edi, offset MiscPtr
        mov     ecx, MiscSize / 4
        rep     stosd

        ret


; --- $5000-$5FFF -----------------------------------------------------------
map_No_ExpModule:
        ; Set the write function
        mov     Write_JMP_Table[14h], offset Write_ERROR

        ret


; --- $6000-$7FFF -----------------------------------------------------------
map_No_WRAM:
        ; Set the write function
        mov     Write_JMP_Table[18h], offset Write_ERROR
        mov     Write_JMP_Table[1ch], offset Write_ERROR

        ret


; --- $8000-$FFFF -----------------------------------------------------------
map_PRG_ROM:
        push    ecx esi

        ; Map the write function
        mov     Write_JMP_Table[20h], offset Write_ERROR
        mov     Write_JMP_Table[24h], offset Write_ERROR
        mov     Write_JMP_Table[28h], offset Write_ERROR
        mov     Write_JMP_Table[2ch], offset Write_ERROR
        mov     Write_JMP_Table[30h], offset Write_ERROR
        mov     Write_JMP_Table[34h], offset Write_ERROR
        mov     Write_JMP_Table[38h], offset Write_ERROR
        mov     Write_JMP_Table[3ch], offset Write_ERROR

        ; Check if we have a small ROM
        mov     edi, RAM
        mov     esi, [edi + NES_H_PRG_ROM]
        cmp     dword ptr [edi + NES_H_N_PRG_ROM], 1
        je      @@SmallROM

        ; Copy the first bank
        mov     ecx, 1000h
        mov     edi, CPU_PRG_ROM
        DUMPD   esi
        DUMPD   edi
        DUMP    " before"
        rep     movsd
        DUMPD   esi
        DUMPD   edi
        DUMP    " after"

        ; Finish to copy the last part
@@SmallROM:
        mov     ecx, 1000h
        mov     edi, CPU_PRG_ROM
        add     edi, 4000h
        DUMPD   esi
        DUMPD   edi
        DUMP    " before"
        rep     movsd
        DUMPD   esi
        DUMPD   edi
        DUMP    " after"

        pop     esi ecx
        ret


; --- PPU: $0000-$1FFF ------------------------------------------------------
map_CHR_RAM:
        push    eax ecx esi edi

        ; Get a pointer on the VRAM
        mov     esi, RAM
        mov     edi, PPU_Pattern_0
        mov     ecx, 1000h
        cmp     dword ptr [esi + NES_H_N_CHR_RAM], 0
        je      @@NoVROM

        ; If CHR-RAM is present, copy it
        mov     esi, [esi + NES_H_CHR_RAM]
        mov     ecx, 800h
        rep     movsd
        mov     ecx, 800h

@@NoVROM:
        pop     edi esi ecx eax
        ret



; In:   None
; Out:  eax =   Status

; --- Memory mapper 00h (Super Mario Bros.) ---
m00:
        push    edi

        ; Map the 6502 memory
        call    map_RAM                 ; Map memory $0000-$1FFF
        call    map_Registers           ; Map memory $2000-$4FFF
        call    map_No_ExpModule        ; Map memory $5000-$5FFF
        call    map_No_WRAM             ; Map memory $6000-$7FFF
        call    map_PRG_ROM             ; Map memory $8000-$FFFF

        ; Map the PPU memory
        call    map_CHR_RAM             ; Map the CHR_RAM

        xor     eax, eax
        pop     edi
        ret


; --- Memory mapper XXh (Unknown memory mapper) ---
mXX:
        mov     eax, NES_ERR_MAPPER
        ret


; ***************************************************************************
; ***** NES registers *******************************************************
; ***************************************************************************

Write_PPU_Register:
        cmp     edx, 2008h
        jge     @@Error
        jmp     dword ptr [PPU_w - 4*2000h + edx * 4]
@@Error:
        DUMPB   al
        DUMP    ": write_PPU_Register"
        DUMPD   edx
        DUMP    " = register"
        ret

Write_IO_Register:
        ret


; ***************************************************************************
; ***** Invalid memory reference ********************************************
; ***************************************************************************

Write_ERROR:
        ;add     esp, 4
        ;mov     ebp, cpu6502_ERR_READ
        ;jmp     cpu6502_Done
        ret


; ***************************************************************************
; ***** NES Registers *******************************************************
; ***************************************************************************

PPU_w:
        dd      offset w2000, offset w2001, offset w20__, offset w2003
        dd      offset w2004, offset w2005a, offset w2006a, offset w2007


; --- PPU write functions ---
wXXXX   macro   regn
w&regn:
        mov     al, byte ptr reg&regn
        ret
endm

wXXXX   %2001
wXXXX   %2003


w20__:
        DUMPB   al
        DUMP    " send"
        DUMPD   edx
        DUMP    " is write protected"

w2000:
        mov     byte ptr reg2000, al

        ; Set the screen pattern table address
        mov     edi, PPU_Pattern_0
        test    al, 10h
        jz      @@SkipScreenPattern
        add     edi, 1000h
@@SkipScreenPattern:
        mov     ScreenPatternTable, edi

        ; Set the sprite pattern table address
        mov     edi, PPU_Pattern_0
        test    al, 08h
        jz      @@SkipSpritePattern
        mov     edi, 1000h
@@SkipSpritePattern:
        mov     SpritePatternTable, edi

        ; Set the PPU address r/w increment
        mov     IncrementVRAM, 01h
        test    al, 04h
        jz      @@SkipIncrement
        mov     IncrementVRAM, 20h
@@SkipIncrement:

        ; Select the Name table and the Attribute Table
        mov     edi, eax
        and     edi, 3
        shl     edi, 10
        add     edi, PPU_Name
        mov     NameTable, edi
        add     edi, 3c0h
        mov     AttributeTable, edi
        ret

w2004:
        mov     edi, reg2003
        add     edi, PPU_SpriteRAM
        mov     [edi], al
        inc     byte ptr reg2003
        ret

w2005a:
        mov     byte ptr reg2005a, al
        mov     dword ptr PPU_w + 4*5, offset w2005b
        ret

w2005b:
        mov     byte ptr reg2005b, al
        mov     dword ptr PPU_w + 4*5, offset w2005a
        ret

w2006a:
        mov     reg2006a, al
        mov     dword ptr PPU_w + 4*6, offset w2006b
        ret

w2006b:
        push    eax
        and     eax, 03h
        mov     reg2006b, al
        mov     dword ptr PPU_w + 4*6, offset w2006a
        pop     eax
        ret

w2007:
        mov     edi, dword ptr reg2006b
        add     edi, PPU_Pattern_0
        mov     [edi], al
        IO_WriteVRAM
        mov     edi, IncrementVRAM
        add     word ptr reg2006b, di
        ret

global  NES_Read2007:   proc
NES_Read2007:
        push    edx

        mov     edx, dword ptr reg2006b
        add     edx, PPU_Pattern_0
        mov     al, [edx]
        mov     edx, IncrementVRAM
        add     word ptr reg2006b, dx

        pop     edx
        ret

IO_w:
        dd      offset w40XX, offset w40XX, offset w40XX, offset w40XX
        dd      offset w40XX, offset w40XX, offset w40XX, offset w40XX
        dd      offset w40XX, offset w40XX, offset w40XX, offset w40XX
        dd      offset w40XX, offset w40XX, offset w40XX, offset w40XX
        dd      offset w40XX, offset w40XX, offset w40XX, offset w40XX
        dd      offset w40XX, offset w40XX, offset w40XX, offset w40XX

w40XX:
        ret


; --- Main Write function ---
global  NES_Write:      proc

NES_Write:
        DUMPD   edx
        DUMP    " = NES_Write"
        DUMPB   al
        DUMP    " = value"
        mov     edi, edx
        shr     edi, 12
        jmp     dword ptr Write_JMP_Table[edi*4]

Write_JMP_Table label   dword
        dd      16 dup (?)

end
