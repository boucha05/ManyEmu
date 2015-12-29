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
#define IME         mRegs.r8.ime

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

    enum ADDR_MODE
    {
        ADDR_NONE,          ADDR_MEM_BC_A,      ADDR_MEM_C_A,       ADDR_MEM_DE_A,
        ADDR_MEM_HL,        ADDR_MEM_HL_A,      ADDR_MEM_HL_B,      ADDR_MEM_HL_C,
        ADDR_MEM_HL_D,      ADDR_MEM_HL_E,      ADDR_MEM_HL_H,      ADDR_MEM_HL_L,
        ADDR_MEM_HL_D8,     ADDR_MEM_HL_INC_A,  ADDR_MEM_HL_DEC_A,  ADDR_MEM_A16_A,
        ADDR_MEM_A16_SP,    ADDR_MEM_A8_A,      ADDR_ZERO,          ADDR_RST,
        ADDR_A,             ADDR_A_MEM_BC,      ADDR_A_MEM_C,       ADDR_A_MEM_DE,
        ADDR_A_MEM_HL,      ADDR_A_MEM_HL_INC,  ADDR_A_MEM_HL_DEC,  ADDR_A_MEM_A16,
        ADDR_A_MEM_A8,      ADDR_A_A,           ADDR_A_B,           ADDR_A_C,
        ADDR_A_D,           ADDR_A_E,           ADDR_A_H,           ADDR_A_L,
        ADDR_A_D8,          ADDR_AF,            ADDR_B,             ADDR_B_MEM_HL,
        ADDR_B_A,           ADDR_B_B,           ADDR_B_C,           ADDR_B_D,
        ADDR_B_E,           ADDR_B_H,           ADDR_B_L,           ADDR_B_D8,
        ADDR_BC,            ADDR_BC_D16,        ADDR_C,             ADDR_C_MEM_HL,
        ADDR_C_A,           ADDR_C_B,           ADDR_C_C,           ADDR_C_D,
        ADDR_C_E,           ADDR_C_H,           ADDR_C_L,           ADDR_C_A16,
        ADDR_C_D8,          ADDR_C_R8,          ADDR_CB,            ADDR_D,
        ADDR_D_MEM_HL,      ADDR_D_A,           ADDR_D_B,           ADDR_D_C,
        ADDR_D_D,           ADDR_D_E,           ADDR_D_H,           ADDR_D_L,
        ADDR_D_D8,          ADDR_DE,            ADDR_DE_D16,        ADDR_E,
        ADDR_E_MEM_HL,      ADDR_E_A,           ADDR_E_B,           ADDR_E_C,
        ADDR_E_D,           ADDR_E_E,           ADDR_E_H,           ADDR_E_L,
        ADDR_E_D8,          ADDR_H,             ADDR_H_MEM_HL,      ADDR_H_A,
        ADDR_H_B,           ADDR_H_C,           ADDR_H_D,           ADDR_H_E,
        ADDR_H_H,           ADDR_H_L,           ADDR_H_D8,          ADDR_HL,
        ADDR_HL_BC,         ADDR_HL_DE,         ADDR_HL_HL,         ADDR_HL_SP,
        ADDR_HL_SP_INC_R8,  ADDR_HL_D16,        ADDR_L,             ADDR_L_MEM_HL,
        ADDR_L_A,           ADDR_L_B,           ADDR_L_C,           ADDR_L_D,
        ADDR_L_E,           ADDR_L_H,           ADDR_L_L,           ADDR_L_D8,
        ADDR_NC,            ADDR_NC_A16,        ADDR_NC_R8,         ADDR_NZ,
        ADDR_NZ_A16,        ADDR_NZ_R8,         ADDR_SP,            ADDR_SP_HL,
        ADDR_SP_D16,        ADDR_SP_R8,         ADDR_Z,             ADDR_Z_A16,
        ADDR_Z_R8,          ADDR_A16,           ADDR_D8,            ADDR_R8,
        ADDR_CB1_A,         ADDR_CB1_B,         ADDR_CB1_C,         ADDR_CB1_D,
        ADDR_CB1_E,         ADDR_CB1_H,         ADDR_CB1_L,         ADDR_CB1_MEM_HL,
        ADDR_CB2_A,         ADDR_CB2_B,         ADDR_CB2_C,         ADDR_CB2_D,
        ADDR_CB2_E,         ADDR_CB2_H,         ADDR_CB2_L,         ADDR_CB2_MEM_HL,
    };

    static const char* addrModeFormat[] =
    {
        "",             "(BC),A",       "(C),A",        "(DE),A",
        "(HL)",         "(HL),A",       "(HL),B",       "(HL),C",
        "(HL),D",       "(HL),E",       "(HL),H",       "(HL),L",
        "(HL),$%02X",   "(HL+),A",      "(HL-),A",      "($%04X),A",
        "($%04X),SP",   "($%02X),A",    "0",            "$%02X",
        "A",            "A,(BC)",       "A,(C)",        "A,(DE)",
        "A,(HL)",       "A,(HL+)",      "A,(HL-)",      "A,($%04X)",
        "A,($%02X)",    "A,A",          "A,B",          "A,C",
        "A,D",          "A,E",          "A,H",          "A,L",
        "A,$%02X",      "AF",           "B",            "B,(HL)",
        "B,A",          "B,B",          "B,C",          "B,D",
        "B,E",          "B,H",          "B,L",          "B,$%02X",
        "BC",           "BC,$%04X",     "C",            "C,(HL)",
        "C,A",          "C,B",          "C,C",          "C,D",
        "C,E",          "C,H",          "C,L",          "C,$%04X",
        "C,$%02X",      "C,$%04X",      "",             "D",
        "D,(HL)",       "D,A",          "D,B",          "D,C",
        "D,D",          "D,E",          "D,H",          "D,L",
        "D,$%02X",      "DE",           "DE,$%04X",     "E",
        "E,(HL)",       "E,A",          "E,B",          "E,C",
        "E,D",          "E,E",          "E,H",          "E,L",
        "E,$%02X",      "H",            "H,(HL)",       "H,A",
        "H,B",          "H,C",          "H,D",          "H,E",
        "H,H",          "H,L",          "H,$%02X",      "HL",
        "HL,BC",        "HL,DE",        "HL,HL",        "HL,SP",
        "HL,SP+$%04X",  "HL,$%04X",     "L",            "L,(HL)",
        "L,A",          "L,B",          "L,C",          "L,D",
        "L,E",          "L,H",          "L,L",          "L,$%02X",
        "NC",           "NC,$%04X",     "NC,$%04X",     "NZ",
        "NZ,$%04X",     "NZ,$%04X",     "SP",           "SP,HL",
        "SP,$%04X",     "SP,$%04X",     "Z",            "Z,$%04X",
        "Z,$%04X",      "$%04X",        "$%02X",        "$%04X",
        "A",            "B",            "C",            "D",
        "E",            "H",            "L",            "(HL)",
        "%d,A",         "%d,B",         "%d,C",         "%d,D",
        "%d,E",         "%d,H",         "%d,L",         "%d,(HL)",
    };

    enum INSN_TYPE
    {
        INSN_ADC,       INSN_ADD,       INSN_AND,       INSN_BIT,
        INSN_CALL,      INSN_CCF,       INSN_CP,        INSN_CPL,
        INSN_DAA,       INSN_DEC,       INSN_DI,        INSN_EI,
        INSN_HALT,      INSN_INC,       INSN_INVALID,   INSN_JP,
        INSN_JR,        INSN_LD,        INSN_LDH,       INSN_NOP,
        INSN_OR,        INSN_POP,       INSN_PREFIX,    INSN_PUSH,
        INSN_RES,       INSN_RET,       INSN_RETI,      INSN_RL,
        INSN_RLA,       INSN_RLC,       INSN_RLCA,      INSN_RR,
        INSN_RRA,       INSN_RRC,       INSN_RRCA,      INSN_RST,
        INSN_SBC,       INSN_SCF,       INSN_SET,       INSN_SLA,
        INSN_SRA,       INSN_SRL,       INSN_STOP,      INSN_SUB,
        INSN_SWAP,      INSN_XOR,
    };

    static const char* insnName[] =
    {
        "ADC",  "ADD",  "AND",  "BIT",  "CALL", "CCF",  "CP",   "CPL",
        "DAA",  "DEC",  "DI",   "EI",   "HALT", "INC",  "???",  "JP",
        "JR",   "LD",   "LDH",  "NOP",  "OR",   "POP",  "???",  "PUSH",
        "RES",  "RET",  "RETI", "RL",   "RLA",  "RLC",  "RLCA", "RR",
        "RRA",  "RRC",  "RRCA", "RST",  "SBC",  "SCF",  "SET",  "SLA",
        "SRA",  "SRL",  "STOP", "SUB",  "SWAP", "XOR",
    };

    static const uint8_t insnTypeMain[] =
    {
        INSN_NOP,       INSN_LD,        INSN_LD,        INSN_INC,       INSN_INC,       INSN_DEC,       INSN_LD,        INSN_RLCA,
        INSN_LD,        INSN_ADD,       INSN_LD,        INSN_DEC,       INSN_INC,       INSN_DEC,       INSN_LD,        INSN_RRCA,
        INSN_STOP,      INSN_LD,        INSN_LD,        INSN_INC,       INSN_INC,       INSN_DEC,       INSN_LD,        INSN_RLA,
        INSN_JR,        INSN_ADD,       INSN_LD,        INSN_DEC,       INSN_INC,       INSN_DEC,       INSN_LD,        INSN_RRA,
        INSN_JR,        INSN_LD,        INSN_LD,        INSN_INC,       INSN_INC,       INSN_DEC,       INSN_LD,        INSN_DAA,
        INSN_JR,        INSN_ADD,       INSN_LD,        INSN_DEC,       INSN_INC,       INSN_DEC,       INSN_LD,        INSN_CPL,
        INSN_JR,        INSN_LD,        INSN_LD,        INSN_INC,       INSN_INC,       INSN_DEC,       INSN_LD,        INSN_SCF,
        INSN_JR,        INSN_ADD,       INSN_LD,        INSN_DEC,       INSN_INC,       INSN_DEC,       INSN_LD,        INSN_CCF,
        INSN_LD,        INSN_LD,        INSN_LD,        INSN_LD,        INSN_LD,        INSN_LD,        INSN_LD,        INSN_LD,
        INSN_LD,        INSN_LD,        INSN_LD,        INSN_LD,        INSN_LD,        INSN_LD,        INSN_LD,        INSN_LD,
        INSN_LD,        INSN_LD,        INSN_LD,        INSN_LD,        INSN_LD,        INSN_LD,        INSN_LD,        INSN_LD,
        INSN_LD,        INSN_LD,        INSN_LD,        INSN_LD,        INSN_LD,        INSN_LD,        INSN_LD,        INSN_LD,
        INSN_LD,        INSN_LD,        INSN_LD,        INSN_LD,        INSN_LD,        INSN_LD,        INSN_LD,        INSN_LD,
        INSN_LD,        INSN_LD,        INSN_LD,        INSN_LD,        INSN_LD,        INSN_LD,        INSN_LD,        INSN_LD,
        INSN_LD,        INSN_LD,        INSN_LD,        INSN_LD,        INSN_LD,        INSN_LD,        INSN_HALT,      INSN_LD,
        INSN_LD,        INSN_LD,        INSN_LD,        INSN_LD,        INSN_LD,        INSN_LD,        INSN_LD,        INSN_LD,
        INSN_ADD,       INSN_ADD,       INSN_ADD,       INSN_ADD,       INSN_ADD,       INSN_ADD,       INSN_ADD,       INSN_ADD,
        INSN_ADC,       INSN_ADC,       INSN_ADC,       INSN_ADC,       INSN_ADC,       INSN_ADC,       INSN_ADC,       INSN_ADC,
        INSN_SUB,       INSN_SUB,       INSN_SUB,       INSN_SUB,       INSN_SUB,       INSN_SUB,       INSN_SUB,       INSN_SUB,
        INSN_SBC,       INSN_SBC,       INSN_SBC,       INSN_SBC,       INSN_SBC,       INSN_SBC,       INSN_SBC,       INSN_SBC,
        INSN_AND,       INSN_AND,       INSN_AND,       INSN_AND,       INSN_AND,       INSN_AND,       INSN_AND,       INSN_AND,
        INSN_XOR,       INSN_XOR,       INSN_XOR,       INSN_XOR,       INSN_XOR,       INSN_XOR,       INSN_XOR,       INSN_XOR,
        INSN_OR,        INSN_OR,        INSN_OR,        INSN_OR,        INSN_OR,        INSN_OR,        INSN_OR,        INSN_OR,
        INSN_CP,        INSN_CP,        INSN_CP,        INSN_CP,        INSN_CP,        INSN_CP,        INSN_CP,        INSN_CP,
        INSN_RET,       INSN_POP,       INSN_JP,        INSN_JP,        INSN_CALL,      INSN_PUSH,      INSN_ADD,       INSN_RST,
        INSN_RET,       INSN_RET,       INSN_JP,        INSN_PREFIX,    INSN_CALL,      INSN_CALL,      INSN_ADC,       INSN_RST,
        INSN_RET,       INSN_POP,       INSN_JP,        INSN_INVALID,   INSN_CALL,      INSN_PUSH,      INSN_SUB,       INSN_RST,
        INSN_RET,       INSN_RETI,      INSN_JP,        INSN_INVALID,   INSN_CALL,      INSN_INVALID,   INSN_SBC,       INSN_RST,
        INSN_LDH,       INSN_POP,       INSN_LD,        INSN_INVALID,   INSN_INVALID,   INSN_PUSH,      INSN_AND,       INSN_RST,
        INSN_ADD,       INSN_JP,        INSN_LD,        INSN_INVALID,   INSN_INVALID,   INSN_INVALID,   INSN_XOR,       INSN_RST,
        INSN_LDH,       INSN_POP,       INSN_LD,        INSN_DI,        INSN_INVALID,   INSN_PUSH,      INSN_OR,        INSN_RST,
        INSN_LD,        INSN_LD,        INSN_LD,        INSN_EI,        INSN_INVALID,   INSN_INVALID,   INSN_CP,        INSN_RST,
    };

    static const uint8_t insnTypeCB[] =
    {
        INSN_RLC,   INSN_RRC,   INSN_RL,    INSN_RR,    INSN_SLA,   INSN_SRA,   INSN_SWAP,  INSN_SRL,
        INSN_BIT,   INSN_BIT,   INSN_BIT,   INSN_BIT,   INSN_BIT,   INSN_BIT,   INSN_BIT,   INSN_BIT,
        INSN_RES,   INSN_RES,   INSN_RES,   INSN_RES,   INSN_RES,   INSN_RES,   INSN_RES,   INSN_RES,
        INSN_SET,   INSN_SET,   INSN_SET,   INSN_SET,   INSN_SET,   INSN_SET,   INSN_SET,   INSN_SET,
    };

    static const uint8_t addrModeMain[] =
    {
        ADDR_NONE,          ADDR_BC_D16,        ADDR_MEM_BC_A,      ADDR_BC,            ADDR_B,             ADDR_B,             ADDR_B_D8,          ADDR_NONE,
        ADDR_MEM_A16_SP,    ADDR_HL_BC,         ADDR_A_MEM_BC,      ADDR_BC,            ADDR_C,             ADDR_C,             ADDR_C_D8,          ADDR_NONE,
        ADDR_ZERO,          ADDR_DE_D16,        ADDR_MEM_DE_A,      ADDR_DE,            ADDR_D,             ADDR_D,             ADDR_D_D8,          ADDR_NONE,
        ADDR_R8,            ADDR_HL_DE,         ADDR_A_MEM_DE,      ADDR_DE,            ADDR_E,             ADDR_E,             ADDR_E_D8,          ADDR_NONE,
        ADDR_NZ_R8,         ADDR_HL_D16,        ADDR_MEM_HL_INC_A,  ADDR_HL,            ADDR_H,             ADDR_H,             ADDR_H_D8,          ADDR_NONE,
        ADDR_Z_R8,          ADDR_HL_HL,         ADDR_A_MEM_HL_INC,  ADDR_HL,            ADDR_L,             ADDR_L,             ADDR_L_D8,          ADDR_NONE,
        ADDR_NC_R8,         ADDR_SP_D16,        ADDR_MEM_HL_DEC_A,  ADDR_SP,            ADDR_MEM_HL,        ADDR_MEM_HL,        ADDR_MEM_HL_D8,     ADDR_NONE,
        ADDR_C_R8,          ADDR_HL_SP,         ADDR_A_MEM_HL_DEC,  ADDR_SP,            ADDR_A,             ADDR_A,             ADDR_A_D8,          ADDR_NONE,
        ADDR_B_B,           ADDR_B_C,           ADDR_B_D,           ADDR_B_E,           ADDR_B_H,           ADDR_B_L,           ADDR_B_MEM_HL,      ADDR_B_A,
        ADDR_C_B,           ADDR_C_C,           ADDR_C_D,           ADDR_C_E,           ADDR_C_H,           ADDR_C_L,           ADDR_C_MEM_HL,      ADDR_C_A,
        ADDR_D_B,           ADDR_D_C,           ADDR_D_D,           ADDR_D_E,           ADDR_D_H,           ADDR_D_L,           ADDR_D_MEM_HL,      ADDR_D_A,
        ADDR_E_B,           ADDR_E_C,           ADDR_E_D,           ADDR_E_E,           ADDR_E_H,           ADDR_E_L,           ADDR_E_MEM_HL,      ADDR_E_A,
        ADDR_H_B,           ADDR_H_C,           ADDR_H_D,           ADDR_H_E,           ADDR_H_H,           ADDR_H_L,           ADDR_H_MEM_HL,      ADDR_H_A,
        ADDR_L_B,           ADDR_L_C,           ADDR_L_D,           ADDR_L_E,           ADDR_L_H,           ADDR_L_L,           ADDR_L_MEM_HL,      ADDR_L_A,
        ADDR_MEM_HL_B,      ADDR_MEM_HL_C,      ADDR_MEM_HL_D,      ADDR_MEM_HL_E,      ADDR_MEM_HL_H,      ADDR_MEM_HL_L,      ADDR_NONE,          ADDR_MEM_HL_A,
        ADDR_A_B,           ADDR_A_C,           ADDR_A_D,           ADDR_A_E,           ADDR_A_H,           ADDR_A_L,           ADDR_A_MEM_HL,      ADDR_A_A,
        ADDR_A_B,           ADDR_A_C,           ADDR_A_D,           ADDR_A_E,           ADDR_A_H,           ADDR_A_L,           ADDR_A_MEM_HL,      ADDR_A_A,
        ADDR_A_B,           ADDR_A_C,           ADDR_A_D,           ADDR_A_E,           ADDR_A_H,           ADDR_A_L,           ADDR_A_MEM_HL,      ADDR_A_A,
        ADDR_B,             ADDR_C,             ADDR_D,             ADDR_E,             ADDR_H,             ADDR_L,             ADDR_MEM_HL,        ADDR_A,
        ADDR_A_B,           ADDR_A_C,           ADDR_A_D,           ADDR_A_E,           ADDR_A_H,           ADDR_A_L,           ADDR_A_MEM_HL,      ADDR_A_A,
        ADDR_B,             ADDR_C,             ADDR_D,             ADDR_E,             ADDR_H,             ADDR_L,             ADDR_MEM_HL,        ADDR_A,
        ADDR_B,             ADDR_C,             ADDR_D,             ADDR_E,             ADDR_H,             ADDR_L,             ADDR_MEM_HL,        ADDR_A,
        ADDR_B,             ADDR_C,             ADDR_D,             ADDR_E,             ADDR_H,             ADDR_L,             ADDR_MEM_HL,        ADDR_A,
        ADDR_B,             ADDR_C,             ADDR_D,             ADDR_E,             ADDR_H,             ADDR_L,             ADDR_MEM_HL,        ADDR_A,
        ADDR_NZ,            ADDR_BC,            ADDR_NZ_A16,        ADDR_A16,           ADDR_NZ_A16,        ADDR_BC,            ADDR_A_D8,          ADDR_RST,
        ADDR_Z,             ADDR_NONE,          ADDR_Z_A16,         ADDR_CB,            ADDR_Z_A16,         ADDR_A16,           ADDR_A_D8,          ADDR_RST,
        ADDR_NC,            ADDR_DE,            ADDR_NC_A16,        ADDR_NONE,          ADDR_NC_A16,        ADDR_DE,            ADDR_D8,            ADDR_RST,
        ADDR_C,             ADDR_NONE,          ADDR_C_A16,         ADDR_NONE,          ADDR_C_A16,         ADDR_NONE,          ADDR_A_D8,          ADDR_RST,
        ADDR_MEM_A8_A,      ADDR_HL,            ADDR_MEM_C_A,       ADDR_NONE,          ADDR_NONE,          ADDR_HL,            ADDR_D8,            ADDR_RST,
        ADDR_SP_R8,         ADDR_MEM_HL,        ADDR_MEM_A16_A,     ADDR_NONE,          ADDR_NONE,          ADDR_NONE,          ADDR_D8,            ADDR_RST,
        ADDR_A_MEM_A8,      ADDR_AF,            ADDR_A_MEM_C,       ADDR_NONE,          ADDR_NONE,          ADDR_AF,            ADDR_D8,            ADDR_RST,
        ADDR_HL_SP_INC_R8,  ADDR_SP_HL,         ADDR_A_MEM_A16,     ADDR_NONE,          ADDR_NONE,          ADDR_NONE,          ADDR_D8,            ADDR_RST,
    };

    static const uint8_t addrModeCB1[] =
    {
        ADDR_CB1_B,         ADDR_CB1_C,         ADDR_CB1_D,         ADDR_CB1_E,
        ADDR_CB1_H,         ADDR_CB1_L,         ADDR_CB1_MEM_HL,    ADDR_CB1_A,
    };

    static const uint8_t addrModeCB2[] =
    {
        ADDR_CB2_B,         ADDR_CB2_C,         ADDR_CB2_D,         ADDR_CB2_E,
        ADDR_CB2_H,         ADDR_CB2_L,         ADDR_CB2_MEM_HL,    ADDR_CB2_A,
    };

    static const uint8_t refTicksMain[] =
    {
        4,  12, 8,  8,  4,  4,  8,  4,  20, 8,  8,  8,  4,  4,  8,  4,
        4,  12, 8,  8,  4,  4,  8,  4,  12, 8,  8,  8,  4,  4,  8,  4,
        8,  12, 8,  8,  4,  4,  8,  4,  8,  8,  8,  8,  4,  4,  8,  4,
        8,  12, 8,  8,  12, 12, 12, 4,  8,  8,  8,  8,  4,  4,  8,  4,
        4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
        4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
        4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
        8,  8,  8,  8,  8,  8,  4,  8,  4,  4,  4,  4,  4,  4,  8,  4,
        4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
        4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
        4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
        4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
        8,  12, 12, 16, 12, 16, 8,  16, 8,  16, 12, 4,  12, 24, 8,  16,
        8,  12, 12, 0,  12, 16, 8,  16, 8,  16, 12, 0,  12, 0,  8,  16,
        12, 12, 8,  0,  0,  16, 8,  16, 16, 4,  16, 0,  0,  0,  8,  16,
        12, 12, 8,  4,  0,  16, 8,  16, 12, 8,  16, 4,  0,  0,  8,  16,
    };

    static const uint8_t refTicksCB[] =
    {
        8,  8,  8,  8,  8,  8,  16, 8
    };

    static const uint8_t refTicksCond_call = 12;
    static const uint8_t refTicksCond_ret = 12;
    static const uint8_t refTicksCond_jp = 4;
    static const uint8_t refTicksCond_jr = 4;

    static const uint8_t FLAG_Z = 0x80;
    static const uint8_t FLAG_N = 0x40;
    static const uint8_t FLAG_H = 0x20;
    static const uint8_t FLAG_C = 0x10;
    static const uint8_t FLAG_ZNHC = FLAG_Z | FLAG_N | FLAG_H | FLAG_C;
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

    void CpuZ80::write16(uint16_t addr, uint16_t value)
    {
        emu::word16_t data;
        data.u = value;
        write8(addr, data.w.l.u);
        write8(addr + 1, data.w.h.u);
    }

    uint16_t CpuZ80::read16(uint16_t addr)
    {
        emu::word16_t value;
        value.w.l.u = read8(addr);
        value.w.h.u = read8(addr + 1);
        return value.u;
    }

    uint8_t CpuZ80::fetch8()
    {
        return read8(PC++);
    }

    uint16_t CpuZ80::fetch16()
    {
        emu::word16_t addr;
        addr.w.l.u = fetch8();
        addr.w.h.u = fetch8();
        return addr.u;
    }

    uint16_t CpuZ80::fetchSigned8()
    {
        int8_t offset = fetch8();
        uint16_t result = static_cast<int16_t>(offset);
        return result;
    }

    uint16_t CpuZ80::fetchPC()
    {
        int8_t offset = fetch8();
        uint16_t addr = (PC + static_cast<int16_t>(offset)) & 0xffff;
        return addr;
    }

    ///////////////////////////////////////////////////////////////////////////

    uint8_t CpuZ80::peek8(uint16_t& addr)
    {
        return read8(addr++);
    }

    uint16_t CpuZ80::peek16(uint16_t& addr)
    {
        emu::word16_t value;
        value.w.l.u = peek8(addr);
        value.w.h.u = peek8(addr);
        return value.u;
    }

    uint16_t CpuZ80::peekSigned8(uint16_t& addr)
    {
        int8_t offset = peek8(addr);
        uint16_t result = static_cast<int16_t>(offset);
        return result;
    }

    ///////////////////////////////////////////////////////////////////////////

    void CpuZ80::push8(uint8_t value)
    {
        uint16_t addr = --SP;
        write8(addr, value);
    }

    uint8_t CpuZ80::pop8()
    {
        uint16_t addr = SP++;
        uint8_t value = read8(addr);
        return value;
    }

    void CpuZ80::push16(uint16_t value)
    {
        emu::word16_t data;
        data.u = value;
        push8(data.w.h.u);
        push8(data.w.l.u);
    }

    uint16_t CpuZ80::pop16()
    {
        emu::word16_t data;
        data.w.l.u = pop8();
        data.w.h.u = pop8();
        return data.u;
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
        mMemory = &bus;

        for (auto index = 0; index < EMU_ARRAY_SIZE(mTicksMain); ++index)
            mTicksMain[index] = refTicksMain[index] * master_clock_divider;
        for (auto index = 0; index < EMU_ARRAY_SIZE(mTicksCB); ++index)
            mTicksCB[index] = refTicksCB[index] * master_clock_divider;
        mTicksCond_call = refTicksCond_call * master_clock_divider;
        mTicksCond_ret = refTicksCond_ret * master_clock_divider;
        mTicksCond_jp = refTicksCond_jp * master_clock_divider;
        mTicksCond_jr = refTicksCond_jr * master_clock_divider;

        IME = 0;
        reset();
        return true;
    }

    void CpuZ80::destroy()
    {
        while (!mInterruptListeners.empty())
            removeInterruptListener(*mInterruptListeners.back());

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
        setIME(false);
        resetClock();
    }

    void CpuZ80::interrupt(int32_t tick, uint16_t addr)
    {
        push16(PC);
        PC = addr;
        setIME(false);
    }

    void CpuZ80::resetClock()
    {
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

    // Flags

    void CpuZ80::flags_z0hc(uint16_t result, uint8_t value1, uint8_t value2, uint8_t carry)
    {
        auto half = (value1 & 0x0f) + (value2 & 0x0f) + carry;
        FLAGS = 0;
        FLAGS |= (result & 0xff) ? 0 : FLAG_Z;
        FLAGS |= (half >= 0x10) ? FLAG_H : 0;
        FLAGS |= (result >= 0x100) ? FLAG_C : 0;
    }

    void CpuZ80::flags_z1hc(uint16_t result, uint8_t value1, uint8_t value2, uint8_t carry)
    {
        auto half = (value1 & 0x0f) - (value2 & 0x0f) - carry;
        FLAGS = FLAG_N;
        FLAGS |= (result & 0xff) ? 0 : FLAG_Z;
        FLAGS |= (half & 0x80) ? FLAG_H : 0;
        FLAGS |= (result & 0x8000) ? FLAG_C : 0;
    }

    void CpuZ80::flags_z010(uint8_t result)
    {
        FLAGS = FLAG_H;
        FLAGS |= result ? 0 : FLAG_Z;
    }

    void CpuZ80::flags_z000(uint8_t result)
    {
        FLAGS = 0;
        FLAGS |= result ? 0 : FLAG_Z;
    }

    void CpuZ80::flags_z0h_(uint8_t result, uint8_t value)
    {
        FLAGS &= FLAG_C;
        FLAGS |= result ? 0 : FLAG_Z;
        FLAGS |= ((result & 0x0f) < (value & 0x0f)) ? FLAG_H : 0;
    }

    void CpuZ80::flags_z1h_(uint8_t result, uint8_t value)
    {
        FLAGS &= FLAG_C;
        FLAGS |= result ? 0 : FLAG_Z;
        FLAGS |= FLAG_N;
        FLAGS |= ((result & 0x0f) > (value & 0x0f)) ? FLAG_H : 0;
    }

    void CpuZ80::flags_z_0x(uint16_t result)
    {
        FLAGS &= FLAG_N | FLAG_C;
        FLAGS |= (result & 0xff) ? 0 : FLAG_Z;
        FLAGS |= (result & 0x100) ? FLAG_C : 0;
    }

    void CpuZ80::flags__11_()
    {
        FLAGS |= FLAG_N | FLAG_H;
    }

    void CpuZ80::flags__0hc(uint32_t result, uint16_t value1, uint16_t value2)
    {
        auto half = (value1 & 0x0fff) + (value2 & 0x0fff);
        FLAGS &= FLAG_Z;
        FLAGS |= (half >= 0x1000) ? FLAG_H : 0;
        FLAGS |= (result >= 0x10000) ? FLAG_C : 0;
    }

    void CpuZ80::flags_00hc(uint16_t value1, uint16_t value2)
    {
        auto result = (value1 & 0x00ff) + (value2 & 0x00ff);
        auto half = (value1 & 0x000f) + (value2 & 0x000f);
        FLAGS = 0;
        FLAGS |= (half >= 0x10) ? FLAG_H : 0;
        FLAGS |= (result >= 0x100) ? FLAG_C : 0;
    }

    void CpuZ80::flags_000c(uint8_t carry)
    {
        FLAGS = 0;
        FLAGS |= carry ? FLAG_C : 0;
    }

    void CpuZ80::flags_z00c(uint8_t result, uint8_t carry)
    {
        FLAGS = 0;
        FLAGS |= result ? 0 : FLAG_Z;
        FLAGS |= carry ? FLAG_C : 0;
    }

    void CpuZ80::flags_z01_(uint8_t result)
    {
        FLAGS &= FLAG_C;
        FLAGS |= result ? 0 : FLAG_Z;
        FLAGS |= FLAG_H;
    }

    // GMB 8bit - Loadcommands

    void CpuZ80::insn_ld(uint8_t& dest, uint8_t src)
    {
        dest = src;
    }

    void CpuZ80::insn_ld(addr& dest, uint8_t src)
    {
        write8(dest.value, src);
    }

    // GMB 16bit - Loadcommands

    void CpuZ80::insn_ld(uint16_t& dest, uint16_t src)
    {
        dest = src;
    }

    void CpuZ80::insn_ld(addr& dest, uint16_t src)
    {
        write16(dest.value, src);
    }

    void CpuZ80::insn_push(uint16_t src)
    {
        push16(src);
    }

    void CpuZ80::insn_pop(uint16_t& dest)
    {
        dest = pop16();
    }

    // GMB 8bit - Arithmetic / logical Commands

    void CpuZ80::insn_add(uint8_t src)
    {
        auto value = A;
        auto result = static_cast<uint16_t>(value) + src;
        A = result;
        flags_z0hc(result, value, src, 0);
    }

    void CpuZ80::insn_adc(uint8_t src)
    {
        uint8_t c_in = (FLAGS & FLAG_C) ? 1 : 0;
        auto value = A;
        auto result = static_cast<uint16_t>(value) + src + c_in;
        uint8_t c_out = (result >= 0x100) ? 1 : 0;
        A = result & 0xff;
        flags_z0hc(result, value, src, c_in);
    }

    void CpuZ80::insn_sub(uint8_t src)
    {
        auto value = A;
        auto result = static_cast<uint16_t>(value) - src;
        A = result & 0xff;
        flags_z1hc(result, value, src, 0);
    }

    void CpuZ80::insn_sbc(uint8_t src)
    {
        uint8_t c_in = (FLAGS & FLAG_C) ? 1 : 0;
        auto value = A;
        auto result = static_cast<uint16_t>(value) - src - c_in;
        A = result & 0xff;
        flags_z1hc(result, value, src, c_in);
    }

    void CpuZ80::insn_and(uint8_t src)
    {
        A &= src;
        flags_z010(A);
    }

    void CpuZ80::insn_xor(uint8_t src)
    {
        A ^= src;
        flags_z000(A);
    }

    void CpuZ80::insn_or(uint8_t src)
    {
        A |= src;
        flags_z000(A);
    }

    void CpuZ80::insn_cp(uint8_t src)
    {
        auto value = A;
        uint16_t result = static_cast<uint16_t>(value)-src;
        flags_z1hc(result, value, src, 0);
    }

    void CpuZ80::insn_inc(uint8_t& dest)
    {
        auto value = dest;
        auto result = value + 1;
        dest = result;
        flags_z0h_(result, value);
    }

    void CpuZ80::insn_inc(addr& dest)
    {
        auto value = read8(dest.value);
        auto result = value + 1;
        write8(dest.value, result);
        flags_z0h_(result, value);
    }

    void CpuZ80::insn_dec(uint8_t& dest)
    {
        auto value = dest;
        auto result = value - 1;
        dest = result;
        flags_z1h_(result, value);
    }

    void CpuZ80::insn_dec(addr& dest)
    {
        auto value = read8(dest.value);
        auto result = value - 1;
        write8(dest.value, result);
        flags_z1h_(result, value);
    }

    void CpuZ80::insn_daa()
    {
        auto result = static_cast<uint16_t>(A);
        if (FLAGS & FLAG_N)
        {
            if (FLAGS & FLAG_H)
                result = (result - 0x06) & 0xff;
            if (FLAGS & FLAG_C)
                result -= 0x60;
        }
        else
        {
            if ((FLAGS & FLAG_H) || ((result & 0x0f) > 0x09))
                result += 0x06;
            if ((FLAGS & FLAG_C) || (result > 0x9f))
                result += 0x60;
        }
        A = result & 0xff;
        flags_z_0x(result);
    }

    void CpuZ80::insn_cpl()
    {
        A = A ^ 0xff;
        flags__11_();
    }

    // GMB 16bit - Arithmetic/logical Commands

    void CpuZ80::insn_add(uint16_t src)
    {
        auto value = HL;
        auto result = static_cast<uint32_t>(value) + src;
        HL = result & 0xffff;
        flags__0hc(result, value, src);
    }

    void CpuZ80::insn_inc(uint16_t& dest)
    {
        ++dest;
    }

    void CpuZ80::insn_dec(uint16_t& dest)
    {
        --dest;
    }

    void CpuZ80::insn_add_sp(uint16_t src)
    {
        auto value = SP;
        auto result = value + src;
        SP = result;
        flags_00hc(value, src);
    }

    void CpuZ80::insn_ld_sp(uint16_t& dest, uint16_t src)
    {
        auto value = SP;
        auto result = value + src;
        dest = result;
        flags_00hc(value, src);
    }

    // GMB Rotate - and Shift - Commands

    void CpuZ80::insn_rlca()
    {
        auto value = A;
        auto bit_in = (value >> 7) & 0x01;
        auto bit_out = bit_in;
        auto result = (value << 1) | bit_in;
        A = result;
        flags_000c(bit_out);
    }

    void CpuZ80::insn_rla()
    {
        auto value = A;
        auto bit_in = (FLAGS & FLAG_C) ? 0x01 : 0x00;
        auto bit_out = (value >> 7) & 0x01;
        auto result = (value << 1) | bit_in;
        A = result;
        flags_000c(bit_out);
    }

    void CpuZ80::insn_rrca()
    {
        auto value = A;
        auto bit_in = (value << 7) & 0x80;
        auto bit_out = bit_in;
        auto result = (value >> 1) | bit_in;
        A = result;
        flags_000c(bit_out);
    }

    void CpuZ80::insn_rra()
    {
        auto value = A;
        auto carry_in = (FLAGS & FLAG_C) ? 0x80 : 0x00;
        auto carry_out = value & 0x01;
        auto result = (value >> 1) | carry_in;
        A = result;
        flags_000c(carry_out);
    }

    void CpuZ80::insn_rlc(uint8_t& dest)
    {
        auto value = dest;
        auto bit_in = (value >> 7) & 0x01;
        auto bit_out = bit_in;
        auto result = (value << 1) | bit_in;
        dest = result;
        flags_z00c(result, bit_out);
    }

    void CpuZ80::insn_rlc(addr& dest)
    {
        auto value = read8(dest.value);
        auto bit_in = (value >> 7) & 0x01;
        auto bit_out = bit_in;
        auto result = (value << 1) | bit_in;
        write8(dest.value, result);
        flags_z00c(result, bit_out);
    }

    void CpuZ80::insn_rl(uint8_t& dest)
    {
        auto value = dest;
        auto bit_in = (FLAGS & FLAG_C) ? 0x01 : 0x00;
        auto bit_out = (value >> 7) & 0x01;
        auto result = (value << 1) | bit_in;
        dest = result;
        flags_z00c(result, bit_out);
    }

    void CpuZ80::insn_rl(addr& dest)
    {
        auto value = read8(dest.value);
        auto bit_in = (FLAGS & FLAG_C) ? 0x01 : 0x00;
        auto bit_out = (value >> 7) & 0x01;
        auto result = (value << 1) | bit_in;
        write8(dest.value, result);
        flags_z00c(result, bit_out);
    }

    void CpuZ80::insn_rrc(uint8_t& dest)
    {
        auto value = dest;
        auto bit_in = (value << 7) & 0x80;
        auto bit_out = bit_in;
        auto result = (value >> 1) | bit_in;
        dest = result;
        flags_z00c(result, bit_out);
    }

    void CpuZ80::insn_rrc(addr& dest)
    {
        auto value = read8(dest.value);
        auto bit_in = (value << 7) & 0x80;
        auto bit_out = bit_in;
        auto result = (value >> 1) | bit_in;
        write8(dest.value, result);
        flags_z00c(result, bit_out);
    }

    void CpuZ80::insn_rr(uint8_t& dest)
    {
        auto value = dest;
        auto carry_in = (FLAGS & FLAG_C) ? 0x80 : 0x00;
        auto carry_out = value & 0x01;
        auto result = (value >> 1) | carry_in;
        dest = result;
        flags_z00c(result, carry_out);
    }

    void CpuZ80::insn_rr(addr& dest)
    {
        auto value = read8(dest.value);
        auto carry_in = (FLAGS & FLAG_C) ? 0x80 : 0x00;
        auto carry_out = value & 0x01;
        auto result = (value >> 1) | carry_in;
        write8(dest.value, result);
        flags_z00c(result, carry_out);
    }

    void CpuZ80::insn_sla(uint8_t& dest)
    {
        auto value = dest;
        auto bit_out = value & 0x80;
        auto result = value << 1;
        dest = result;
        flags_z00c(result, bit_out);
    }

    void CpuZ80::insn_sla(addr& dest)
    {
        auto value = read8(dest.value);
        auto bit_out = value & 0x80;
        auto result = value << 1;
        write8(dest.value, result);
        flags_z00c(result, bit_out);
    }

    void CpuZ80::insn_swap(uint8_t& dest)
    {
        uint8_t value = dest;
        uint8_t result = ((value >> 4) & 0x0f) | ((value << 4) & 0xf0);
        dest = result;
        flags_z000(result);
    }

    void CpuZ80::insn_swap(addr& dest)
    {
        uint8_t value = read8(dest.value);
        uint8_t result = ((value >> 4) & 0x0f) | ((value << 4) & 0xf0);
        write8(dest.value, result);
        flags_z000(result);
    }

    void CpuZ80::insn_sra(uint8_t& dest)
    {
        auto value = dest;
        auto bit_in = value & 0x80;
        auto bit_out = value & 0x01;
        auto result = (value >> 1) | bit_in;
        dest = result;
        flags_z00c(result, bit_out);
    }

    void CpuZ80::insn_sra(addr& dest)
    {
        uint8_t value = read8(dest.value);
        auto bit_in = value & 0x80;
        auto bit_out = value & 0x01;
        auto result = (value >> 1) | bit_in;
        write8(dest.value, result);
        flags_z00c(result, bit_out);
    }

    void CpuZ80::insn_srl(uint8_t& dest)
    {
        auto value = dest;
        auto bit_out = value & 0x01;
        auto result = value >> 1;
        dest = result;
        flags_z00c(result, bit_out);
    }

    void CpuZ80::insn_srl(addr& dest)
    {
        uint8_t value = read8(dest.value);
        auto bit_out = value & 0x01;
        auto result = value >> 1;
        write8(dest.value, result);
        flags_z00c(result, bit_out);
    }

    // GMB Singlebit Operation Commands

    void CpuZ80::insn_bit(uint8_t mask, uint8_t src)
    {
        auto result = mask & src;
        flags_z01_(result);
    }

    void CpuZ80::insn_set(uint8_t mask, uint8_t& dest)
    {
        dest |= mask;
    }

    void CpuZ80::insn_set(uint8_t mask, addr& dest)
    {
        auto result = read8(dest.value) | mask;
        write8(dest.value, result);
    }

    void CpuZ80::insn_res(uint8_t mask, uint8_t& dest)
    {
        dest &= ~mask;
    }

    void CpuZ80::insn_res(uint8_t mask, addr& dest)
    {
        auto result = read8(dest.value) & ~mask;
        write8(dest.value, result);
    }

    // GMB CPU-Controlcommands

    void CpuZ80::insn_ccf()
    {
        FLAGS &= FLAG_Z | FLAG_C;
        FLAGS ^= FLAG_C;
    }

    void CpuZ80::insn_scf()
    {
        FLAGS &= FLAG_Z;
        FLAGS |= FLAG_C;
    }

    void CpuZ80::insn_nop()
    {
    }

    void CpuZ80::insn_halt()
    {
        EMU_NOT_IMPLEMENTED();
    }

    void CpuZ80::insn_stop()
    {
        EMU_NOT_IMPLEMENTED();
    }

    void CpuZ80::insn_di()
    {
        setIME(false);
    }

    void CpuZ80::insn_ei()
    {
        setIME(true);
    }

    // GMB Jumpcommands

    void CpuZ80::insn_jp(uint16_t dest)
    {
        PC = dest;
    }

    void CpuZ80::insn_jp(bool cond, uint16_t dest)
    {
        if (cond)
        {
            mExecutedTicks += mTicksCond_jp;
            PC = dest;
        }
    }

    void CpuZ80::insn_jr(uint16_t dest)
    {
        PC = dest;
    }

    void CpuZ80::insn_jr(bool cond, uint16_t dest)
    {
        if (cond)
        {
            mExecutedTicks += mTicksCond_jr;
            PC = dest;
        }
    }

    void CpuZ80::insn_call(uint16_t dest)
    {
        push16(PC);
        PC = dest;
    }

    void CpuZ80::insn_call(bool cond, uint16_t dest)
    {
        if (cond)
        {
            mExecutedTicks += mTicksCond_call;
            push16(PC);
            PC = dest;
        }
    }

    void CpuZ80::insn_ret()
    {
        PC = pop16();
    }

    void CpuZ80::insn_ret(bool cond)
    {
        if (cond)
        {
            mExecutedTicks += mTicksCond_ret;
            PC = pop16();
        }
    }

    void CpuZ80::insn_reti()
    {
        PC = pop16();
        setIME(true);
    }

    void CpuZ80::insn_rst(uint8_t dest)
    {
        push16(PC);
        PC = static_cast<uint16_t>(dest);
    }

    void CpuZ80::insn_invalid()
    {
        EMU_NOT_IMPLEMENTED();
    }

    void CpuZ80::executeCB()
    {
        auto opcode = fetch8();
        mExecutedTicks += mTicksCB[opcode] & 7;
        switch (opcode)
        {
        case 0x00: insn_rlc(B); break;
        case 0x01: insn_rlc(C); break;
        case 0x02: insn_rlc(D); break;
        case 0x03: insn_rlc(E); break;
        case 0x04: insn_rlc(H); break;
        case 0x05: insn_rlc(L); break;
        case 0x06: insn_rlc(addr(HL)); break;
        case 0x07: insn_rlc(A); break;
        case 0x08: insn_rrc(B); break;
        case 0x09: insn_rrc(C); break;
        case 0x0a: insn_rrc(D); break;
        case 0x0b: insn_rrc(E); break;
        case 0x0c: insn_rrc(H); break;
        case 0x0d: insn_rrc(L); break;
        case 0x0e: insn_rrc(addr(HL)); break;
        case 0x0f: insn_rrc(A); break;
        case 0x10: insn_rl(B); break;
        case 0x11: insn_rl(C); break;
        case 0x12: insn_rl(D); break;
        case 0x13: insn_rl(E); break;
        case 0x14: insn_rl(H); break;
        case 0x15: insn_rl(L); break;
        case 0x16: insn_rl(addr(HL)); break;
        case 0x17: insn_rl(A); break;
        case 0x18: insn_rr(B); break;
        case 0x19: insn_rr(C); break;
        case 0x1a: insn_rr(D); break;
        case 0x1b: insn_rr(E); break;
        case 0x1c: insn_rr(H); break;
        case 0x1d: insn_rr(L); break;
        case 0x1e: insn_rr(addr(HL)); break;
        case 0x1f: insn_rr(A); break;
        case 0x20: insn_sla(B); break;
        case 0x21: insn_sla(C); break;
        case 0x22: insn_sla(D); break;
        case 0x23: insn_sla(E); break;
        case 0x24: insn_sla(H); break;
        case 0x25: insn_sla(L); break;
        case 0x26: insn_sla(addr(HL)); break;
        case 0x27: insn_sla(A); break;
        case 0x28: insn_sra(B); break;
        case 0x29: insn_sra(C); break;
        case 0x2a: insn_sra(D); break;
        case 0x2b: insn_sra(E); break;
        case 0x2c: insn_sra(H); break;
        case 0x2d: insn_sra(L); break;
        case 0x2e: insn_sra(addr(HL)); break;
        case 0x2f: insn_sra(A); break;
        case 0x30: insn_swap(B); break;
        case 0x31: insn_swap(C); break;
        case 0x32: insn_swap(D); break;
        case 0x33: insn_swap(E); break;
        case 0x34: insn_swap(H); break;
        case 0x35: insn_swap(L); break;
        case 0x36: insn_swap(addr(HL)); break;
        case 0x37: insn_swap(A); break;
        case 0x38: insn_srl(B); break;
        case 0x39: insn_srl(C); break;
        case 0x3a: insn_srl(D); break;
        case 0x3b: insn_srl(E); break;
        case 0x3c: insn_srl(H); break;
        case 0x3d: insn_srl(L); break;
        case 0x3e: insn_srl(addr(HL)); break;
        case 0x3f: insn_srl(A); break;
        case 0x40: insn_bit(1 << 0, B); break;
        case 0x41: insn_bit(1 << 0, C); break;
        case 0x42: insn_bit(1 << 0, D); break;
        case 0x43: insn_bit(1 << 0, E); break;
        case 0x44: insn_bit(1 << 0, H); break;
        case 0x45: insn_bit(1 << 0, L); break;
        case 0x46: insn_bit(1 << 0, read8(HL)); break;
        case 0x47: insn_bit(1 << 0, A); break;
        case 0x48: insn_bit(1 << 1, B); break;
        case 0x49: insn_bit(1 << 1, C); break;
        case 0x4a: insn_bit(1 << 1, D); break;
        case 0x4b: insn_bit(1 << 1, E); break;
        case 0x4c: insn_bit(1 << 1, H); break;
        case 0x4d: insn_bit(1 << 1, L); break;
        case 0x4e: insn_bit(1 << 1, read8(HL)); break;
        case 0x4f: insn_bit(1 << 1, A); break;
        case 0x50: insn_bit(1 << 2, B); break;
        case 0x51: insn_bit(1 << 2, C); break;
        case 0x52: insn_bit(1 << 2, D); break;
        case 0x53: insn_bit(1 << 2, E); break;
        case 0x54: insn_bit(1 << 2, H); break;
        case 0x55: insn_bit(1 << 2, L); break;
        case 0x56: insn_bit(1 << 2, read8(HL)); break;
        case 0x57: insn_bit(1 << 2, A); break;
        case 0x58: insn_bit(1 << 3, B); break;
        case 0x59: insn_bit(1 << 3, C); break;
        case 0x5a: insn_bit(1 << 3, D); break;
        case 0x5b: insn_bit(1 << 3, E); break;
        case 0x5c: insn_bit(1 << 3, H); break;
        case 0x5d: insn_bit(1 << 3, L); break;
        case 0x5e: insn_bit(1 << 3, read8(HL)); break;
        case 0x5f: insn_bit(1 << 3, A); break;
        case 0x60: insn_bit(1 << 4, B); break;
        case 0x61: insn_bit(1 << 4, C); break;
        case 0x62: insn_bit(1 << 4, D); break;
        case 0x63: insn_bit(1 << 4, E); break;
        case 0x64: insn_bit(1 << 4, H); break;
        case 0x65: insn_bit(1 << 4, L); break;
        case 0x66: insn_bit(1 << 4, read8(HL)); break;
        case 0x67: insn_bit(1 << 4, A); break;
        case 0x68: insn_bit(1 << 5, B); break;
        case 0x69: insn_bit(1 << 5, C); break;
        case 0x6a: insn_bit(1 << 5, D); break;
        case 0x6b: insn_bit(1 << 5, E); break;
        case 0x6c: insn_bit(1 << 5, H); break;
        case 0x6d: insn_bit(1 << 5, L); break;
        case 0x6e: insn_bit(1 << 5, read8(HL)); break;
        case 0x6f: insn_bit(1 << 5, A); break;
        case 0x70: insn_bit(1 << 6, B); break;
        case 0x71: insn_bit(1 << 6, C); break;
        case 0x72: insn_bit(1 << 6, D); break;
        case 0x73: insn_bit(1 << 6, E); break;
        case 0x74: insn_bit(1 << 6, H); break;
        case 0x75: insn_bit(1 << 6, L); break;
        case 0x76: insn_bit(1 << 6, read8(HL)); break;
        case 0x77: insn_bit(1 << 6, A); break;
        case 0x78: insn_bit(1 << 7, B); break;
        case 0x79: insn_bit(1 << 7, C); break;
        case 0x7a: insn_bit(1 << 7, D); break;
        case 0x7b: insn_bit(1 << 7, E); break;
        case 0x7c: insn_bit(1 << 7, H); break;
        case 0x7d: insn_bit(1 << 7, L); break;
        case 0x7e: insn_bit(1 << 7, read8(HL)); break;
        case 0x7f: insn_bit(1 << 7, A); break;
        case 0x80: insn_res(1 << 0, B); break;
        case 0x81: insn_res(1 << 0, C); break;
        case 0x82: insn_res(1 << 0, D); break;
        case 0x83: insn_res(1 << 0, E); break;
        case 0x84: insn_res(1 << 0, H); break;
        case 0x85: insn_res(1 << 0, L); break;
        case 0x86: insn_res(1 << 0, addr(HL)); break;
        case 0x87: insn_res(1 << 0, A); break;
        case 0x88: insn_res(1 << 1, B); break;
        case 0x89: insn_res(1 << 1, C); break;
        case 0x8a: insn_res(1 << 1, D); break;
        case 0x8b: insn_res(1 << 1, E); break;
        case 0x8c: insn_res(1 << 1, H); break;
        case 0x8d: insn_res(1 << 1, L); break;
        case 0x8e: insn_res(1 << 1, addr(HL)); break;
        case 0x8f: insn_res(1 << 1, A); break;
        case 0x90: insn_res(1 << 2, B); break;
        case 0x91: insn_res(1 << 2, C); break;
        case 0x92: insn_res(1 << 2, D); break;
        case 0x93: insn_res(1 << 2, E); break;
        case 0x94: insn_res(1 << 2, H); break;
        case 0x95: insn_res(1 << 2, L); break;
        case 0x96: insn_res(1 << 2, addr(HL)); break;
        case 0x97: insn_res(1 << 2, A); break;
        case 0x98: insn_res(1 << 3, B); break;
        case 0x99: insn_res(1 << 3, C); break;
        case 0x9a: insn_res(1 << 3, D); break;
        case 0x9b: insn_res(1 << 3, E); break;
        case 0x9c: insn_res(1 << 3, H); break;
        case 0x9d: insn_res(1 << 3, L); break;
        case 0x9e: insn_res(1 << 3, addr(HL)); break;
        case 0x9f: insn_res(1 << 3, A); break;
        case 0xa0: insn_res(1 << 4, B); break;
        case 0xa1: insn_res(1 << 4, C); break;
        case 0xa2: insn_res(1 << 4, D); break;
        case 0xa3: insn_res(1 << 4, E); break;
        case 0xa4: insn_res(1 << 4, H); break;
        case 0xa5: insn_res(1 << 4, L); break;
        case 0xa6: insn_res(1 << 4, addr(HL)); break;
        case 0xa7: insn_res(1 << 4, A); break;
        case 0xa8: insn_res(1 << 5, B); break;
        case 0xa9: insn_res(1 << 5, C); break;
        case 0xaa: insn_res(1 << 5, D); break;
        case 0xab: insn_res(1 << 5, E); break;
        case 0xac: insn_res(1 << 5, H); break;
        case 0xad: insn_res(1 << 5, L); break;
        case 0xae: insn_res(1 << 5, addr(HL)); break;
        case 0xaf: insn_res(1 << 5, A); break;
        case 0xb0: insn_res(1 << 6, B); break;
        case 0xb1: insn_res(1 << 6, C); break;
        case 0xb2: insn_res(1 << 6, D); break;
        case 0xb3: insn_res(1 << 6, E); break;
        case 0xb4: insn_res(1 << 6, H); break;
        case 0xb5: insn_res(1 << 6, L); break;
        case 0xb6: insn_res(1 << 6, addr(HL)); break;
        case 0xb7: insn_res(1 << 6, A); break;
        case 0xb8: insn_res(1 << 7, B); break;
        case 0xb9: insn_res(1 << 7, C); break;
        case 0xba: insn_res(1 << 7, D); break;
        case 0xbb: insn_res(1 << 7, E); break;
        case 0xbc: insn_res(1 << 7, H); break;
        case 0xbd: insn_res(1 << 7, L); break;
        case 0xbe: insn_res(1 << 7, addr(HL)); break;
        case 0xbf: insn_res(1 << 7, A); break;
        case 0xc0: insn_set(1 << 0, B); break;
        case 0xc1: insn_set(1 << 0, C); break;
        case 0xc2: insn_set(1 << 0, D); break;
        case 0xc3: insn_set(1 << 0, E); break;
        case 0xc4: insn_set(1 << 0, H); break;
        case 0xc5: insn_set(1 << 0, L); break;
        case 0xc6: insn_set(1 << 0, addr(HL)); break;
        case 0xc7: insn_set(1 << 0, A); break;
        case 0xc8: insn_set(1 << 1, B); break;
        case 0xc9: insn_set(1 << 1, C); break;
        case 0xca: insn_set(1 << 1, D); break;
        case 0xcb: insn_set(1 << 1, E); break;
        case 0xcc: insn_set(1 << 1, H); break;
        case 0xcd: insn_set(1 << 1, L); break;
        case 0xce: insn_set(1 << 1, addr(HL)); break;
        case 0xcf: insn_set(1 << 1, A); break;
        case 0xd0: insn_set(1 << 2, B); break;
        case 0xd1: insn_set(1 << 2, C); break;
        case 0xd2: insn_set(1 << 2, D); break;
        case 0xd3: insn_set(1 << 2, E); break;
        case 0xd4: insn_set(1 << 2, H); break;
        case 0xd5: insn_set(1 << 2, L); break;
        case 0xd6: insn_set(1 << 2, addr(HL)); break;
        case 0xd7: insn_set(1 << 2, A); break;
        case 0xd8: insn_set(1 << 3, B); break;
        case 0xd9: insn_set(1 << 3, C); break;
        case 0xda: insn_set(1 << 3, D); break;
        case 0xdb: insn_set(1 << 3, E); break;
        case 0xdc: insn_set(1 << 3, H); break;
        case 0xdd: insn_set(1 << 3, L); break;
        case 0xde: insn_set(1 << 3, addr(HL)); break;
        case 0xdf: insn_set(1 << 3, A); break;
        case 0xe0: insn_set(1 << 4, B); break;
        case 0xe1: insn_set(1 << 4, C); break;
        case 0xe2: insn_set(1 << 4, D); break;
        case 0xe3: insn_set(1 << 4, E); break;
        case 0xe4: insn_set(1 << 4, H); break;
        case 0xe5: insn_set(1 << 4, L); break;
        case 0xe6: insn_set(1 << 4, addr(HL)); break;
        case 0xe7: insn_set(1 << 4, A); break;
        case 0xe8: insn_set(1 << 5, B); break;
        case 0xe9: insn_set(1 << 5, C); break;
        case 0xea: insn_set(1 << 5, D); break;
        case 0xeb: insn_set(1 << 5, E); break;
        case 0xec: insn_set(1 << 5, H); break;
        case 0xed: insn_set(1 << 5, L); break;
        case 0xee: insn_set(1 << 5, addr(HL)); break;
        case 0xef: insn_set(1 << 5, A); break;
        case 0xf0: insn_set(1 << 6, B); break;
        case 0xf1: insn_set(1 << 6, C); break;
        case 0xf2: insn_set(1 << 6, D); break;
        case 0xf3: insn_set(1 << 6, E); break;
        case 0xf4: insn_set(1 << 6, H); break;
        case 0xf5: insn_set(1 << 6, L); break;
        case 0xf6: insn_set(1 << 6, addr(HL)); break;
        case 0xf7: insn_set(1 << 6, A); break;
        case 0xf8: insn_set(1 << 7, B); break;
        case 0xf9: insn_set(1 << 7, C); break;
        case 0xfa: insn_set(1 << 7, D); break;
        case 0xfb: insn_set(1 << 7, E); break;
        case 0xfc: insn_set(1 << 7, H); break;
        case 0xfd: insn_set(1 << 7, L); break;
        case 0xfe: insn_set(1 << 7, addr(HL)); break;
        case 0xff: insn_set(1 << 7, A); break;
        default: break;
        }
    }

    void CpuZ80::executeMain()
    {
        auto opcode = fetch8();
        mExecutedTicks += mTicksMain[opcode];
        switch (opcode)
        {
        case 0x00: insn_nop(); break;
        case 0x01: insn_ld(BC, fetch16()); break;
        case 0x02: insn_ld(addr(BC), A); break;
        case 0x03: insn_inc(BC); break;
        case 0x04: insn_inc(B); break;
        case 0x05: insn_dec(B); break;
        case 0x06: insn_ld(B, fetch8()); break;
        case 0x07: insn_rlca(); break;
        case 0x08: insn_ld(addr(fetch16()), SP); break;
        case 0x09: insn_add(BC); break;
        case 0x0a: insn_ld(A, read8(BC)); break;
        case 0x0b: insn_dec(BC); break;
        case 0x0c: insn_inc(C); break;
        case 0x0d: insn_dec(C); break;
        case 0x0e: insn_ld(C, fetch8()); break;
        case 0x0f: insn_rrca(); break;
        case 0x10: fetch8(); insn_stop(); break;
        case 0x11: insn_ld(DE, fetch16()); break;
        case 0x12: insn_ld(addr(DE), A); break;
        case 0x13: insn_inc(DE); break;
        case 0x14: insn_inc(D); break;
        case 0x15: insn_dec(D); break;
        case 0x16: insn_ld(D, fetch8()); break;
        case 0x17: insn_rla(); break;
        case 0x18: insn_jr(fetchPC()); break;
        case 0x19: insn_add(DE); break;
        case 0x1a: insn_ld(A, read8(DE)); break;
        case 0x1b: insn_dec(DE); break;
        case 0x1c: insn_inc(E); break;
        case 0x1d: insn_dec(E); break;
        case 0x1e: insn_ld(E, fetch8()); break;
        case 0x1f: insn_rra(); break;
        case 0x20: insn_jr((FLAGS & FLAG_Z) == 0, fetchPC()); break;
        case 0x21: insn_ld(HL, fetch16()); break;
        case 0x22: insn_ld(addr(HL++), A); break;
        case 0x23: insn_inc(HL); break;
        case 0x24: insn_inc(H); break;
        case 0x25: insn_dec(H); break;
        case 0x26: insn_ld(H, fetch8()); break;
        case 0x27: insn_daa(); break;
        case 0x28: insn_jr((FLAGS & FLAG_Z) != 0, fetchPC()); break;
        case 0x29: insn_add(HL); break;
        case 0x2a: insn_ld(A, read8(HL++)); break;
        case 0x2b: insn_dec(HL); break;
        case 0x2c: insn_inc(L); break;
        case 0x2d: insn_dec(L); break;
        case 0x2e: insn_ld(L, fetch8()); break;
        case 0x2f: insn_cpl(); break;
        case 0x30: insn_jr((FLAGS & FLAG_C) == 0, fetchPC()); break;
        case 0x31: insn_ld(SP, fetch16()); break;
        case 0x32: insn_ld(addr(HL--), A); break;
        case 0x33: insn_inc(SP); break;
        case 0x34: insn_inc(addr(HL)); break;
        case 0x35: insn_dec(addr(HL)); break;
        case 0x36: insn_ld(addr(HL), fetch8()); break;
        case 0x37: insn_scf(); break;
        case 0x38: insn_jr((FLAGS & FLAG_C) != 0, fetchPC()); break;
        case 0x39: insn_add(SP); break;
        case 0x3a: insn_ld(A, read8(HL--)); break;
        case 0x3b: insn_dec(SP); break;
        case 0x3c: insn_inc(A); break;
        case 0x3d: insn_dec(A); break;
        case 0x3e: insn_ld(A, fetch8()); break;
        case 0x3f: insn_ccf(); break;
        case 0x40: insn_ld(B, B); break;
        case 0x41: insn_ld(B, C); break;
        case 0x42: insn_ld(B, D); break;
        case 0x43: insn_ld(B, E); break;
        case 0x44: insn_ld(B, H); break;
        case 0x45: insn_ld(B, L); break;
        case 0x46: insn_ld(B, read8(HL)); break;
        case 0x47: insn_ld(B, A); break;
        case 0x48: insn_ld(C, B); break;
        case 0x49: insn_ld(C, C); break;
        case 0x4a: insn_ld(C, D); break;
        case 0x4b: insn_ld(C, E); break;
        case 0x4c: insn_ld(C, H); break;
        case 0x4d: insn_ld(C, L); break;
        case 0x4e: insn_ld(C, read8(HL)); break;
        case 0x4f: insn_ld(C, A); break;
        case 0x50: insn_ld(D, B); break;
        case 0x51: insn_ld(D, C); break;
        case 0x52: insn_ld(D, D); break;
        case 0x53: insn_ld(D, E); break;
        case 0x54: insn_ld(D, H); break;
        case 0x55: insn_ld(D, L); break;
        case 0x56: insn_ld(D, read8(HL)); break;
        case 0x57: insn_ld(D, A); break;
        case 0x58: insn_ld(E, B); break;
        case 0x59: insn_ld(E, C); break;
        case 0x5a: insn_ld(E, D); break;
        case 0x5b: insn_ld(E, E); break;
        case 0x5c: insn_ld(E, H); break;
        case 0x5d: insn_ld(E, L); break;
        case 0x5e: insn_ld(E, read8(HL)); break;
        case 0x5f: insn_ld(E, A); break;
        case 0x60: insn_ld(H, B); break;
        case 0x61: insn_ld(H, C); break;
        case 0x62: insn_ld(H, D); break;
        case 0x63: insn_ld(H, E); break;
        case 0x64: insn_ld(H, H); break;
        case 0x65: insn_ld(H, L); break;
        case 0x66: insn_ld(H, read8(HL)); break;
        case 0x67: insn_ld(H, A); break;
        case 0x68: insn_ld(L, B); break;
        case 0x69: insn_ld(L, C); break;
        case 0x6a: insn_ld(L, D); break;
        case 0x6b: insn_ld(L, E); break;
        case 0x6c: insn_ld(L, H); break;
        case 0x6d: insn_ld(L, L); break;
        case 0x6e: insn_ld(L, read8(HL)); break;
        case 0x6f: insn_ld(L, A); break;
        case 0x70: insn_ld(addr(HL), B); break;
        case 0x71: insn_ld(addr(HL), C); break;
        case 0x72: insn_ld(addr(HL), D); break;
        case 0x73: insn_ld(addr(HL), E); break;
        case 0x74: insn_ld(addr(HL), H); break;
        case 0x75: insn_ld(addr(HL), L); break;
        case 0x76: insn_halt(); break;
        case 0x77: insn_ld(addr(HL), A); break;
        case 0x78: insn_ld(A, B); break;
        case 0x79: insn_ld(A, C); break;
        case 0x7a: insn_ld(A, D); break;
        case 0x7b: insn_ld(A, E); break;
        case 0x7c: insn_ld(A, H); break;
        case 0x7d: insn_ld(A, L); break;
        case 0x7e: insn_ld(A, read8(HL)); break;
        case 0x7f: insn_ld(A, A); break;
        case 0x80: insn_add(B); break;
        case 0x81: insn_add(C); break;
        case 0x82: insn_add(D); break;
        case 0x83: insn_add(E); break;
        case 0x84: insn_add(H); break;
        case 0x85: insn_add(L); break;
        case 0x86: insn_add(read8(HL)); break;
        case 0x87: insn_add(A); break;
        case 0x88: insn_adc(B); break;
        case 0x89: insn_adc(C); break;
        case 0x8a: insn_adc(D); break;
        case 0x8b: insn_adc(E); break;
        case 0x8c: insn_adc(H); break;
        case 0x8d: insn_adc(L); break;
        case 0x8e: insn_adc(read8(HL)); break;
        case 0x8f: insn_adc(A); break;
        case 0x90: insn_sub(B); break;
        case 0x91: insn_sub(C); break;
        case 0x92: insn_sub(D); break;
        case 0x93: insn_sub(E); break;
        case 0x94: insn_sub(H); break;
        case 0x95: insn_sub(L); break;
        case 0x96: insn_sub(read8(HL)); break;
        case 0x97: insn_sub(A); break;
        case 0x98: insn_sbc(B); break;
        case 0x99: insn_sbc(C); break;
        case 0x9a: insn_sbc(D); break;
        case 0x9b: insn_sbc(E); break;
        case 0x9c: insn_sbc(H); break;
        case 0x9d: insn_sbc(L); break;
        case 0x9e: insn_sbc(read8(HL)); break;
        case 0x9f: insn_sbc(A); break;
        case 0xa0: insn_and(B); break;
        case 0xa1: insn_and(C); break;
        case 0xa2: insn_and(D); break;
        case 0xa3: insn_and(E); break;
        case 0xa4: insn_and(H); break;
        case 0xa5: insn_and(L); break;
        case 0xa6: insn_and(read8(HL)); break;
        case 0xa7: insn_and(A); break;
        case 0xa8: insn_xor(B); break;
        case 0xa9: insn_xor(C); break;
        case 0xaa: insn_xor(D); break;
        case 0xab: insn_xor(E); break;
        case 0xac: insn_xor(H); break;
        case 0xad: insn_xor(L); break;
        case 0xae: insn_xor(read8(HL)); break;
        case 0xaf: insn_xor(A); break;
        case 0xb0: insn_or(B); break;
        case 0xb1: insn_or(C); break;
        case 0xb2: insn_or(D); break;
        case 0xb3: insn_or(E); break;
        case 0xb4: insn_or(H); break;
        case 0xb5: insn_or(L); break;
        case 0xb6: insn_or(read8(HL)); break;
        case 0xb7: insn_or(A); break;
        case 0xb8: insn_cp(B); break;
        case 0xb9: insn_cp(C); break;
        case 0xba: insn_cp(D); break;
        case 0xbb: insn_cp(E); break;
        case 0xbc: insn_cp(H); break;
        case 0xbd: insn_cp(L); break;
        case 0xbe: insn_cp(read8(HL)); break;
        case 0xbf: insn_cp(A); break;
        case 0xc0: insn_ret((FLAGS & FLAG_Z) == 0); break;
        case 0xc1: insn_pop(BC); break;
        case 0xc2: insn_jp((FLAGS & FLAG_Z) == 0, fetch16()); break;
        case 0xc3: insn_jp(fetch16()); break;
        case 0xc4: insn_call((FLAGS & FLAG_Z) == 0, fetch16()); break;
        case 0xc5: insn_push(BC); break;
        case 0xc6: insn_add(fetch8()); break;
        case 0xc7: insn_rst(0x00); break;
        case 0xc8: insn_ret((FLAGS & FLAG_Z) != 0); break;
        case 0xc9: insn_ret(); break;
        case 0xca: insn_jp((FLAGS & FLAG_Z) != 0, fetch16()); break;
        case 0xcb: executeCB(); break;
        case 0xcc: insn_call((FLAGS & FLAG_Z) != 0, fetch16()); break;
        case 0xcd: insn_call(fetch16()); break;
        case 0xce: insn_adc(fetch8()); break;
        case 0xcf: insn_rst(0x08); break;
        case 0xd0: insn_ret((FLAGS & FLAG_C) == 0); break;
        case 0xd1: insn_pop(DE); break;
        case 0xd2: insn_jp((FLAGS & FLAG_C) == 0, fetch16()); break;
        case 0xd3: insn_invalid(); break;
        case 0xd4: insn_call((FLAGS & FLAG_C) == 0, fetch16()); break;
        case 0xd5: insn_push(DE); break;
        case 0xd6: insn_sub(fetch8()); break;
        case 0xd7: insn_rst(0x10); break;
        case 0xd8: insn_ret((FLAGS & FLAG_C) != 0); break;
        case 0xd9: insn_reti(); break;
        case 0xda: insn_jp((FLAGS & FLAG_C) != 0, fetch16()); break;
        case 0xdb: insn_invalid(); break;
        case 0xdc: insn_call((FLAGS & FLAG_C) != 0, fetch16()); break;
        case 0xdd: insn_invalid(); break;
        case 0xde: insn_sbc(fetch8()); break;
        case 0xdf: insn_rst(0x18); break;
        case 0xe0: insn_ld(addr(0xff00 + fetch8()), A); break;
        case 0xe1: insn_pop(HL); break;
        case 0xe2: insn_ld(addr(0xff00 + C), A); break;
        case 0xe3: insn_invalid(); break;
        case 0xe4: insn_invalid(); break;
        case 0xe5: insn_push(HL); break;
        case 0xe6: insn_and(fetch8()); break;
        case 0xe7: insn_rst(0x20); break;
        case 0xe8: insn_add_sp(fetchSigned8()); break;
        case 0xe9: insn_jp((HL)); break;
        case 0xea: insn_ld(addr(fetch16()), A); break;
        case 0xeb: insn_invalid(); break;
        case 0xec: insn_invalid(); break;
        case 0xed: insn_invalid(); break;
        case 0xee: insn_xor(fetch8()); break;
        case 0xef: insn_rst(0x28); break;
        case 0xf0: insn_ld(A, read8(0xff00 + fetch8())); break;
        case 0xf1: insn_pop(AF); FLAGS &= FLAG_ZNHC; break;
        case 0xf2: insn_ld(A, read8(0xff00 + C)); break;
        case 0xf3: insn_di(); break;
        case 0xf4: insn_invalid(); break;
        case 0xf5: insn_push(AF); break;
        case 0xf6: insn_or(fetch8()); break;
        case 0xf7: insn_rst(0x30); break;
        case 0xf8: insn_ld_sp(HL, fetchSigned8()); break;
        case 0xf9: insn_ld(SP, HL); break;
        case 0xfa: insn_ld(A, read8(fetch16())); break;
        case 0xfb: insn_ei(); break;
        case 0xfc: insn_invalid(); break;
        case 0xfd: insn_invalid(); break;
        case 0xfe: insn_cp(fetch8()); break;
        case 0xff: insn_rst(0x38); break;
        default: break;
        }
    }

    void CpuZ80::trace()
    {
#if 0
        //static FILE* log = fopen("..\\gb.log", "w");
        static FILE* log = nullptr;
        static int traceStart = 0x0000;
        static int traceBreak = 0x0000;
        static int traceCount = 0;
        ++traceCount;
        if (log && (traceCount >= traceStart))
        {
            char temp[32];
            auto nextPC = disassemble(temp, sizeof(temp), PC);
            uint16_t byteCount = nextPC - PC;
            char temp2[16];
            char* temp2Pos = temp2;
            for (uint16_t offset = 0; offset < byteCount; ++offset)
                temp2Pos += sprintf(temp2Pos, "%02X ", read8(PC + offset));
            fprintf(log, "%04X  %-9s %-20s  AF:%04X BC:%04X DE:%04X HL:%04X SP:%04X\n",
                PC, temp2, temp, AF, BC, DE, HL, SP);
        }

        if (traceCount == traceBreak)
        {
            traceBreak = traceBreak;
        }
#endif
    }

    void CpuZ80::execute()
    {
        while (mExecutedTicks < mDesiredTicks)
        {
            trace();
            executeMain();
        }
    }

    uint16_t CpuZ80::disassemble(char* buffer, size_t size, uint16_t addr)
    {
        // Decode
        auto pc = addr;
        auto data = peek8(pc);
        auto insnType = static_cast<INSN_TYPE>(insnTypeMain[data]);
        auto addrMode = static_cast<ADDR_MODE>(addrModeMain[data]);
        if (insnType == INSN_PREFIX)
        {
            data = peek8(pc);
            insnType = static_cast<INSN_TYPE>(insnTypeCB[data >> 3]);
            const auto& addrModeTable = (data > 0x40) ? addrModeCB2 : addrModeCB1;
            addrMode = static_cast<ADDR_MODE>(addrModeTable[data & 7]);
        }
        auto opcode = insnName[insnType];

        // Format
        char temp[32];
        char* text = temp;
        text += sprintf(text, (addrMode == ADDR_NONE) ? "%s" : "%-4s ", opcode);
        auto format = addrModeFormat[addrMode];
        switch (addrMode)
        {
        case ADDR_MEM_HL_D8:
        case ADDR_MEM_A8_A:
        case ADDR_A_MEM_A8:
        case ADDR_A_D8:
        case ADDR_B_D8:
        case ADDR_C_D8:
        case ADDR_D_D8:
        case ADDR_E_D8:
        case ADDR_H_D8:
        case ADDR_L_D8:
        case ADDR_D8:
            text += sprintf(text, format, peek8(pc));
            break;

        case ADDR_MEM_A16_A:
        case ADDR_MEM_A16_SP:
        case ADDR_A_MEM_A16:
        case ADDR_BC_D16:
        case ADDR_C_A16:
        case ADDR_DE_D16:
        case ADDR_HL_D16:
        case ADDR_NC_A16:
        case ADDR_NZ_A16:
        case ADDR_SP_D16:
        case ADDR_Z_A16:
        case ADDR_A16:
            text += sprintf(text, format, peek16(pc));
            break;

        case ADDR_RST:
            text += sprintf(text, format, data & 0x38);
            break;

        case ADDR_C_R8:
        case ADDR_NC_R8:
        case ADDR_NZ_R8:
        case ADDR_Z_R8:
        case ADDR_R8:
            text += sprintf(text, format, static_cast<uint16_t>(peekSigned8(pc) + pc));
            break;

        case ADDR_HL_SP_INC_R8:
        case ADDR_SP_R8:
            text += sprintf(text, format, peekSigned8(pc));
            break;

        case ADDR_CB2_B:
        case ADDR_CB2_C:
        case ADDR_CB2_D:
        case ADDR_CB2_E:
        case ADDR_CB2_H:
        case ADDR_CB2_L:
        case ADDR_CB2_MEM_HL:
        case ADDR_CB2_A:
            text += sprintf(text, format, (data >> 3) & 7);
            break;

        default:
            text += sprintf(text, format);
        }
        if (size-- > 0)
            strncpy(buffer, temp, size);
        buffer[size] = 0;

        return pc;
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
    }

    void CpuZ80::setIME(bool enable)
    {
        if (enable)
        {
            if (!IME)
            {
                IME = 1;
                for (auto listener : mInterruptListeners)
                    listener->onInterruptEnable(mExecutedTicks);
            }
        }
        else
        {
            if (IME)
            {
                IME = 0;
                for (auto listener : mInterruptListeners)
                    listener->onInterruptDisable(mExecutedTicks);
            }
        }
    }

    void CpuZ80::addInterruptListener(IInterruptListener& listener)
    {
        mInterruptListeners.push_back(&listener);
        if (IME)
            listener.onInterruptEnable(mExecutedTicks);
        else
            listener.onInterruptDisable(mExecutedTicks);
    }

    void CpuZ80::removeInterruptListener(IInterruptListener& listener)
    {
        auto item = std::find(mInterruptListeners.begin(), mInterruptListeners.end(), &listener);
        if (item != mInterruptListeners.end())
        {
            if (IME)
                (*item)->onInterruptDisable(mExecutedTicks);
            mInterruptListeners.erase(item);
        }
    }
}
