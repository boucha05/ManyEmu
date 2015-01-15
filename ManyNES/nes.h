/*********
 * nes.h *
 *********/


#ifndef __NES_H
#define __NES_H


#include "6502.h"


struct conNES {
  struct cpu6502 *CPU;
  char *ROM, *RAM;
};

#ifdef __cplusplus
  extern "C" {
#endif

extern int conNES_NCycles;

#pragma aux conNES_Reset "_*" parm [eax];
int conNES_Reset(struct conNES *NES);

#pragma aux conNES_Setup "_*" parm [ebx];
void conNES_Setup(struct conNES *NES);

#pragma aux conNES_Backup "_*" parm [ebx];
void conNES_Backup(struct conNES *NES);

#pragma aux conNES_Execute "_*" parm [ebx];
int conNES_Execute(void);

#ifdef __cplusplus
  }
#endif


/*** CONSTANTS *************************************************************/

/* --- CPU variables --- */
#define NES_CPU_RAM             0x00000 // RAM ($0000-$0800 mirrored 4 times)
#define NES_CPU_REGS            0x02000 // Registers
#define NES_CPU_EXP             0x05000 // Expansion Modules
#define NES_CPU_WRAM            0x06000 // WRAM
#define NES_CPU_TRAINER         0x07000 // WRAM or RAM (Trainer)
#define NES_CPU_PRG_ROM         0x08000 // PRG-ROM (Lower)

/* --- PPU variables --- */
#define NES_PPU_PATTERN_0       0x10000 // Pattern Table #0
#define NES_PPU_PATTERN_1       0x11000 // Pattern Table #1
#define NES_PPU_NAME            0x12000 // Name Table #0
#define NES_PPU_ATTRIBUTE       0x123C0 // Attribute Table #0
#define NES_PPU_IMAGEPAL        0x13F00 // Image Palette
#define NES_PPU_SPRITEPAL       0x13F10 // Sprite Palette
#define NES_PPU_SPRITERAM       0x14000 // Sprite RAM

/* --- ROM header variables --- */
#define NES_H_N_PRG_ROM         0x14100 // Number of 16K PRG-ROM banks
#define NES_H_N_CHR_RAM         0x14104 // Number of 8K CHR-RAM banks
#define NES_H_PRG_ROM           0x14108 // Pointer on the first PRG-ROM bank
#define NES_H_CHR_RAM           0x1410C // Pointer on the first CHR-RAM bank
#define NES_H_MAPPER            0x14110 // Number of the memory mappers
#define NES_H_ATTRIB            0x14114 // Cartridge's attributes

/* --- ROM specific variables that need to be on a savegame --- */
#define NES_MISC                0x14118 // Miscellaneous datas

#define NES_RAM_SIZE            0x18000 // THE RAM SIZE


/* --- Error codes --- */
#define NES_ERR_NONE            0x00    // Okay
#define NES_ERR_RESET           0x01    // Fail to reset
#define NES_ERR_READ            0x02    // Fail to read byte
#define NES_ERR_WRITE           0x03    // Fail to write byte
#define NES_ERR_NOT_NES         0x04    // Not a NES rom
#define NES_ERR_MAPPER          0x05    // Unknown memory mapper
#define NES_ERR_EXECUTE         0x06    // Execute error


#endif
