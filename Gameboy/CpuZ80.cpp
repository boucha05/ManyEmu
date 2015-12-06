#include <Core/MemoryBus.h>
#include <Core/Serialization.h>
#include "CpuZ80.h"
#include "GB.h"
#include <memory.h>
#include <stdio.h>

#define A           mRegs.r8.a
#define FLAGS       mRegs.r8.flags
#define B           mRegs.r8.b
#define C           mRegs.r8.c
#define D           mRegs.r8.d
#define E           mRegs.r8.e
#define H           mRegs.r8.h
#define L           mRegs.r8.l
#define FLAG_Z      mRegs.r8.flag_z
#define FLAG_N      mRegs.r8.flag_n
#define FLAG_H      mRegs.r8.flag_h
#define FLAG_C      mRegs.r8.flag_c

#define AF          mRegs.r16.af
#define BC          mRegs.r16.bc
#define DE          mRegs.r16.de
#define HL          mRegs.r16.hl
#define SP          mRegs.r16.sp
#define PC          mRegs.r16.pc

namespace
{
    /***************************************************************************
    
    Gameboy CPU (LR35902) instruction set 

                  x0           x1           x2           x3           x4           x5           x6           x7           x8           x9           xA           xB           xC           xD           xE           xF 
     0x           NOP       LD BC,d16    LD (BC),A     INC BC        INC B        DEC B       LD B,d8       RLCA      LD (a16),SP   ADD HL,BC    LD A,(BC)     DEC BC        INC C        DEC C       LD C,d8       RRCA
                 1  4         3  12        1  8         1  8         1  4         1  4         2  8         1  4         3  20        1  8         1  8         1  8         1  4         1  4         2  8         1  4
                - - - -      - - - -      - - - -      - - - -      Z 0 H -      Z 1 H -      - - - -      0 0 0 C      - - - -      - 0 H C      - - - -      - - - -      Z 0 H -      Z 1 H -      - - - -      0 0 0 C
     1x         STOP 0      LD DE,d16    LD (DE),A     INC DE        INC D        DEC D       LD D,d8        RLA         JR r8      ADD HL,DE    LD A,(DE)     DEC DE        INC E        DEC E       LD E,d8        RRA
                 2  4         3  12        1  8         1  8         1  4         1  4         2  8         1  4         2  12        1  8         1  8         1  8         1  4         1  4         2  8         1  4
                - - - -      - - - -      - - - -      - - - -      Z 0 H -      Z 1 H -      - - - -      0 0 0 C      - - - -      - 0 H C      - - - -      - - - -      Z 0 H -      Z 1 H -      - - - -      0 0 0 C
     2x        JR NZ,r8     LD HL,d16   LD (HL+),A     INC HL        INC H        DEC H       LD H,d8        DAA        JR Z,r8     ADD HL,HL   LD A,(HL+)     DEC HL        INC L        DEC L       LD L,d8        CPL
                2  12/8       3  12        1  8         1  8         1  4         1  4         2  8         1  4        2  12/8       1  8         1  8         1  8         1  4         1  4         2  8         1  4
                - - - -      - - - -      - - - -      - - - -      Z 0 H -      Z 1 H -      - - - -      Z - 0 C      - - - -      - 0 H C      - - - -      - - - -      Z 0 H -      Z 1 H -      - - - -      - 1 1 -
     3x        JR NC,r8     LD SP,d16   LD (HL-),A     INC SP      INC (HL)     DEC (HL)    LD (HL),d8       SCF        JR C,r8     ADD HL,SP   LD A,(HL-)     DEC SP        INC A        DEC A       LD A,d8        CCF
                2  12/8       3  12        1  8         1  8         1  12        1  12        2  12        1  4        2  12/8       1  8         1  8         1  8         1  4         1  4         2  8         1  4
                - - - -      - - - -      - - - -      - - - -      Z 0 H -      Z 1 H -      - - - -      - 0 0 1      - - - -      - 0 H C      - - - -      - - - -      Z 0 H -      Z 1 H -      - - - -      - 0 0 C
     4x         LD B,B       LD B,C       LD B,D       LD B,E       LD B,H       LD B,L      LD B,(HL)     LD B,A       LD C,B       LD C,C       LD C,D       LD C,E       LD C,H       LD C,L      LD C,(HL)     LD C,A
                 1  4         1  4         1  4         1  4         1  4         1  4         1  8         1  4         1  4         1  4         1  4         1  4         1  4         1  4         1  8         1  4
                - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -
     5x         LD D,B       LD D,C       LD D,D       LD D,E       LD D,H       LD D,L      LD D,(HL)     LD D,A       LD E,B       LD E,C       LD E,D       LD E,E       LD E,H       LD E,L      LD E,(HL)     LD E,A
                 1  4         1  4         1  4         1  4         1  4         1  4         1  8         1  4         1  4         1  4         1  4         1  4         1  4         1  4         1  8         1  4
                - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -
     6x         LD H,B       LD H,C       LD H,D       LD H,E       LD H,H       LD H,L      LD H,(HL)     LD H,A       LD L,B       LD L,C       LD L,D       LD L,E       LD L,H       LD L,L      LD L,(HL)     LD L,A
                 1  4         1  4         1  4         1  4         1  4         1  4         1  8         1  4         1  4         1  4         1  4         1  4         1  4         1  4         1  8         1  4
                - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -
     7x        LD (HL),B    LD (HL),C    LD (HL),D    LD (HL),E    LD (HL),H    LD (HL),L      HALT       LD (HL),A     LD A,B       LD A,C       LD A,D       LD A,E       LD A,H       LD A,L      LD A,(HL)     LD A,A
                 1  8         1  8         1  8         1  8         1  8         1  8         1  4         1  8         1  4         1  4         1  4         1  4         1  4         1  4         1  8         1  4
                - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -
     8x         ADD A,B      ADD A,C      ADD A,D      ADD A,E      ADD A,H      ADD A,L    ADD A,(HL)     ADD A,A      ADC A,B      ADC A,C      ADC A,D      ADC A,E      ADC A,H      ADC A,L    ADC A,(HL)     ADC A,A
                 1  4         1  4         1  4         1  4         1  4         1  4         1  8         1  4         1  4         1  4         1  4         1  4         1  4         1  4         1  8         1  4
                Z 0 H C      Z 0 H C      Z 0 H C      Z 0 H C      Z 0 H C      Z 0 H C      Z 0 H C      Z 0 H C      Z 0 H C      Z 0 H C      Z 0 H C      Z 0 H C      Z 0 H C      Z 0 H C      Z 0 H C      Z 0 H C
     9x          SUB B        SUB C        SUB D        SUB E        SUB H        SUB L      SUB (HL)       SUB A       SBC A,B      SBC A,C      SBC A,D      SBC A,E      SBC A,H      SBC A,L    SBC A,(HL)     SBC A,A
                 1  4         1  4         1  4         1  4         1  4         1  4         1  8         1  4         1  4         1  4         1  4         1  4         1  4         1  4         1  8         1  4
                Z 1 H C      Z 1 H C      Z 1 H C      Z 1 H C      Z 1 H C      Z 1 H C      Z 1 H C      Z 1 H C      Z 1 H C      Z 1 H C      Z 1 H C      Z 1 H C      Z 1 H C      Z 1 H C      Z 1 H C      Z 1 H C
     Ax          AND B        AND C        AND D        AND E        AND H        AND L      AND (HL)       AND A        XOR B        XOR C        XOR D        XOR E        XOR H        XOR L      XOR (HL)       XOR A
                 1  4         1  4         1  4         1  4         1  4         1  4         1  8         1  4         1  4         1  4         1  4         1  4         1  4         1  4         1  8         1  4
                Z 0 1 0      Z 0 1 0      Z 0 1 0      Z 0 1 0      Z 0 1 0      Z 0 1 0      Z 0 1 0      Z 0 1 0      Z 0 0 0      Z 0 0 0      Z 0 0 0      Z 0 0 0      Z 0 0 0      Z 0 0 0      Z 0 0 0      Z 0 0 0
     Bx          OR B         OR C         OR D         OR E         OR H         OR L        OR (HL)       OR A         CP B         CP C         CP D         CP E         CP H         CP L        CP (HL)       CP A
                 1  4         1  4         1  4         1  4         1  4         1  4         1  8         1  4         1  4         1  4         1  4         1  4         1  4         1  4         1  8         1  4
                Z 0 0 0      Z 0 0 0      Z 0 0 0      Z 0 0 0      Z 0 0 0      Z 0 0 0      Z 0 0 0      Z 0 0 0      Z 1 H C      Z 1 H C      Z 1 H C      Z 1 H C      Z 1 H C      Z 1 H C      Z 1 H C      Z 1 H C
     Cx         RET NZ       POP BC      JP NZ,a16     JP a16     CALL NZ,a16    PUSH BC     ADD A,d8      RST 00H       RET Z         RET       JP Z,a16     PREFIX CB   CALL Z,a16    CALL a16     ADC A,d8      RST 08H
                1  20/8       1  12      3  16/12       3  16      3  24/12       1  16        2  8         1  16       1  20/8       1  16      3  16/12       1  4       3  24/12       3  24        2  8         1  16
                - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      Z 0 H C      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      Z 0 H C      - - - -
     Dx         RET NC       POP DE      JP NC,a16                CALL NC,a16    PUSH DE      SUB d8       RST 10H       RET C        RETI       JP C,a16                 CALL C,a16                 SBC A,d8      RST 18H
                1  20/8       1  12      3  16/12                  3  24/12       1  16        2  8         1  16       1  20/8       1  16      3  16/12                  3  24/12                    2  8         1  16
                - - - -      - - - -      - - - -                   - - - -      - - - -      Z 1 H C      - - - -      - - - -      - - - -      - - - -                   - - - -                   Z 1 H C      - - - -
     Ex       LDH (a8),A     POP HL      LD (C),A                                PUSH HL      AND d8       RST 20H     ADD SP,r8     JP (HL)    LD (a16),A                                            XOR d8       RST 28H
                 2  12        1  12        2  8                                   1  16        2  8         1  16        2  16        1  4         3  16                                               2  8         1  16
                - - - -      - - - -      - - - -                                - - - -      Z 0 1 0      - - - -      0 0 H C      - - - -      - - - -                                             Z 0 0 0      - - - -
     Fx       LDH A,(a8)     POP AF      LD A,(C)        DI                      PUSH AF       OR d8       RST 30H    LD HL,SP+r8   LD SP,HL    LD A,(a16)       EI                                    CP d8       RST 38H
                 2  12        1  12        2  8         1  4                      1  16        2  8         1  16        2  12        1  8         3  16        1  4                                   2  8         1  16
                - - - -      Z N H C      - - - -      - - - -                   - - - -      Z 0 0 0      - - - -      0 0 H C      - - - -      - - - -      - - - -                                Z 1 H C      - - - -


Prefix CB 

                  x0           x1           x2           x3           x4           x5           x6           x7           x8           x9           xA           xB           xC           xD           xE           xF 
     0x          RLC B        RLC C        RLC D        RLC E        RLC H        RLC L      RLC (HL)       RLC A        RRC B        RRC C        RRC D        RRC E        RRC H        RRC L      RRC (HL)       RRC A
                 2  8         2  8         2  8         2  8         2  8         2  8         2  16        2  8         2  8         2  8         2  8         2  8         2  8         2  8         2  16        2  8
                Z 0 0 C      Z 0 0 C      Z 0 0 C      Z 0 0 C      Z 0 0 C      Z 0 0 C      Z 0 0 C      Z 0 0 C      Z 0 0 C      Z 0 0 C      Z 0 0 C      Z 0 0 C      Z 0 0 C      Z 0 0 C      Z 0 0 C      Z 0 0 C
     1x          RL B         RL C         RL D         RL E         RL H         RL L        RL (HL)       RL A         RR B         RR C         RR D         RR E         RR H         RR L        RR (HL)       RR A
                 2  8         2  8         2  8         2  8         2  8         2  8         2  16        2  8         2  8         2  8         2  8         2  8         2  8         2  8         2  16        2  8
                Z 0 0 C      Z 0 0 C      Z 0 0 C      Z 0 0 C      Z 0 0 C      Z 0 0 C      Z 0 0 C      Z 0 0 C      Z 0 0 C      Z 0 0 C      Z 0 0 C      Z 0 0 C      Z 0 0 C      Z 0 0 C      Z 0 0 C      Z 0 0 C
     2x          SLA B        SLA C        SLA D        SLA E        SLA H        SLA L      SLA (HL)       SLA A        SRA B        SRA C        SRA D        SRA E        SRA H        SRA L      SRA (HL)       SRA A
                 2  8         2  8         2  8         2  8         2  8         2  8         2  16        2  8         2  8         2  8         2  8         2  8         2  8         2  8         2  16        2  8
                Z 0 0 C      Z 0 0 C      Z 0 0 C      Z 0 0 C      Z 0 0 C      Z 0 0 C      Z 0 0 C      Z 0 0 C      Z 0 0 0      Z 0 0 0      Z 0 0 0      Z 0 0 0      Z 0 0 0      Z 0 0 0      Z 0 0 0      Z 0 0 0
     3x         SWAP B       SWAP C       SWAP D       SWAP E       SWAP H       SWAP L      SWAP (HL)     SWAP A        SRL B        SRL C        SRL D        SRL E        SRL H        SRL L      SRL (HL)       SRL A
                 2  8         2  8         2  8         2  8         2  8         2  8         2  16        2  8         2  8         2  8         2  8         2  8         2  8         2  8         2  16        2  8
                Z 0 0 0      Z 0 0 0      Z 0 0 0      Z 0 0 0      Z 0 0 0      Z 0 0 0      Z 0 0 0      Z 0 0 0      Z 0 0 C      Z 0 0 C      Z 0 0 C      Z 0 0 C      Z 0 0 C      Z 0 0 C      Z 0 0 C      Z 0 0 C
     4x         BIT 0,B      BIT 0,C      BIT 0,D      BIT 0,E      BIT 0,H      BIT 0,L    BIT 0,(HL)     BIT 0,A      BIT 1,B      BIT 1,C      BIT 1,D      BIT 1,E      BIT 1,H      BIT 1,L    BIT 1,(HL)     BIT 1,A
                 2  8         2  8         2  8         2  8         2  8         2  8         2  16        2  8         2  8         2  8         2  8         2  8         2  8         2  8         2  16        2  8
                Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -
     5x         BIT 2,B      BIT 2,C      BIT 2,D      BIT 2,E      BIT 2,H      BIT 2,L    BIT 2,(HL)     BIT 2,A      BIT 3,B      BIT 3,C      BIT 3,D      BIT 3,E      BIT 3,H      BIT 3,L    BIT 3,(HL)     BIT 3,A
                 2  8         2  8         2  8         2  8         2  8         2  8         2  16        2  8         2  8         2  8         2  8         2  8         2  8         2  8         2  16        2  8
                Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -
     6x         BIT 4,B      BIT 4,C      BIT 4,D      BIT 4,E      BIT 4,H      BIT 4,L    BIT 4,(HL)     BIT 4,A      BIT 5,B      BIT 5,C      BIT 5,D      BIT 5,E      BIT 5,H      BIT 5,L    BIT 5,(HL)     BIT 5,A
                 2  8         2  8         2  8         2  8         2  8         2  8         2  16        2  8         2  8         2  8         2  8         2  8         2  8         2  8         2  16        2  8
                Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -
     7x         BIT 6,B      BIT 6,C      BIT 6,D      BIT 6,E      BIT 6,H      BIT 6,L    BIT 6,(HL)     BIT 6,A      BIT 7,B      BIT 7,C      BIT 7,D      BIT 7,E      BIT 7,H      BIT 7,L    BIT 7,(HL)     BIT 7,A
                 2  8         2  8         2  8         2  8         2  8         2  8         2  16        2  8         2  8         2  8         2  8         2  8         2  8         2  8         2  16        2  8
                Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -      Z 0 1 -
     8x         RES 0,B      RES 0,C      RES 0,D      RES 0,E      RES 0,H      RES 0,L    RES 0,(HL)     RES 0,A      RES 1,B      RES 1,C      RES 1,D      RES 1,E      RES 1,H      RES 1,L    RES 1,(HL)     RES 1,A
                 2  8         2  8         2  8         2  8         2  8         2  8         2  16        2  8         2  8         2  8         2  8         2  8         2  8         2  8         2  16        2  8
                - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -
     9x         RES 2,B      RES 2,C      RES 2,D      RES 2,E      RES 2,H      RES 2,L    RES 2,(HL)     RES 2,A      RES 3,B      RES 3,C      RES 3,D      RES 3,E      RES 3,H      RES 3,L    RES 3,(HL)     RES 3,A
                 2  8         2  8         2  8         2  8         2  8         2  8         2  16        2  8         2  8         2  8         2  8         2  8         2  8         2  8         2  16        2  8
                - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -
     Ax         RES 4,B      RES 4,C      RES 4,D      RES 4,E      RES 4,H      RES 4,L    RES 4,(HL)     RES 4,A      RES 5,B      RES 5,C      RES 5,D      RES 5,E      RES 5,H      RES 5,L    RES 5,(HL)     RES 5,A
                 2  8         2  8         2  8         2  8         2  8         2  8         2  16        2  8         2  8         2  8         2  8         2  8         2  8         2  8         2  16        2  8
                - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -
     Bx         RES 6,B      RES 6,C      RES 6,D      RES 6,E      RES 6,H      RES 6,L    RES 6,(HL)     RES 6,A      RES 7,B      RES 7,C      RES 7,D      RES 7,E      RES 7,H      RES 7,L    RES 7,(HL)     RES 7,A
                 2  8         2  8         2  8         2  8         2  8         2  8         2  16        2  8         2  8         2  8         2  8         2  8         2  8         2  8         2  16        2  8
                - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -
     Cx         SET 0,B      SET 0,C      SET 0,D      SET 0,E      SET 0,H      SET 0,L    SET 0,(HL)     SET 0,A      SET 1,B      SET 1,C      SET 1,D      SET 1,E      SET 1,H      SET 1,L    SET 1,(HL)     SET 1,A
                 2  8         2  8         2  8         2  8         2  8         2  8         2  16        2  8         2  8         2  8         2  8         2  8         2  8         2  8         2  16        2  8
                - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -
     Dx         SET 2,B      SET 2,C      SET 2,D      SET 2,E      SET 2,H      SET 2,L    SET 2,(HL)     SET 2,A      SET 3,B      SET 3,C      SET 3,D      SET 3,E      SET 3,H      SET 3,L    SET 3,(HL)     SET 3,A
                 2  8         2  8         2  8         2  8         2  8         2  8         2  16        2  8         2  8         2  8         2  8         2  8         2  8         2  8         2  16        2  8
                - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -
     Ex         SET 4,B      SET 4,C      SET 4,D      SET 4,E      SET 4,H      SET 4,L    SET 4,(HL)     SET 4,A      SET 5,B      SET 5,C      SET 5,D      SET 5,E      SET 5,H      SET 5,L    SET 5,(HL)     SET 5,A
                 2  8         2  8         2  8         2  8         2  8         2  8         2  16        2  8         2  8         2  8         2  8         2  8         2  8         2  8         2  16        2  8
                - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -
     Fx         SET 6,B      SET 6,C      SET 6,D      SET 6,E      SET 6,H      SET 6,L    SET 6,(HL)     SET 6,A      SET 7,B      SET 7,C      SET 7,D      SET 7,E      SET 7,H      SET 7,L    SET 7,(HL)     SET 7,A
                 2  8         2  8         2  8         2  8         2  8         2  8         2  16        2  8         2  8         2  8         2  8         2  8         2  8         2  8         2  16        2  8
                - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -      - - - -


                          Misc/control instructions                              INS reg                ? Instruction mnemonic    Duration of conditional calls and returns is different when action is taken or not. This is
                          Jumps/calls               Length in bytes ?             2  8                  ? Duration in cycles
                          8bit load/store/move instructions                      Z N H C                ? Flags affected
                          16bit load/store/move instructions
                          8bit arithmetic/logical instructions
                          16bit arithmetic/logical instructions
                          8bit rotations/shifts and bit instructions

Instruction STOP has according to manuals opcode 10 00 and thus is 2 bytes long. Anyhow it seems there is no reason for it so some assemblers code it simply as one byte instruction 10.
Flags affected are always shown in Z H N C order. If flag is marked by "0" it means it is reset after the instruction. If it is marked by "1" it is set. If it is marked by "-" it is not changed. If it is marked by "Z", "N

d8  means immediate 8 bit data
d16 means immediate 16 bit data
a8  means 8 bit unsigned data, which are added to $FF00 in certain instructions (replacement for missing IN and OUT instructions)
a16 means 16 bit address
r8  means 8 bit signed data, which are added to program counter

LD A,(C) has alternative mnemonic LD A,($FF00+C)
LD C,(A) has alternative mnemonic LD ($FF00+C),A
LDH A,(a8) has alternative mnemonic LD A,($FF00+a8)
LDH (a8),A has alternative mnemonic LD ($FF00+a8),A
LD A,(HL+) has alternative mnemonic LD A,(HLI) or LDI A,(HL)
LD (HL+),A has alternative mnemonic LD (HLI),A or LDI (HL),A
LD A,(HL-) has alternative mnemonic LD A,(HLD) or LDD A,(HL)
LD (HL-),A has alternative mnemonic LD (HLD),A or LDD (HL),A
LD HL,SP+r8 has alternative mnemonic LDHL SP,r8


Registers 

15  . . .  8  7  . . .  0              Flag register (F) bits:
A (accumulato  F (flags)
      B            C                         7            6            5            4            3            2            1            0
      D            E                         Z            N            H            C            0            0            0            0
      H            L
                                       Z - Zero Flag
15  . . .  0                           N - Subtract Flag
SP (stack pointer)                     H - Half Carry Flag
PC (program counter)                   C - Carry Flag
                                       0 - Not used, always zero

    ***************************************************************************/
}

namespace gb
{
    uint8_t CpuZ80::read8(uint16_t addr)
    {
        return memory_bus_read8(*mMemory, mExecutedTicks, addr);
    }

    void CpuZ80::write8(uint16_t addr, uint8_t value)
    {
        memory_bus_write8(*mMemory, mExecutedTicks, addr, value);
    }

    uint16_t CpuZ80::read16(uint16_t addr)
    {
        uint8_t lo = read8(addr);
        uint8_t hi = read8(addr + 1);
        uint16_t value = static_cast<uint16_t>(lo) | (static_cast<uint16_t>(hi) << 8);
        return value;
    }

    uint8_t CpuZ80::fetch8()
    {
        return read8(PC++);
    }

    uint16_t CpuZ80::fetch16()
    {
        uint8_t lo = fetch8();
        uint8_t hi = fetch8();
        uint16_t addr = static_cast<uint16_t>(lo) | (static_cast<uint16_t>(hi) << 8);
        return addr;
    }

    uint16_t CpuZ80::fetchPC()
    {
        int8_t offset = fetch8();
        uint16_t addr = PC + static_cast<int16_t>(offset);
        return addr;
    }

    ///////////////////////////////////////////////////////////////////////////

    uint8_t CpuZ80::peek8(uint16_t& addr)
    {
        return read8(addr++);
    }

    uint16_t CpuZ80::peek16(uint16_t& addr)
    {
        uint8_t lo = peek8(addr);
        uint8_t hi = peek8(addr);
        uint16_t value = static_cast<uint16_t>(lo) | (static_cast<uint16_t>(hi) << 8);
        return value;
    }

    ///////////////////////////////////////////////////////////////////////////

    void CpuZ80::push8(uint8_t value)
    {
        uint16_t addr = SP-- + 0x100;
        write8(addr, value);
    }
    
    uint8_t CpuZ80::pop8()
    {
        uint16_t addr = ++SP + 0x100;
        uint8_t value = read8(addr);
        return value;
    }

    void CpuZ80::push16(uint16_t value)
    {
        uint8_t lo = value & 0xff;
        uint8_t hi = (value >> 8) & 0xff;
        push8(hi);
        push8(lo);
    }

    uint16_t CpuZ80::pop16()
    {
        uint8_t lo = pop8();
        uint8_t hi = pop8();
        uint16_t value = (static_cast<uint16_t>(lo) | (static_cast<uint16_t>(hi) << 8));
        return value;
    }

    ///////////////////////////////////////////////////////////////////////////

    CpuZ80::CpuZ80()
        : mClock(nullptr)
    {
    }

    CpuZ80::~CpuZ80()
    {
        destroy();
    }

    bool CpuZ80::create(emu::Clock& clock, MEMORY_BUS& bus, uint32_t master_clock_divider)
    {
        mClock = &clock;
        mClock->addListener(*this);
        return true;
    }

    void CpuZ80::destroy()
    {
        if (mClock)
        {
            mClock->removeListener(*this);
            mClock = nullptr;
        }
    }

    void CpuZ80::reset()
    {
        AF = 0x01b0;
        BC = 0x0013;
        DE = 0x00d8;
        HL = 0x014d;
        SP = 0xfffe;
        PC = 0x0100;
        mExecutedTicks = 0;
        mDesiredTicks = 0;
    }

    void CpuZ80::advanceClock(int32_t ticks)
    {
        mExecutedTicks -= ticks;
        mDesiredTicks = 0;
    }

    void CpuZ80::setDesiredTicks(int32_t ticks)
    {
        mDesiredTicks = ticks;
    }

#define NOT_IMPLEMENTED()                                       \
    printf("Instruction %s not implemented\n", __FUNCTION__);   \
    EMU_ASSERT(false)

    struct addr
    {
        explicit addr(uint16_t _value)
            : value(_value)
        {
        }

        uint16_t    value;
    };

    // GMB 8bit-Loadcommands

    void insnLD(uint8_t& dest, uint8_t src)
    {
        NOT_IMPLEMENTED();
    }

    void insnLD(addr& dest, uint16_t src)
    {
        NOT_IMPLEMENTED();
    }

    // GMB 16bit - Loadcommands

    void insnLD(uint16_t& dest, uint16_t src)
    {
        NOT_IMPLEMENTED();
    }

    void insnPUSH(uint16_t src)
    {
        NOT_IMPLEMENTED();
    }

    void insnPOP(uint16_t& dest)
    {
        NOT_IMPLEMENTED();
    }

    // GMB 8bit - Arithmetic / logical Commands

    void insnADD(uint8_t& dest, uint8_t src)
    {
        NOT_IMPLEMENTED();
    }

    void insnADD(addr& dest, uint8_t src)
    {
        NOT_IMPLEMENTED();
    }

    void insnADC(uint8_t& dest, uint8_t src)
    {
        NOT_IMPLEMENTED();
    }

    void insnSUB(uint8_t src)
    {
        NOT_IMPLEMENTED();
    }

    void insnSBC(uint8_t& dest, uint8_t src)
    {
        NOT_IMPLEMENTED();
    }

    void insnAND(uint8_t src)
    {
        NOT_IMPLEMENTED();
    }

    void insnXOR(uint8_t src)
    {
        NOT_IMPLEMENTED();
    }

    void insnOR(uint8_t src)
    {
        NOT_IMPLEMENTED();
    }

    void insnCP(uint8_t src)
    {
        NOT_IMPLEMENTED();
    }

    void insnINC(uint8_t& dest)
    {
        NOT_IMPLEMENTED();
    }

    void insnINC(addr& dest)
    {
        NOT_IMPLEMENTED();
    }

    void insnDEC(uint8_t& dest)
    {
        NOT_IMPLEMENTED();
    }

    void insnDEC(addr& dest)
    {
        NOT_IMPLEMENTED();
    }

    void insnDAA()
    {
        NOT_IMPLEMENTED();
    }

    void insnCPL()
    {
        NOT_IMPLEMENTED();
    }

    // GMB 16bit-Arithmetic/logical Commands

    void insnADD(uint16_t& dest, uint16_t src)
    {
        NOT_IMPLEMENTED();
    }

    void insnADD(addr& dest, uint16_t src)
    {
        NOT_IMPLEMENTED();
    }

    void insnINC(uint16_t& dest)
    {
        NOT_IMPLEMENTED();
    }

    void insnDEC(uint16_t& dest)
    {
        NOT_IMPLEMENTED();
    }

    // GMB Rotate - and Shift - Commands

    void insnRLC(uint8_t& dest)
    {
        NOT_IMPLEMENTED();
    }

    void insnRRC(uint8_t& dest)
    {
        NOT_IMPLEMENTED();
    }

    void insnRL(uint8_t& dest)
    {
        NOT_IMPLEMENTED();
    }

    void insnRR(uint8_t& dest)
    {
        NOT_IMPLEMENTED();
    }

    // GMB Singlebit Operation Commands

    // GMB CPU-Controlcommands

    void insnCCF()
    {
        NOT_IMPLEMENTED();
    }

    void insnSCF()
    {
        NOT_IMPLEMENTED();
    }

    void insnNOP()
    {
        NOT_IMPLEMENTED();
    }

    void insnHALT()
    {
        NOT_IMPLEMENTED();
    }

    void insnSTOP()
    {
        NOT_IMPLEMENTED();
    }

    void insnDI()
    {
        NOT_IMPLEMENTED();
    }

    void insnEI()
    {
        NOT_IMPLEMENTED();
    }

    // GMB Jumpcommands

    void insnJP(uint16_t dest)
    {
        NOT_IMPLEMENTED();
    }

    void insnJP(bool cond, uint16_t dest)
    {
        NOT_IMPLEMENTED();
    }

    void insnJR(uint16_t dest)
    {
        NOT_IMPLEMENTED();
    }

    void insnJR(bool cond, uint16_t dest)
    {
        NOT_IMPLEMENTED();
    }

    void insnCALL(uint16_t dest)
    {
        NOT_IMPLEMENTED();
    }

    void insnCALL(bool cond, uint16_t dest)
    {
        NOT_IMPLEMENTED();
    }

    void insnRET()
    {
        NOT_IMPLEMENTED();
    }

    void insnRET(bool cond)
    {
        NOT_IMPLEMENTED();
    }

    void insnRETI()
    {
        NOT_IMPLEMENTED();
    }

    void insnRST(uint8_t dest)
    {
        NOT_IMPLEMENTED();
    }

    void insnINVALID()
    {
        NOT_IMPLEMENTED();
    }

    void CpuZ80::executeCB()
    {
    }

    void CpuZ80::executeMain()
    {
        auto opcode = fetch8();
        switch (opcode)
        {
        case 0x00: insnNOP(); break;
        case 0x01: insnLD(BC, fetch16()); break;
        case 0x02: insnLD(addr(BC), A); break;
        case 0x03: insnINC(BC); break;
        case 0x04: insnINC(B); break;
        case 0x05: insnDEC(B); break;
        case 0x06: insnLD(B, fetch8()); break;
        case 0x07: insnRLC(A); break;
        case 0x08: insnLD(addr(fetch16()), SP); break;
        case 0x09: insnADD(HL, BC); break;
        case 0x0a: insnLD(A, read8(BC)); break;
        case 0x0b: insnDEC(BC); break;
        case 0x0c: insnINC(C); break;
        case 0x0d: insnDEC(C); break;
        case 0x0e: insnLD(C, fetch8()); break;
        case 0x0f: insnRRC(A); break;
        case 0x10: insnSTOP(); fetch8(); break;
        case 0x11: insnLD(DE, fetch16()); break;
        case 0x12: insnLD(addr(DE), A); break;
        case 0x13: insnINC(DE); break;
        case 0x14: insnINC(D); break;
        case 0x15: insnDEC(D); break;
        case 0x16: insnLD(D, fetch8()); break;
        case 0x17: insnRL(A); break;
        case 0x18: insnJR(fetchPC()); break;
        case 0x19: insnADD(HL, DE); break;
        case 0x1a: insnLD(A, read8(DE)); break;
        case 0x1b: insnDEC(DE); break;
        case 0x1c: insnINC(E); break;
        case 0x1d: insnDEC(E); break;
        case 0x1e: insnLD(E, fetch8()); break;
        case 0x1f: insnRR(A); break;
        case 0x20: insnJR(FLAG_Z != 0, fetchPC()); break;
        case 0x21: insnLD(HL, fetch16()); break;
        case 0x22: insnLD(addr(HL++), A); break;
        case 0x23: insnINC(HL); break;
        case 0x24: insnINC(H); break;
        case 0x25: insnDEC(H); break;
        case 0x26: insnLD(H, fetch8()); break;
        case 0x27: insnDAA(); break;
        case 0x28: insnJR(FLAG_Z == 0, fetchPC()); break;
        case 0x29: insnADD(HL, HL); break;
        case 0x2a: insnLD(A, read8(HL++)); break;
        case 0x2b: insnDEC(HL); break;
        case 0x2c: insnINC(L); break;
        case 0x2d: insnDEC(L); break;
        case 0x2e: insnLD(L, fetch8()); break;
        case 0x2f: insnCPL(); break;
        case 0x30: insnJR(FLAG_C == 0, fetchPC()); break;
        case 0x31: insnLD(SP, fetch16()); break;
        case 0x32: insnLD(addr(HL--), A); break;
        case 0x33: insnINC(SP); break;
        case 0x34: insnINC(addr(HL)); break;
        case 0x35: insnDEC(addr(HL)); break;
        case 0x36: insnLD(addr(HL), fetch8()); break;
        case 0x37: insnSCF(); break;
        case 0x38: insnJR(C != 0, fetchPC()); break;
        case 0x39: insnADD(HL, SP); break;
        case 0x3a: insnLD(A, read8(HL--)); break;
        case 0x3b: insnDEC(SP); break;
        case 0x3c: insnINC(A); break;
        case 0x3d: insnDEC(A); break;
        case 0x3e: insnLD(A, fetch8()); break;
        case 0x3f: insnCCF(); break;
        case 0x40: insnLD(B, B); break;
        case 0x41: insnLD(B, C); break;
        case 0x42: insnLD(B, D); break;
        case 0x43: insnLD(B, E); break;
        case 0x44: insnLD(B, H); break;
        case 0x45: insnLD(B, L); break;
        case 0x46: insnLD(B, read8(HL)); break;
        case 0x47: insnLD(B, A); break;
        case 0x48: insnLD(C, B); break;
        case 0x49: insnLD(C, C); break;
        case 0x4a: insnLD(C, D); break;
        case 0x4b: insnLD(C, E); break;
        case 0x4c: insnLD(C, H); break;
        case 0x4d: insnLD(C, L); break;
        case 0x4e: insnLD(C, read8(HL)); break;
        case 0x4f: insnLD(C, A); break;
        case 0x50: insnLD(D, B); break;
        case 0x51: insnLD(D, C); break;
        case 0x52: insnLD(D, D); break;
        case 0x53: insnLD(D, E); break;
        case 0x54: insnLD(D, H); break;
        case 0x55: insnLD(D, L); break;
        case 0x56: insnLD(D, read8(HL)); break;
        case 0x57: insnLD(D, A); break;
        case 0x58: insnLD(E, B); break;
        case 0x59: insnLD(E, C); break;
        case 0x5a: insnLD(E, D); break;
        case 0x5b: insnLD(E, E); break;
        case 0x5c: insnLD(E, H); break;
        case 0x5d: insnLD(E, L); break;
        case 0x5e: insnLD(E, read8(HL)); break;
        case 0x5f: insnLD(E, A); break;
        case 0x60: insnLD(H, B); break;
        case 0x61: insnLD(H, C); break;
        case 0x62: insnLD(H, D); break;
        case 0x63: insnLD(H, E); break;
        case 0x64: insnLD(H, H); break;
        case 0x65: insnLD(H, L); break;
        case 0x66: insnLD(H, read8(HL)); break;
        case 0x67: insnLD(H, A); break;
        case 0x68: insnLD(L, B); break;
        case 0x69: insnLD(L, C); break;
        case 0x6a: insnLD(L, D); break;
        case 0x6b: insnLD(L, E); break;
        case 0x6c: insnLD(L, H); break;
        case 0x6d: insnLD(L, L); break;
        case 0x6e: insnLD(L, read8(HL)); break;
        case 0x6f: insnLD(L, A); break;
        case 0x70: insnLD(addr(HL), B); break;
        case 0x71: insnLD(addr(HL), C); break;
        case 0x72: insnLD(addr(HL), D); break;
        case 0x73: insnLD(addr(HL), E); break;
        case 0x74: insnLD(addr(HL), H); break;
        case 0x75: insnLD(addr(HL), L); break;
        case 0x76: insnHALT(); break;
        case 0x77: insnLD(addr(HL), A); break;
        case 0x78: insnLD(A, B); break;
        case 0x79: insnLD(A, C); break;
        case 0x7a: insnLD(A, D); break;
        case 0x7b: insnLD(A, E); break;
        case 0x7c: insnLD(A, H); break;
        case 0x7d: insnLD(A, L); break;
        case 0x7e: insnLD(A, read8(HL)); break;
        case 0x7f: insnLD(A, A); break;
        case 0x80: insnADD(A, B); break;
        case 0x81: insnADD(A, C); break;
        case 0x82: insnADD(A, D); break;
        case 0x83: insnADD(A, E); break;
        case 0x84: insnADD(A, H); break;
        case 0x85: insnADD(A, L); break;
        case 0x86: insnADD(A, read8(HL)); break;
        case 0x87: insnADD(A, A); break;
        case 0x88: insnADC(A, B); break;
        case 0x89: insnADC(A, C); break;
        case 0x8a: insnADC(A, D); break;
        case 0x8b: insnADC(A, E); break;
        case 0x8c: insnADC(A, H); break;
        case 0x8d: insnADC(A, L); break;
        case 0x8e: insnADC(A, read8(HL)); break;
        case 0x8f: insnADC(A, A); break;
        case 0x90: insnSUB(B); break;
        case 0x91: insnSUB(C); break;
        case 0x92: insnSUB(D); break;
        case 0x93: insnSUB(E); break;
        case 0x94: insnSUB(H); break;
        case 0x95: insnSUB(L); break;
        case 0x96: insnSUB(read8(HL)); break;
        case 0x97: insnSUB(A); break;
        case 0x98: insnSBC(A, B); break;
        case 0x99: insnSBC(A, C); break;
        case 0x9a: insnSBC(A, D); break;
        case 0x9b: insnSBC(A, E); break;
        case 0x9c: insnSBC(A, H); break;
        case 0x9d: insnSBC(A, L); break;
        case 0x9e: insnSBC(A, read8(HL)); break;
        case 0x9f: insnSBC(A, A); break;
        case 0xa0: insnAND(B); break;
        case 0xa1: insnAND(C); break;
        case 0xa2: insnAND(D); break;
        case 0xa3: insnAND(E); break;
        case 0xa4: insnAND(H); break;
        case 0xa5: insnAND(L); break;
        case 0xa6: insnAND(read8(HL)); break;
        case 0xa7: insnAND(A); break;
        case 0xa8: insnXOR(B); break;
        case 0xa9: insnXOR(C); break;
        case 0xaa: insnXOR(D); break;
        case 0xab: insnXOR(E); break;
        case 0xac: insnXOR(H); break;
        case 0xad: insnXOR(L); break;
        case 0xae: insnXOR(read8(HL)); break;
        case 0xaf: insnXOR(A); break;
        case 0xb0: insnOR(B); break;
        case 0xb1: insnOR(C); break;
        case 0xb2: insnOR(D); break;
        case 0xb3: insnOR(E); break;
        case 0xb4: insnOR(H); break;
        case 0xb5: insnOR(L); break;
        case 0xb6: insnOR(read8(HL)); break;
        case 0xb7: insnOR(A); break;
        case 0xb8: insnCP(B); break;
        case 0xb9: insnCP(C); break;
        case 0xba: insnCP(D); break;
        case 0xbb: insnCP(E); break;
        case 0xbc: insnCP(H); break;
        case 0xbd: insnCP(L); break;
        case 0xbe: insnCP(read8(HL)); break;
        case 0xbf: insnCP(A); break;
        case 0xc0: insnRET(FLAG_Z != 0); break;
        case 0xc1: insnPOP(BC); break;
        case 0xc2: insnJP(FLAG_Z != 0, fetch16()); break;
        case 0xc3: insnJP(fetch16()); break;
        case 0xc4: insnCALL(FLAG_Z != 0, fetch16()); break;
        case 0xc5: insnPUSH(BC); break;
        case 0xc6: insnADD(A, fetch8()); break;
        case 0xc7: insnRST(0x00); break;
        case 0xc8: insnRET(FLAG_Z == 0); break;
        case 0xc9: insnRET(); break;
        case 0xca: insnJP(FLAG_Z == 0, fetch16()); break;
        case 0xcb: executeCB(); break;
        case 0xcc: insnCALL(FLAG_Z == 0, fetch16()); break;
        case 0xcd: insnCALL(fetch16()); break;
        case 0xce: insnADC(A, fetch8()); break;
        case 0xcf: insnRST(0x08); break;
        case 0xd0: insnRET(FLAG_C == 0); break;
        case 0xd1: insnPOP(DE); break;
        case 0xd2: insnJP(FLAG_C == 0, fetch16()); break;
        case 0xd3: insnINVALID(); break;
        case 0xd4: insnCALL(FLAG_C == 0, fetch16()); break;
        case 0xd5: insnPUSH(DE); break;
        case 0xd6: insnSUB(fetch8()); break;
        case 0xd7: insnRST(0x10); break;
        case 0xd8: insnRET(FLAG_C != 0); break;
        case 0xd9: insnRETI(); break;
        case 0xda: insnJP(FLAG_C != 0, fetch16()); break;
        case 0xdb: insnINVALID(); break;
        case 0xdc: insnCALL(FLAG_C != 0, fetch16()); break;
        case 0xdd: insnINVALID(); break;
        case 0xde: insnSBC(A, fetch8()); break;
        case 0xdf: insnRST(0x18); break;
        case 0xe0: insnLD(addr(0xff00 + fetch8()), A); break;
        case 0xe1: insnPOP(HL); break;
        case 0xe2: insnLD((C), A); break;
        case 0xe3: insnINVALID(); break;
        case 0xe4: insnINVALID(); break;
        case 0xe5: insnPUSH(HL); break;
        case 0xe6: insnAND(fetch8()); break;
        case 0xe7: insnRST(0x20); break;
        case 0xe8: insnADD(SP, fetchPC()); break;
        case 0xe9: insnJP((HL)); break;
        case 0xea: insnLD(addr(fetch16()), A); break;
        case 0xeb: insnINVALID(); break;
        case 0xec: insnINVALID(); break;
        case 0xed: insnINVALID(); break;
        case 0xee: insnXOR(fetch8()); break;
        case 0xef: insnRST(0x28); break;
        case 0xf0: insnLD(A, read8(0xff00 + fetch8())); break;
        case 0xf1: insnPOP(AF); break;
        case 0xf2: insnLD(A, (C)); break;
        case 0xf3: insnDI(); break;
        case 0xf4: insnINVALID(); break;
        case 0xf5: insnPUSH(AF); break;
        case 0xf6: insnOR(fetch8()); break;
        case 0xf7: insnRST(0x30); break;
        case 0xf8: insnLD(HL, SP + fetchPC()); break;
        case 0xf9: insnLD(SP, HL); break;
        case 0xfa: insnLD(A, read8(fetch16())); break;
        case 0xfb: insnEI(); break;
        case 0xfc: insnINVALID(); break;
        case 0xfd: insnINVALID(); break;
        case 0xfe: insnCP(fetch8()); break;
        case 0xff: insnRST(0x38); break;
        default: break;
        }
    }

    void CpuZ80::execute()
    {
        while (mExecutedTicks < mDesiredTicks)
        {
            uint8_t insn = fetch8();
            switch (insn)
            {
            case 0:
            default:
                EMU_ASSERT(false);
            }
        }
    }

    uint16_t CpuZ80::disassemble(char* /*buffer*/, size_t /*size*/, uint16_t addr)
    {
        return addr;
    }

    void CpuZ80::serialize(emu::ISerializer& serializer)
    {
        uint32_t version = 1;
        serializer.serialize(version);
        serializer.serialize(AF);
        serializer.serialize(BC);
        serializer.serialize(DE);
        serializer.serialize(HL);
        serializer.serialize(SP);
        serializer.serialize(PC);
        serializer.serialize(FLAG_Z);
        serializer.serialize(FLAG_N);
        serializer.serialize(FLAG_H);
        serializer.serialize(FLAG_C);
    }
}
