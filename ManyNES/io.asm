; **********
; * IO.ASM *
; **********


; === HEADER ================================================================

.386
.model  flat, C
include nes.inc
include r6502.inc

; === CONSTANTS =============================================================

VID_RowSize     equ     256
VID_Height      equ     224

; === DATA ==================================================================

.data

externdef   APage:  dword
externdef   VPage:  dword

; Local variables
ALine           dd      ?
VLine           dd      ?
APage           dd      ?
VPage           dd      ?
count           dd      ?

VID_BufferPtr   dd      0   ; THIS SHOULD BE PROVIDED BY THE ENGINE

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


; === CODE ==================================================================

.code

; *** NES_DrawTile **********************************************************
;       Draw a NES tile to an output buffer.
;
; In:   esi =   Pointer on the extended bitmap
;       edi =   Pointer on the object to an output buffer
;       edx =   Extended color attribute
;       ecx =   Row width
;
; Out:  none
; ***************************************************************************
rowcount        dd      ?
externdef C NES_DrawTile:  proc
NES_DrawTile    proc
        push    eax
        push    ebx
        push    ecx
        push    esi
        push    edi
        push    ebp

        mov     ebp, ecx

        xor     ecx, ecx
        mov     rowcount, 8

        ; Process one row
@@MainLoop:
        mov     cl, [esi]
        mov     eax, bit0lut[ecx*8]
        mov     ebx, bit0lut[ecx*8 + 4]
        mov     cl, [esi + 8]
        add     eax, bit1lut[ecx*8]
        add     ebx, bit1lut[ecx*8 + 4]
        add     eax, edx
        add     ebx, edx
        mov     [edi], eax
        mov     [edi + 4], ebx

        ; Check if we can draw another row
        inc     esi
        add     edi, ebp
        dec     rowcount
        jnz     @@MainLoop

        pop     ebp
        pop     edi
        pop     esi
        pop     ecx
        pop     ebx
        pop     eax
        ret
NES_DrawTile    endp


; *** BufferToVideo *********************************************************
;       Copy a memory buffer into the video RAM.
;
; In:   ecx =   Source
;       edx =   Destination
;       ebx =   Number of rows
;       ebp =   Row size
;
; Out:  none
; ***************************************************************************
BufferToVideo   proc
        push    eax
        push    ebx
        push    ecx
        push    edx
        push    esi
        push    edi

        ; Prepare the first iteration
        mov     ecx, video_buffer
        mov     edx, APage
        mov     ebx, 240

        ; Copy an entire row of pixel
@@Loop:
        mov     esi, ecx
        mov     edi, edx
        push    ecx
        mov     ecx, 256 / 4
        rep     movsd
        pop     ecx

        ; Update row values
        add     ecx, 32*9
        add     edx, VID_RowSize
        dec     ebx
        jnz     @@Loop

        pop     edi
        pop     esi
        pop     edx
        pop     ecx
        pop     ebx
        pop     eax
        ret
BufferToVideo   endp


; *** SetupPages ************************************************************
;       Initialise the variables used to swap the pages.
;
; Syntax:       void SetupPages(void)
;
; In:           None
;
; Out:          None
; ***************************************************************************
externdef C SetupPages:    proc
SetupPages  proc
        push    eax
        push    ebx

        ; Initialise ALine and VLine
        mov     VLine, 0
        mov     eax, VID_Height
        mov     ALine, eax

        ; Initialise APage and VPage
        mov     ebx, VID_BufferPtr
        mov     VPage, ebx
        imul    eax, VID_RowSize
        add     eax, ebx
        mov     APage, eax

        pop     ebx
        pop     eax
        ret
SetupPages  endp

; *** SwapPages *************************************************************
;       Swap the video pages.
;
; Syntax:       void SwapPages(void)
;
; In:           None
;
; Out:          None
; ***************************************************************************
externdef C SwapPages:     proc
SwapPages   proc
        push    eax
        push    ebx
        push    ecx
        push    edx

        ; Swap the page pointers
        mov     eax, APage
        mov     ebx, VPage
        mov     APage, ebx
        mov     VPage, eax

        ; Swap the first line pointers
        mov     edx, ALine
        mov     eax, VLine
        mov     ALine, eax
        mov     VLine, edx

        ; Swap the display page and the working page
        mov     eax, 4f07h
        mov     ebx, 000h
        xor     ecx, ecx
        int     10h

        pop     edx
        pop     ecx
        pop     ebx
        pop     eax
        ret
SwapPages   endp

end
