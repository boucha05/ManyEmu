/**********
 * 6502.h *
 **********/


#ifndef __6502_H
#define __6502_H


/*
  readb and writeb must use the input address in edx and use or return a
  value in al without modifying ah and the upper part of eax.
*/


#define cpu6502_OKAY            0
#define cpu6502_ERR_BADOPCODE   1


struct cpu6502 {
  void *error;
  int a, x, y, stack, flags, pc;
  void *pc_base;
};

#ifdef __cplusplus
  extern "C" {
#endif

#pragma aux cpu6502_Reset "_*" parm [ebx];
int cpu6502_Reset(struct cpu6502 *CPU);

#pragma aux cpu6502_Execute "_*" parm [ebx];
int cpu6502_Execute(void);

#ifdef __cplusplus
  }
#endif


#endif
