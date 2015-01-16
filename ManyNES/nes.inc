;***********
;* nes.inc *
;***********


; === STRUCTURES ============================================================

; The NES console structure
conNESs         struct
        conNES_CPU      dd      ?       ; Pointer on the CPU structure
        conNES_ROM      dd      ?       ; Pointer on the ROM buffer
        conNES_RAM      dd      ?       ; Pointer on the RAM buffer
conNESs         ends


; === CONSTANTS =============================================================


; --- CPU variables ---
NES_CPU_RAM             equ     00000h  ; RAM ($0000-$0800 mirrored 4 times)
NES_CPU_REGS            equ     02000h  ; Registers
NES_CPU_EXP             equ     05000h  ; Expansion Modules
NES_CPU_WRAM            equ     06000h  ; WRAM
NES_CPU_TRAINER         equ     07000h  ; WRAM or RAM (Trainer)
NES_CPU_PRG_ROM         equ     08000h  ; PRG-ROM (Lower)

; --- PPU variables ---
NES_PPU_PATTERN_0       equ     10000h  ; Pattern Table #0
NES_PPU_PATTERN_1       equ     11000h  ; Pattern Table #1
NES_PPU_NAME            equ     12000h  ; Name Table #0
NES_PPU_ATTRIBUTE       equ     123C0h  ; Attribute Table #0
NES_PPU_IMAGEPAL        equ     13F00h  ; Image Palette
NES_PPU_SPRITEPAL       equ     13F10h  ; Sprite Palette
NES_PPU_SPRITERAM       equ     14000h  ; Sprite RAM

; --- ROM header variables ---
NES_H_N_PRG_ROM         equ     14100h  ; Number of 16K PRG-ROM banks
NES_H_N_CHR_RAM         equ     14104h  ; Number of 8K CHR-RAM banks
NES_H_PRG_ROM           equ     14108h  ; Pointer on the first PRG-ROM bank
NES_H_CHR_RAM           equ     1410Ch  ; Pointer on the first CHR-RAM bank
NES_H_MAPPER            equ     14110h  ; Number of the memory mapper
NES_H_ATTRIB            equ     14114h  ; Cartridge's attributes


; --- ROM specific variables that need to be on a savegame ---
NES_MISC                equ     14118h  ; Miscellaneous datas

NES_RAM_SIZE    equ     18000h          ; THE RAM SIZE


; --- Error codes ---
NES_ERR_NONE            equ     0       ; Okay
NES_ERR_RESET           equ     1       ; Fail to reset
NES_ERR_READ            equ     2       ; Fail to read byte
NES_ERR_WRITE           equ     3       ; Fail to write byte
NES_ERR_NOT_NES         equ     4       ; Not a NES rom
NES_ERR_MAPPER          equ     5       ; Unknown memory mapper
NES_ERR_EXECUTE         equ     6       ; Execute error



; === DATAS =================================================================

externdef   conNES_NCycles:     dword
externdef   PPU_Name:           dword
externdef   ScreenPatternTable: dword

externdef   video_buffer:       dword
externdef   BufferToVideo:      proc
externdef   SwapPages:          proc
externdef   Palette:            dword


; === CODE ==================================================================

externdef   conNES_Reset:   proc
externdef   conNES_Execute: proc
externdef   ResetCPU:       proc
externdef   DrawCurRow:     proc
externdef   UpdateFrame:    proc
