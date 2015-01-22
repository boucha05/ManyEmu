#include "Cpu6502.h"
#include "MemoryBus.h"
#include <assert.h>
#include <memory.h>
#include <stdio.h>

namespace
{
    static const uint16_t ADDR_VECTOR_NMI = 0xfffa;
    static const uint16_t ADDR_VECTOR_RESET = 0xfffc;
    static const uint16_t ADDR_VECTOR_IRQ = 0xfffe;

    static const uint8_t STATUS_N = 0x80;
    static const uint8_t STATUS_V = 0x40;
    static const uint8_t STATUS_RESERVED1 = 0x20;
    static const uint8_t STATUS_RESERVED0 = 0x10;
    static const uint8_t STATUS_D = 0x08;
    static const uint8_t STATUS_I = 0x04;
    static const uint8_t STATUS_Z = 0x02;
    static const uint8_t STATUS_C = 0x01;

    // http://nesdev.com/6502.txt
    /*  00        01         02        03        04        05        06        07        08        09        0a        0b        0c        0d        0e        0f
    00  BRK_impl, ORA_indx,  NOP_impl, NOP_impl, NOP_impl, ORA_zpg,  ASL_zpg,  NOP_impl, PHP_impl, ORA_imm,  ASL_acc,  NOP_impl, NOP_impl, ORA_abs,  ASL_abs,  NOP_impl,
    10  BPL_rel,  ORA_indy,  NOP_impl, NOP_impl, NOP_impl, ORA_zpgx, ASL_zpgx, NOP_impl, CLC_impl, ORA_absy, NOP_impl, NOP_impl, NOP_impl, ORA_absx, ASL_absx, NOP_impl,
    20  JSR_abs,  AND_indx,  NOP_impl, NOP_impl, BIT_zpg,  AND_zpg,  ROL_zpg,  NOP_impl, PLP_impl, AND_imm,  ROL_acc,  NOP_impl, BIT_abs,  AND_abs,  ROL_abs,  NOP_impl,
    30  BMI_rel,  AND_indy,  NOP_impl, NOP_impl, NOP_impl, AND_zpgx, ROL_zpgx, NOP_impl, SEC_impl, AND_absy, NOP_impl, NOP_impl, NOP_impl, AND_absx, ROL_absx, NOP_impl,
    40  RTI_impl, EOR_indx,  NOP_impl, NOP_impl, NOP_impl, EOR_zpg,  LSR_zpg,  NOP_impl, PHA_impl, EOR_imm,  LSR_acc,  NOP_impl, JMP_abs,  EOR_abs,  LSR_abs,  NOP_impl,
    50  BVC_rel,  EOR_indy,  NOP_impl, NOP_impl, NOP_impl, EOR_zpgx, LSR_zpgx, NOP_impl, CLI_impl, EOR_absy, NOP_impl, NOP_impl, NOP_impl, EOR_absx, LSR_absx, NOP_impl,
    60  RTS_impl, ADC_indx,  NOP_impl, NOP_impl, NOP_impl, ADC_zpg,  ROR_zpg,  NOP_impl, PLA_impl, ADC_imm,  ROR_acc,  NOP_impl, JMP_ind,  ADC_abs,  ROR_abs,  NOP_impl,
    70  BVS_rel,  ADC_indy,  NOP_impl, NOP_impl, NOP_impl, ADC_zpgx, ROR_zpgx, NOP_impl, SEI_impl, ADC_absy, NOP_impl, NOP_impl, NOP_impl, ADC_absx, ROR_absx, NOP_impl,
    80  NOP_impl, STA_indx,  NOP_impl, NOP_impl, STY_zpg,  STA_zpg,  STX_zpg,  NOP_impl, DEY_impl, NOP_impl, TXA_impl, NOP_impl, STY_abs,  STA_abs,  STX_abs,  NOP_impl,
    90  BCC_rel,  STA_indy,  NOP_impl, NOP_impl, STY_zpgx, STA_zpgx, STX_zpgy, NOP_impl, TYA_impl, STA_absy, TXS_impl, NOP_impl, NOP_impl, STA_absx, NOP_impl, NOP_impl,
    a0  LDY_imm,  LDA_indx,  LDX_imm,  NOP_impl, LDY_zpg,  LDA_zpg,  LDX_zpg,  NOP_impl, TAY_impl, LDA_imm,  TAX_impl, NOP_impl, LDY_abs,  LDA_abs,  LDX_abs,  NOP_impl,
    b0  BCS_rel,  LDA_indy,  NOP_impl, NOP_impl, LDY_zpgx, LDA_zpgx, LDX_zpgy, NOP_impl, CLV_impl, LDA_absy, TSX_impl, NOP_impl, LDY_absx, LDA_absx, LDX_absy, NOP_impl,
    c0  CPY_imm,  CMP_indx,  NOP_impl, NOP_impl, CPY_zpg,  CMP_zpg,  DEC_zpg,  NOP_impl, INY_impl, CMP_imm,  DEX_impl, NOP_impl, CPY_abs,  CMP_abs,  DEC_abs,  NOP_impl,
    d0  BNE_rel,  CMP_indy,  NOP_impl, NOP_impl, NOP_impl, CMP_zpgx, DEC_zpgx, NOP_impl, CLD_impl, CMP_absy, NOP_impl, NOP_impl, NOP_impl, CMP_absx, DEC_absx, NOP_impl,
    e0  CPX_imm,  SBC_indx,  NOP_impl, NOP_impl, CPX_zpg,  SBC_zpg,  INC_zpg,  NOP_impl, INX_impl, SBC_imm,  NOP_impl, NOP_impl, CPX_abs,  SBC_abs,  INC_abs,  NOP_impl,
    f0  BEQ_rel,  SBC_indy,  NOP_impl, NOP_impl, NOP_impl, SBC_zpgx, INC_zpgx, NOP_impl, SED_impl, SBC_absy, NOP_impl, NOP_impl, NOP_impl, SBC_absx, INC_absx, NOP_impl,
    */

    void NOT_IMPLEMENTED_ADDR_MODE(const char* mode)
    {
        printf("Address mode %s not implemented\n", mode);
        assert(false);
    }

    void NOT_IMPLEMENTED(const char* insn)
    {
        printf("Instruction %s not implemented\n", insn);
        assert(false);
    }

    inline uint8_t read8(CPU_STATE& state, uint16_t addr)
    {
        return memory_bus_read8(*state.bus, addr);
    }

    inline void write8(CPU_STATE& state, uint16_t addr, uint8_t value)
    {
        memory_bus_write8(*state.bus, addr, value);
    }

    inline uint16_t read16(CPU_STATE& state, uint16_t addr)
    {
        uint8_t lo = read8(state, addr);
        uint8_t hi = read8(state, addr + 1);
        uint16_t value = static_cast<uint16_t>(lo) | (static_cast<uint16_t>(hi) << 8);
        return value;
    }

    inline uint8_t fetch8(CPU_STATE& state)
    {
        return read8(state, state.pc++);
    }

    inline uint16_t fetch16(CPU_STATE& state)
    {
        uint8_t lo = fetch8(state);
        uint8_t hi = fetch8(state);
        uint16_t addr = static_cast<uint16_t>(lo) | (static_cast<uint16_t>(hi) << 8);
        return addr;
    }

    ///////////////////////////////////////////////////////////////////////////

    inline uint16_t addr_abs(CPU_STATE& state)
    {
        uint16_t addr = fetch16(state);
        return addr;
    }

    inline uint16_t addr_zpg(CPU_STATE& state)
    {
        NOT_IMPLEMENTED_ADDR_MODE("zpg");
        uint16_t addr = static_cast<uint16_t>(fetch8(state));
        return addr;
    }

    inline uint16_t addr_absx(CPU_STATE& state, int32_t clk = 0)
    {
        NOT_IMPLEMENTED_ADDR_MODE("absx");
        uint16_t addr = fetch16(state) + static_cast<uint16_t>(state.x);
        if (static_cast<uint8_t>(addr) < state.x)
            state.clk -= clk;
        return addr;
    }

    inline uint16_t addr_absy(CPU_STATE& state)
    {
        NOT_IMPLEMENTED_ADDR_MODE("absy");
        uint16_t addr = fetch16(state) + static_cast<uint16_t>(state.y);
        return addr;
    }

    inline uint16_t addr_zpgx(CPU_STATE& state)
    {
        NOT_IMPLEMENTED_ADDR_MODE("zpgx");
        uint16_t addr = static_cast<uint16_t>(fetch8(state) + state.x);
        return addr;
    }

    inline uint16_t addr_zpgy(CPU_STATE& state)
    {
        NOT_IMPLEMENTED_ADDR_MODE("zpgy");
        uint16_t addr = static_cast<uint16_t>(fetch8(state) + state.y);
        return addr;
    }

    inline uint16_t addr_ind(CPU_STATE& state)
    {
        NOT_IMPLEMENTED_ADDR_MODE("ind");
        uint16_t addr = fetch16(state);
        addr = read16(state, addr);
        return addr;
    }

    inline uint16_t addr_indx(CPU_STATE& state)
    {
        NOT_IMPLEMENTED_ADDR_MODE("indx");
        uint16_t addr_lo = static_cast<uint16_t>(fetch8(state) + state.x);
        uint16_t addr_hi = static_cast<uint16_t>(fetch8(state) + state.x + 1);
        uint8_t lo = read8(state, addr_lo);
        uint8_t hi = read8(state, addr_hi);
        uint16_t addr = static_cast<uint16_t>(lo) | (static_cast<uint16_t>(hi) << 8);
        return addr;
    }

    inline uint16_t addr_indy(CPU_STATE& state, int32_t clk = 0)
    {
        NOT_IMPLEMENTED_ADDR_MODE("indy");
        uint16_t addr_lo = static_cast<uint16_t>(fetch8(state));
        uint16_t addr_hi = static_cast<uint16_t>(fetch8(state) + 1);
        uint8_t lo = read8(state, addr_lo);
        uint8_t hi = read8(state, addr_hi);
        uint16_t addr = (static_cast<uint16_t>(lo) | (static_cast<uint16_t>(hi) << 8)) + state.y;
        if (static_cast<uint8_t>(addr) < state.y)
            state.clk -= clk;
        return addr;
    }

    ///////////////////////////////////////////////////////////////////////////

    inline uint8_t read8_imm(CPU_STATE& state)
    {
        return fetch8(state);
    }

    inline uint8_t read8_abs(CPU_STATE& state)
    {
        return read8(state, addr_abs(state));
    }

    inline uint8_t read8_zpg(CPU_STATE& state)
    {
        return read8(state, addr_zpg(state));
    }

    inline uint8_t read8_acc(CPU_STATE& state)
    {
        return state.a;
    }

    inline uint8_t read8_absx(CPU_STATE& state, int32_t clk = 0)
    {
        return read8(state, addr_absx(state, clk));
    }

    inline uint8_t read8_absy(CPU_STATE& state)
    {
        return read8(state, addr_absy(state));
    }

    inline uint8_t read8_zpgx(CPU_STATE& state)
    {
        return read8(state, addr_zpgx(state));
    }

    inline uint8_t read8_zpgy(CPU_STATE& state)
    {
        return read8(state, addr_zpgy(state));
    }

    inline uint8_t read8_indx(CPU_STATE& state)
    {
        return read8(state, addr_indx(state));
    }

    inline uint8_t read8_indy(CPU_STATE& state, int32_t clk = 0)
    {
        return read8(state, addr_indy(state, clk));
    }

    ///////////////////////////////////////////////////////////////////////////

    inline void push8(CPU_STATE& state, uint8_t value)
    {
        write8(state, state.sp-- + 0x100, value);
    }

    inline uint8_t pop8(CPU_STATE& state)
    {
        return read8(state, ++state.sp + 0x100);
    }

    inline void branch_if(CPU_STATE& state, bool branch)
    {
        int16_t offset = static_cast<int16_t>(static_cast<int8_t>(fetch8(state)));
        uint16_t pc = state.pc;
        uint16_t addr = pc + offset;
        if (branch)
        {
            state.pc = addr;
            --state.clk;
            if ((pc & 0xff00) != (addr & 0xff00))
                --state.clk;
        }
        state.clk -= 2;
    }

    ///////////////////////////////////////////////////////////////////////////

    inline void insn_adc(CPU_STATE& state, uint8_t src, int32_t clk)
    {
        NOT_IMPLEMENTED("adc");
    }

    inline void insn_and(CPU_STATE& state, uint8_t src, int32_t clk)
    {
        NOT_IMPLEMENTED("and");
    }

    inline void insn_asl(CPU_STATE& state, int32_t clk)
    {
        NOT_IMPLEMENTED("asl a");
    }

    inline void insn_asl(CPU_STATE& state, uint16_t addr, int32_t clk)
    {
        NOT_IMPLEMENTED("asl");
    }

    inline void insn_bcc(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("bcc");
    }

    inline void insn_bcs(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("bcs");
    }

    inline void insn_beq(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("beq");
    }

    inline void insn_bit(CPU_STATE& state, uint8_t src, int32_t clk)
    {
        NOT_IMPLEMENTED("bit");
    }

    inline void insn_bmi(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("bmi");
    }

    inline void insn_bne(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("bne");
    }

    inline void insn_bpl(CPU_STATE& state)
    {
        branch_if(state, !state.flag_n);
    }

    inline void insn_brk(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("brk");
    }

    inline void insn_bvc(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("bvc");
    }

    inline void insn_bvs(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("bvs");
    }

    inline void insn_clc(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("cld");
    }

    inline void insn_cld(CPU_STATE& state)
    {
        state.sr &= ~STATUS_D;
        state.clk -= 2;
    }

    inline void insn_cli(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("cli");
    }

    inline void insn_clv(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("clv");
    }

    inline void insn_cmp(CPU_STATE& state, uint8_t src, int32_t clk)
    {
        NOT_IMPLEMENTED("cmp");
    }

    inline void insn_cpx(CPU_STATE& state, uint8_t src, int32_t clk)
    {
        NOT_IMPLEMENTED("cpx");
    }

    inline void insn_cpy(CPU_STATE& state, uint8_t src, int32_t clk)
    {
        NOT_IMPLEMENTED("cpy");
    }

    inline void insn_dec(CPU_STATE& state, uint16_t addr, int32_t clk)
    {
        NOT_IMPLEMENTED("dec");
    }

    inline void insn_dex(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("dex");
    }

    inline void insn_dey(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("dey");
    }

    inline void insn_eor(CPU_STATE& state, uint8_t src, int32_t clk)
    {
        NOT_IMPLEMENTED("eor");
    }

    inline void insn_inc(CPU_STATE& state, uint16_t addr, int32_t clk)
    {
        NOT_IMPLEMENTED("inc");
    }

    inline void insn_inx(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("inx");
    }

    inline void insn_iny(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("iny");
    }

    inline void insn_jmp(CPU_STATE& state, uint16_t addr, int32_t clk)
    {
        NOT_IMPLEMENTED("jmp");
    }

    inline void insn_jsr(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("jsr");
    }

    inline void insn_lda(CPU_STATE& state, uint8_t src, int32_t clk)
    {
        state.flag_z = state.flag_n = state.a = src;
        state.clk -= clk;
    }

    inline void insn_ldx(CPU_STATE& state, uint8_t src, int32_t clk)
    {
        state.flag_z = state.flag_n = state.x = src;
        state.clk -= clk;
    }

    inline void insn_ldy(CPU_STATE& state, uint8_t src, int32_t clk)
    {
        NOT_IMPLEMENTED("ldy");
    }

    inline void insn_lsr(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("lsr a");
    }

    inline void insn_lsr(CPU_STATE& state, uint8_t src, int32_t clk)
    {
        NOT_IMPLEMENTED("lsr");
    }

    inline void insn_nop(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("nop");
    }

    inline void insn_ora(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("ora a");
    }

    inline void insn_ora(CPU_STATE& state, uint8_t src, int32_t clk)
    {
        NOT_IMPLEMENTED("ora");
    }

    inline void insn_pha(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("pha");
    }

    inline void insn_php(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("php");
    }

    inline void insn_pla(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("pla");
    }

    inline void insn_plp(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("plp");
    }

    inline void insn_rol(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("rol a");
    }

    inline void insn_rol(CPU_STATE& state, uint8_t src, int32_t clk)
    {
        NOT_IMPLEMENTED("rol");
    }

    inline void insn_ror(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("ror a");
    }

    inline void insn_ror(CPU_STATE& state, uint8_t src, int32_t clk)
    {
        NOT_IMPLEMENTED("ror");
    }

    inline void insn_rti(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("rti");
    }

    inline void insn_rts(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("rts");
    }

    inline void insn_sbc(CPU_STATE& state, uint8_t src, int32_t clk)
    {
        NOT_IMPLEMENTED("sbc");
    }

    inline void insn_sec(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("sec");
    }

    inline void insn_sed(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("sed");
    }

    inline void insn_sei(CPU_STATE& state)
    {
        // TODO: Implement IRQ interrupt masking
        state.sr |= STATUS_I;
        state.clk -= 2;
    }

    inline void insn_sta(CPU_STATE& state, uint16_t addr, int32_t clk)
    {
        write8(state, addr, state.a);
        state.clk -= clk;
    }

    inline void insn_stx(CPU_STATE& state, uint16_t addr, int32_t clk)
    {
        NOT_IMPLEMENTED("stx");
    }

    inline void insn_sty(CPU_STATE& state, uint16_t addr, int32_t clk)
    {
        NOT_IMPLEMENTED("sty");
    }

    inline void insn_tax(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("tax");
    }

    inline void insn_tay(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("tay");
    }

    inline void insn_tsx(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("tsx");
    }

    inline void insn_txa(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("txa");
    }

    inline void insn_txs(CPU_STATE& state)
    {
        push8(state, state.x);
        state.clk -= 2;
    }

    inline void insn_tya(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("tya");
    }

    /*
    |  Immediate     |   ADC #Oper           |    69   |    2    |    2     |
    |  Zero Page     |   ADC Oper            |    65   |    2    |    3     |
    |  Zero Page,X   |   ADC Oper,X          |    75   |    2    |    4     |
    |  Absolute      |   ADC Oper            |    6D   |    3    |    4     |
    |  Absolute,X    |   ADC Oper,X          |    7D   |    3    |    4*    |
    |  Absolute,Y    |   ADC Oper,Y          |    79   |    3    |    4*    |
    |  (Indirect,X)  |   ADC (Oper,X)        |    61   |    2    |    6     |
    |  (Indirect),Y  |   ADC (Oper),Y        |    71   |    2    |    5*    |
    |  Immediate     |   AND #Oper           |    29   |    2    |    2     |
    |  Zero Page     |   AND Oper            |    25   |    2    |    3     |
    |  Zero Page,X   |   AND Oper,X          |    35   |    2    |    4     |
    |  Absolute      |   AND Oper            |    2D   |    3    |    4     |
    |  Absolute,X    |   AND Oper,X          |    3D   |    3    |    4*    |
    |  Absolute,Y    |   AND Oper,Y          |    39   |    3    |    4*    |
    |  (Indirect,X)  |   AND (Oper,X)        |    21   |    2    |    6     |
    |  (Indirect,Y)  |   AND (Oper),Y        |    31   |    2    |    5     |
    |  Accumulator   |   ASL A               |    0A   |    1    |    2     |
    |  Zero Page     |   ASL Oper            |    06   |    2    |    5     |
    |  Zero Page,X   |   ASL Oper,X          |    16   |    2    |    6     |
    |  Absolute      |   ASL Oper            |    0E   |    3    |    6     |
    |  Absolute, X   |   ASL Oper,X          |    1E   |    3    |    7     |
    |  Relative      |   BCC Oper            |    90   |    2    |    2**   |
    |  Relative      |   BCS Oper            |    B0   |    2    |    2**   |
    |  Relative      |   BEQ Oper            |    F0   |    2    |    2**   |
    |  Zero Page     |   BIT Oper            |    24   |    2    |    3     |
    |  Absolute      |   BIT Oper            |    2C   |    3    |    4     |
    |  Relative      |   BMI Oper            |    30   |    2    |    2**   |
    |  Relative      |   BMI Oper            |    D0   |    2    |    2**   |
    |  Relative      |   BPL Oper            |    10   |    2    |    2**   |
    |  Implied       |   BRK                 |    00   |    1    |    7     |
    |  Relative      |   BVC Oper            |    50   |    2    |    2**   |
    |  Relative      |   BVS Oper            |    70   |    2    |    2**   |
    |  Implied       |   CLC                 |    18   |    1    |    2     |
    |  Implied       |   CLD                 |    D8   |    1    |    2     |
    |  Implied       |   CLI                 |    58   |    1    |    2     |
    |  Implied       |   CLV                 |    B8   |    1    |    2     |
    |  Immediate     |   CMP #Oper           |    C9   |    2    |    2     |
    |  Zero Page     |   CMP Oper            |    C5   |    2    |    3     |
    |  Zero Page,X   |   CMP Oper,X          |    D5   |    2    |    4     |
    |  Absolute      |   CMP Oper            |    CD   |    3    |    4     |
    |  Absolute,X    |   CMP Oper,X          |    DD   |    3    |    4*    |
    |  Absolute,Y    |   CMP Oper,Y          |    D9   |    3    |    4*    |
    |  (Indirect,X)  |   CMP (Oper,X)        |    C1   |    2    |    6     |
    |  (Indirect),Y  |   CMP (Oper),Y        |    D1   |    2    |    5*    |
    |  Immediate     |   CPX *Oper           |    E0   |    2    |    2     |
    |  Zero Page     |   CPX Oper            |    E4   |    2    |    3     |
    |  Absolute      |   CPX Oper            |    EC   |    3    |    4     |
    |  Immediate     |   CPY *Oper           |    C0   |    2    |    2     |
    |  Zero Page     |   CPY Oper            |    C4   |    2    |    3     |
    |  Absolute      |   CPY Oper            |    CC   |    3    |    4     |
    |  Zero Page     |   DEC Oper            |    C6   |    2    |    5     |
    |  Zero Page,X   |   DEC Oper,X          |    D6   |    2    |    6     |
    |  Absolute      |   DEC Oper            |    CE   |    3    |    6     |
    |  Absolute,X    |   DEC Oper,X          |    DE   |    3    |    7     |
    |  Implied       |   DEX                 |    CA   |    1    |    2     |
    |  Implied       |   DEY                 |    88   |    1    |    2     |
    |  Immediate     |   EOR #Oper           |    49   |    2    |    2     |
    |  Zero Page     |   EOR Oper            |    45   |    2    |    3     |
    |  Zero Page,X   |   EOR Oper,X          |    55   |    2    |    4     |
    |  Absolute      |   EOR Oper            |    40   |    3    |    4     |
    |  Absolute,X    |   EOR Oper,X          |    5D   |    3    |    4*    |
    |  Absolute,Y    |   EOR Oper,Y          |    59   |    3    |    4*    |
    |  (Indirect,X)  |   EOR (Oper,X)        |    41   |    2    |    6     |
    |  (Indirect),Y  |   EOR (Oper),Y        |    51   |    2    |    5*    |
    |  Zero Page     |   INC Oper            |    E6   |    2    |    5     |
    |  Zero Page,X   |   INC Oper,X          |    F6   |    2    |    6     |
    |  Absolute      |   INC Oper            |    EE   |    3    |    6     |
    |  Absolute,X    |   INC Oper,X          |    FE   |    3    |    7     |
    |  Implied       |   INX                 |    E8   |    1    |    2     |
    |  Implied       |   INY                 |    C8   |    1    |    2     |
    |  Absolute      |   JMP Oper            |    4C   |    3    |    3     |
    |  Indirect      |   JMP (Oper)          |    6C   |    3    |    5     |
    |  Absolute      |   JSR Oper            |    20   |    3    |    6     |
    |  Immediate     |   LDA #Oper           |    A9   |    2    |    2     |
    |  Zero Page     |   LDA Oper            |    A5   |    2    |    3     |
    |  Zero Page,X   |   LDA Oper,X          |    B5   |    2    |    4     |
    |  Absolute      |   LDA Oper            |    AD   |    3    |    4     |
    |  Absolute,X    |   LDA Oper,X          |    BD   |    3    |    4*    |
    |  Absolute,Y    |   LDA Oper,Y          |    B9   |    3    |    4*    |
    |  (Indirect,X)  |   LDA (Oper,X)        |    A1   |    2    |    6     |
    |  (Indirect),Y  |   LDA (Oper),Y        |    B1   |    2    |    5*    |
    |  Immediate     |   LDX #Oper           |    A2   |    2    |    2     |
    |  Zero Page     |   LDX Oper            |    A6   |    2    |    3     |
    |  Zero Page,Y   |   LDX Oper,Y          |    B6   |    2    |    4     |
    |  Absolute      |   LDX Oper            |    AE   |    3    |    4     |
    |  Absolute,Y    |   LDX Oper,Y          |    BE   |    3    |    4*    |
    |  Immediate     |   LDY #Oper           |    A0   |    2    |    2     |
    |  Zero Page     |   LDY Oper            |    A4   |    2    |    3     |
    |  Zero Page,X   |   LDY Oper,X          |    B4   |    2    |    4     |
    |  Absolute      |   LDY Oper            |    AC   |    3    |    4     |
    |  Absolute,X    |   LDY Oper,X          |    BC   |    3    |    4*    |
    |  Accumulator   |   LSR A               |    4A   |    1    |    2     |
    |  Zero Page     |   LSR Oper            |    46   |    2    |    5     |
    |  Zero Page,X   |   LSR Oper,X          |    56   |    2    |    6     |
    |  Absolute      |   LSR Oper            |    4E   |    3    |    6     |
    |  Absolute,X    |   LSR Oper,X          |    5E   |    3    |    7     |
    |  Implied       |   NOP                 |    EA   |    1    |    2     |
    |  Immediate     |   ORA #Oper           |    09   |    2    |    2     |
    |  Zero Page     |   ORA Oper            |    05   |    2    |    3     |
    |  Zero Page,X   |   ORA Oper,X          |    15   |    2    |    4     |
    |  Absolute      |   ORA Oper            |    0D   |    3    |    4     |
    |  Absolute,X    |   ORA Oper,X          |    1D   |    3    |    4*    |
    |  Absolute,Y    |   ORA Oper,Y          |    19   |    3    |    4*    |
    |  (Indirect,X)  |   ORA (Oper,X)        |    01   |    2    |    6     |
    |  (Indirect),Y  |   ORA (Oper),Y        |    11   |    2    |    5     |
    |  Implied       |   PHA                 |    48   |    1    |    3     |
    |  Implied       |   PHP                 |    08   |    1    |    3     |
    |  Implied       |   PLA                 |    68   |    1    |    4     |
    |  Implied       |   PLP                 |    28   |    1    |    4     |
    |  Accumulator   |   ROL A               |    2A   |    1    |    2     |
    |  Zero Page     |   ROL Oper            |    26   |    2    |    5     |
    |  Zero Page,X   |   ROL Oper,X          |    36   |    2    |    6     |
    |  Absolute      |   ROL Oper            |    2E   |    3    |    6     |
    |  Absolute,X    |   ROL Oper,X          |    3E   |    3    |    7     |
    |  Accumulator   |   ROR A               |    6A   |    1    |    2     |
    |  Zero Page     |   ROR Oper            |    66   |    2    |    5     |
    |  Zero Page,X   |   ROR Oper,X          |    76   |    2    |    6     |
    |  Absolute      |   ROR Oper            |    6E   |    3    |    6     |
    |  Absolute,X    |   ROR Oper,X          |    7E   |    3    |    7     |
    |  Implied       |   RTI                 |    4D   |    1    |    6     |
    |  Implied       |   RTS                 |    60   |    1    |    6     |
    |  Immediate     |   SBC #Oper           |    E9   |    2    |    2     |
    |  Zero Page     |   SBC Oper            |    E5   |    2    |    3     |
    |  Zero Page,X   |   SBC Oper,X          |    F5   |    2    |    4     |
    |  Absolute      |   SBC Oper            |    ED   |    3    |    4     |
    |  Absolute,X    |   SBC Oper,X          |    FD   |    3    |    4*    |
    |  Absolute,Y    |   SBC Oper,Y          |    F9   |    3    |    4*    |
    |  (Indirect,X)  |   SBC (Oper,X)        |    E1   |    2    |    6     |
    |  (Indirect),Y  |   SBC (Oper),Y        |    F1   |    2    |    5     |
    |  Implied       |   SEC                 |    38   |    1    |    2     |
    |  Implied       |   SED                 |    F8   |    1    |    2     |
    |  Implied       |   SEI                 |    78   |    1    |    2     |
    |  Zero Page     |   STA Oper            |    85   |    2    |    3     |
    |  Zero Page,X   |   STA Oper,X          |    95   |    2    |    4     |
    |  Absolute      |   STA Oper            |    80   |    3    |    4     |
    |  Absolute,X    |   STA Oper,X          |    9D   |    3    |    5     |
    |  Absolute,Y    |   STA Oper, Y         |    99   |    3    |    5     |
    |  (Indirect,X)  |   STA (Oper,X)        |    81   |    2    |    6     |
    |  (Indirect),Y  |   STA (Oper),Y        |    91   |    2    |    6     |
    |  Zero Page     |   STX Oper            |    86   |    2    |    3     |
    |  Zero Page,Y   |   STX Oper,Y          |    96   |    2    |    4     |
    |  Absolute      |   STX Oper            |    8E   |    3    |    4     |
    |  Zero Page     |   STY Oper            |    84   |    2    |    3     |
    |  Zero Page,X   |   STY Oper,X          |    94   |    2    |    4     |
    |  Absolute      |   STY Oper            |    8C   |    3    |    4     |
    |  Implied       |   TAX                 |    AA   |    1    |    2     |
    |  Implied       |   TAY                 |    A8   |    1    |    2     |
    |  Implied       |   TSX                 |    BA   |    1    |    2     |
    |  Implied       |   TXA                 |    8A   |    1    |    2     |
    |  Implied       |   TXS                 |    9A   |    1    |    2     |
    |  Implied       |   TYA                 |    98   |    1    |    2     |
    */
}

void cpu_initialize(CPU_STATE& cpu)
{
    memset(&cpu, 0, sizeof(cpu));
    cpu.sr = 0x34;
    cpu.a = cpu.x = cpu.y = 0;
    cpu.sp = 0xff;
}

bool cpu_create(CPU_STATE& cpu, MEMORY_BUS& bus)
{
    cpu.bus = &bus;
    return true;
}

void cpu_destroy(CPU_STATE& cpu)
{
}

void cpu_reset(CPU_STATE& cpu)
{
    cpu.sp -= 3;
    cpu.sr |= 0x04;
}

void cpu_execute(CPU_STATE& state, int32_t clock)
{
    state.clk += clock;
    while (state.clk > 0)
    {
        uint8_t insn = fetch8(state);
        switch (insn)
        {
        //case 0x69:  insn_adc(state, read8_imm(state),     2); break;
        //case 0x65:  insn_adc(state, read8_zpg(state),     3); break;
        //case 0x75:  insn_adc(state, read8_zpgx(state),    4); break;
        //case 0x6D:  insn_adc(state, read8_abs(state),     4); break;
        //case 0x7D:  insn_adc(state, read8_absx(state, 1), 4); break;
        //case 0x79:  insn_adc(state, read8_absx(state, 1), 4); break;
        //case 0x61:  insn_adc(state, read8_indx(state),    6); break;
        //case 0x71:  insn_adc(state, read8_indy(state, 1), 5); break;
        //case 0x29:  insn_and(state, read8_imm(state),     2); break;
        //case 0x25:  insn_and(state, read8_zpg(state),     3); break;
        //case 0x35:  insn_and(state, read8_zpgx(state),    4); break;
        //case 0x2D:  insn_and(state, read8_abs(state),     4); break;
        //case 0x3D:  insn_and(state, read8_absx(state, 1), 4); break;
        //case 0x39:  insn_and(state, read8_absx(state, 1), 4); break;
        //case 0x21:  insn_and(state, read8_indx(state),    6); break;
        //case 0x31:  insn_and(state, read8_indy(state),    5); break;
        //case 0x0A:  insn_asl(state,                   2); break;
        //case 0x06:  insn_asl(state, addr_zpg(state),  5); break;
        //case 0x16:  insn_asl(state, addr_zpgx(state), 6); break;
        //case 0x0E:  insn_asl(state, addr_abs(state),  6); break;
        //case 0x1E:  insn_asl(state, addr_absx(state), 7); break;
        //case 0x90:  insn_bcc(state); break;
        //case 0xB0:  insn_bcs(state); break;
        //case 0xF0:  insn_beq(state); break;
        //case 0x24:  insn_bit(state, read8_zpg(state), 3); break;
        //case 0x2C:  insn_bit(state, read8_abs(state), 3); break;
        //case 0x30:  insn_bmi(state); break;
        //case 0xD0:  insn_bmi(state); break;
        case 0x10:  insn_bpl(state); break;
        //case 0x00:  insn_brk(state); break;
        //case 0x50:  insn_bvc(state); break;
        //case 0x70:  insn_bvs(state); break;
        //case 0x18:  insn_clc(state); break;
        case 0xD8:  insn_cld(state); break;
        //case 0x58:  insn_cli(state); break;
        //case 0xB8:  insn_clv(state); break;
        //case 0xC9:  insn_cmp(state, read8_imm(state),     2); break;
        //case 0xC5:  insn_cmp(state, read8_zpg(state),     3); break;
        //case 0xD5:  insn_cmp(state, read8_zpgx(state),    4); break;
        //case 0xCD:  insn_cmp(state, read8_abs(state),     4); break;
        //case 0xDD:  insn_cmp(state, read8_absx(state, 1), 4); break;
        //case 0xD9:  insn_cmp(state, read8_absx(state, 1), 4); break;
        //case 0xC1:  insn_cmp(state, read8_indx(state),    6); break;
        //case 0xD1:  insn_cmp(state, read8_indy(state, 1), 5); break;
        //case 0xE0:  insn_cpx(state, read8_imm(state), 2); break;
        //case 0xE4:  insn_cpx(state, read8_zpg(state), 3); break;
        //case 0xEC:  insn_cpx(state, read8_abs(state), 4); break;
        //case 0xC0:  insn_cpy(state, read8_imm(state), 2); break;
        //case 0xC4:  insn_cpy(state, read8_zpg(state), 3); break;
        //case 0xCC:  insn_cpy(state, read8_abs(state), 4); break;
        //case 0xC6:  insn_dec(state, addr_zpg(state),  5); break;
        //case 0xD6:  insn_dec(state, addr_zpgx(state), 6); break;
        //case 0xCE:  insn_dec(state, addr_abs(state),  6); break;
        //case 0xDE:  insn_dec(state, addr_absx(state), 7); break;
        //case 0xCA:  insn_dex(state); break;
        //case 0x88:  insn_dey(state); break;
        //case 0x49:  insn_eor(state, read8_imm(state),     2); break;
        //case 0x45:  insn_eor(state, read8_zpg(state),     3); break;
        //case 0x55:  insn_eor(state, read8_zpgx(state),    4); break;
        //case 0x40:  insn_eor(state, read8_abs(state),     4); break;
        //case 0x5D:  insn_eor(state, read8_absx(state, 1), 4); break;
        //case 0x59:  insn_eor(state, read8_absx(state, 1), 4); break;
        //case 0x41:  insn_eor(state, read8_indx(state),    6); break;
        //case 0x51:  insn_eor(state, read8_indy(state, 1), 5); break;
        //case 0xE6:  insn_inc(state, addr_zpg(state),  5); break;
        //case 0xF6:  insn_inc(state, addr_zpgx(state), 6); break;
        //case 0xEE:  insn_inc(state, addr_abs(state),  6); break;
        //case 0xFE:  insn_inc(state, addr_absx(state), 7); break;
        //case 0xE8:  insn_inx(state); break;
        //case 0xC8:  insn_iny(state); break;
        //case 0x4C:  insn_jmp(state, addr_abs(state), 3); break;
        //case 0x6C:  insn_jmp(state, addr_ind(state), 5); break;
        //case 0x20:  insn_jsr(state); break;
        case 0xA9:  insn_lda(state, read8_imm(state),     2); break;
        //case 0xA5:  insn_lda(state, read8_zpg(state),     3); break;
        //case 0xB5:  insn_lda(state, read8_zpgx(state),    4); break;
        case 0xAD:  insn_lda(state, read8_abs(state),     4); break;
        //case 0xBD:  insn_lda(state, read8_absx(state, 1), 4); break;
        //case 0xB9:  insn_lda(state, read8_absx(state, 1), 4); break;
        //case 0xA1:  insn_lda(state, read8_indx(state),    6); break;
        //case 0xB1:  insn_lda(state, read8_indy(state, 1), 5); break;
        case 0xA2:  insn_ldx(state, read8_imm(state),     2); break;
        //case 0xA6:  insn_ldx(state, read8_zpg(state),     3); break;
        //case 0xB6:  insn_ldx(state, read8_zpgy(state),    4); break;
        //case 0xAE:  insn_ldx(state, read8_abs(state),     4); break;
        //case 0xBE:  insn_ldx(state, read8_absx(state, 1), 4); break;
        //case 0xA0:  insn_ldy(state, read8_imm(state),     2); break;
        //case 0xA4:  insn_ldy(state, read8_zpg(state),     3); break;
        //case 0xB4:  insn_ldy(state, read8_zpgx(state),    4); break;
        //case 0xAC:  insn_ldy(state, read8_abs(state),     4); break;
        //case 0xBC:  insn_ldy(state, read8_absx(state, 1), 4); break;
        //case 0x4A:  insn_lsr(state); break;
        //case 0x46:  insn_lsr(state, read8_zpg(state),  5); break;
        //case 0x56:  insn_lsr(state, read8_zpgx(state), 6); break;
        //case 0x4E:  insn_lsr(state, read8_abs(state),  6); break;
        //case 0x5E:  insn_lsr(state, read8_absx(state), 7); break;
        //case 0xEA:  insn_nop(state); break;
        //case 0x09:  insn_ora(state, read8_imm(state),     2); break;
        //case 0x05:  insn_ora(state, read8_zpg(state),     3); break;
        //case 0x15:  insn_ora(state, read8_zpgx(state),    4); break;
        //case 0x0D:  insn_ora(state, read8_abs(state),     4); break;
        //case 0x1D:  insn_ora(state, read8_absx(state, 1), 4); break;
        //case 0x19:  insn_ora(state, read8_absx(state, 1), 4); break;
        //case 0x01:  insn_ora(state, read8_indx(state),    6); break;
        //case 0x11:  insn_ora(state, read8_indy(state),    5); break;
        //case 0x48:  insn_pha(state); break;
        //case 0x08:  insn_php(state); break;
        //case 0x68:  insn_pla(state); break;
        //case 0x28:  insn_plp(state); break;
        //case 0x2A:  insn_rol(state); break;
        //case 0x26:  insn_rol(state, read8_zpg(state),  5); break;
        //case 0x36:  insn_rol(state, read8_zpgx(state), 6); break;
        //case 0x2E:  insn_rol(state, read8_abs(state),  6); break;
        //case 0x3E:  insn_rol(state, read8_absx(state), 7); break;
        //case 0x6A:  insn_ror(state); break;
        //case 0x66:  insn_ror(state, read8_zpg(state),  5); break;
        //case 0x76:  insn_ror(state, read8_zpgx(state), 6); break;
        //case 0x6E:  insn_ror(state, read8_abs(state),  6); break;
        //case 0x7E:  insn_ror(state, read8_absx(state), 7); break;
        //case 0x4D:  insn_rti(state); break;
        //case 0x60:  insn_rts(state); break;
        //case 0xE9:  insn_sbc(state, read8_imm(state),     2); break;
        //case 0xE5:  insn_sbc(state, read8_zpg(state),     3); break;
        //case 0xF5:  insn_sbc(state, read8_zpgx(state),    4); break;
        //case 0xED:  insn_sbc(state, read8_abs(state),     4); break;
        //case 0xFD:  insn_sbc(state, read8_absx(state, 1), 4); break;
        //case 0xF9:  insn_sbc(state, read8_absx(state, 1), 4); break;
        //case 0xE1:  insn_sbc(state, read8_indx(state),    6); break;
        //case 0xF1:  insn_sbc(state, read8_indy(state),    5); break;
        //case 0x38:  insn_sec(state); break;
        //case 0xF8:  insn_sed(state); break;
        case 0x78:  insn_sei(state); break;
        //case 0x85:  insn_sta(state, addr_zpg(state),  3); break;
        //case 0x95:  insn_sta(state, addr_zpgx(state), 4); break;
        case 0x8D:  insn_sta(state, addr_abs(state),  4); break;
        //case 0x9D:  insn_sta(state, addr_absx(state), 5); break;
        //case 0x99:  insn_sta(state, addr_absx(state), 5); break;
        //case 0x81:  insn_sta(state, addr_indx(state), 6); break;
        //case 0x91:  insn_sta(state, addr_indy(state), 6); break;
        //case 0x86:  insn_stx(state, addr_zpg(state),  3); break;
        //case 0x96:  insn_stx(state, addr_zpgy(state), 4); break;
        //case 0x8E:  insn_stx(state, addr_abs(state),  4); break;
        //case 0x84:  insn_sty(state, addr_zpg(state),  3); break;
        //case 0x94:  insn_sty(state, addr_zpgx(state), 4); break;
        //case 0x8C:  insn_sty(state, addr_abs(state),  4); break;
        //case 0xAA:  insn_tax(state); break;
        //case 0xA8:  insn_tay(state); break;
        //case 0xBA:  insn_tsx(state); break;
        //case 0x8A:  insn_txa(state); break;
        case 0x9A:  insn_txs(state); break;
        //case 0x98:  insn_tya(state); break;
        default:
            NOT_IMPLEMENTED("???");
        }
    }
}

Cpu6502::Cpu6502()
{
    cpu_initialize(mState);
}

Cpu6502::~Cpu6502()
{
    destroy();
}

bool Cpu6502::create(MEMORY_BUS& bus)
{
    if (!cpu_create(mState, bus))
        return false;
    return true;
}

void Cpu6502::destroy()
{
    cpu_destroy(mState);
}

void Cpu6502::reset()
{
    cpu_reset(mState);
    mState.pc = read16(mState, ADDR_VECTOR_RESET);
}

void Cpu6502::execute(int32_t cycles)
{
    cpu_execute(mState, cycles);
}