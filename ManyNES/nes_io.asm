; NES_IO.ASM

global C video_buffer:          dword
global C BufferToVideo:         proc
global C SwapPages:             proc
global C Palette:               dword


row_ptr                 dd      ?

bit0lut         label   dword
        I = 0
        REPT    256
        db      (I shr 7) and 1, (I shr 6) and 1, (I shr 5) and 1
        db      (I shr 4) and 1, (I shr 3) and 1, (I shr 2) and 1
        db      (I shr 1) and 1, I and 1
        I = I + 1
        ENDM

bit1lut         label   dword
        I = 0
        REPT    256
        db      (I shr 6) and 2, (I shr 5) and 2, (I shr 4) and 2
        db      (I shr 3) and 2, (I shr 2) and 2, (I shr 1) and 2
        db      I and 2, (I shl 1) and 2
        I = I + 1
        ENDM


; === MACROS ================================================================

IO_ResetCPU     macro
        call    ResetCPU
endm

IO_DrawCurRow   macro
        call    DrawCurRow
endm

IO_UpdateFrame  macro
        call    UpdateFrame
endm

IO_SetPalette   macro
        call    SetPalette
endm

IO_WriteVRAM    macro
        local   @@Done, @@Palette
        sub     edi, PPU_Pattern_0
        cmp     edi, 2000h
        jl      @@Done
        cmp     edi, 3000h
        jge     @@Palette
        jmp     @@Done
@@Palette:
        sub     edi, 3f00h
        DUMPD   edi
        jl      @@Done
        cmp     edi, 20h
        jge     @@Done
        inc     edi
        call    SetPalette
@@Done:
endm


; === FUNCTIONS =============================================================

; *** ResetCPU **************************************************************

ResetCPU:
        mov     eax, video_buffer
        mov     row_ptr, eax
        mov     row_number, 0
        ret



; *** DrawCurRow ************************************************************

row_number      dd      0
;row_ptr         dd      0
;and_mask        dd      0
;shift_mask      dd      0
tile_count      dd      0
pattern_pos     dd      0
DrawCurRow:
        push    eax ebx ecx edx esi edi ebp

        ; Get the position in the Name Table and in the attribute table
        xor     edx, edx
        mov     dl, byte ptr 0          ; dl = vertical scroll
        add     dl, byte ptr row_number ; edx += row_number
        mov     eax, edx
        shl     eax, 5
        mov     esi, dword ptr 0
        and     esi, 0ffh
        add     esi, edx
        add     esi, PPU_Name
        mov     edi, row_ptr

        ; Get the first attribute then start to draw
        and     edx, 07h
        add     edx, ScreenPatternTable
        mov     pattern_pos, edx
        xor     ecx, ecx
        mov     eax, 01010101h
        mov     tile_count, 32
        ; if we start to draw in the middle of a 32x32 tile, skip A

@@GetAttrib:                    ; First get the attribute
        ;add     eax, 01010101h
        xor     ecx, ecx

@@LeftNameTableA:               ; Draw the first part of a 32x32 tile
        xor     edx, edx
        mov     dl, [esi]
        shl     edx, 4
        add     edx, pattern_pos
        mov     cl, [edx]
        mov     ebp, bit0lut[ecx*8]
        mov     ebx, bit0lut[ecx*8 + 4]
        mov     cl, [edx + 8]
        add     ebp, bit1lut[ecx*8]
        add     ebx, bit1lut[ecx*8 + 4]
        add     ebp, eax
        add     ebx, eax
        mov     [edi], ebp
        mov     [edi + 4], ebx

        inc     esi             ; Update the position
        add     edi, 8
        dec     tile_count
        jz      @@Done

@@LeftNameTableB:               ; Draw the second part of a 32x32 tile
        xor     edx, edx
        mov     dl, [esi]
        shl     edx, 4
        add     edx, pattern_pos
        mov     cl, [edx]
        mov     ebp, bit0lut[ecx*8]
        mov     ebx, bit0lut[ecx*8 + 4]
        mov     cl, [edx + 8]
        add     ebp, bit1lut[ecx*8]
        add     ebx, bit1lut[ecx*8 + 4]
        add     ebp, eax
        add     ebx, eax
        mov     [edi], ebp
        mov     [edi + 4], ebx

        inc     esi             ; Update the position
        add     edi, 8
        dec     tile_count
        jnz     @@GetAttrib

@@Done:
        inc     row_number
        add     row_ptr, 32*9

        pop     ebp edi esi edx ecx ebx eax
        ret


; *** UpdateFrame ***********************************************************

UpdateFrame:
        push    eax

        ; Copy the buffer to the RAM
        call    BufferToVideo

        ; Reset the variables
        mov     eax, video_buffer
        mov     row_ptr, eax
        mov     row_number, 0

        ; Swap the video pages
        call    SwapPages

        pop     eax
        ret


; *** SetPalette ************************************************************
; in:   al = index in the lookup table, edi = palette entry
SetPalette:
        DUMP    "PALETTE"
        push    eax

        ; Get a pointer on the RGB entry of the palette
        and     eax, 0ffh
        add     eax, [eax * 2]
        xchg    eax, edi

        ; Write the 3 bytes to the DAC register
        mov     edx, 3c8h
        inc     al
        out     dx, al
        inc     edx
        mov     al, [edi]
        out     dx, al
        mov     al, [edi + 1]
        out     dx, al
        mov     al, [edi + 2]
        out     dx, al

        pop     edx eax
        ret

