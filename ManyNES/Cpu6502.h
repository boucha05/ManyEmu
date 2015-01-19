#ifndef __CPU_6502_H__
#define __CPU_6502_H__

#include <stdint.h>

struct CPU_STATE
{
    uint8_t     a;
    uint8_t     x;
    uint8_t     y;
    uint8_t     sr;
    uint16_t    sp;
    uint16_t    pc;
    int32_t     clk;
    uint8_t     flag_c;
    uint8_t     flag_z;
    uint8_t     flag_v;
    uint8_t     flag_n;
    uint8_t     imm;
    uint16_t    addr;
    uint8_t*    mem;
};

#endif
