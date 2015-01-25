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

    enum ADDR_MODE
    {
        ADDR_IMPL,
        ADDR_IMM,
        ADDR_ACC,
        ADDR_REL,
        ADDR_ABS,
        ADDR_ZPG,
        ADDR_ABSX,
        ADDR_ABSY,
        ADDR_ZPGX,
        ADDR_ZPGY,
        ADDR_IND,
        ADDR_INDX,
        ADDR_INDY,
    };

    enum INSN_TYPE
    {
        INSN_ADC, INSN_AND, INSN_ASL, INSN_BCC, INSN_BCS, INSN_BEQ, INSN_BIT, INSN_BMI,
        INSN_BNE, INSN_BPL, INSN_BRK, INSN_BVC, INSN_BVS, INSN_CLC, INSN_CLD, INSN_CLI,
        INSN_CLV, INSN_CMP, INSN_CPX, INSN_CPY, INSN_DEC, INSN_DEX, INSN_DEY, INSN_EOR,
        INSN_INC, INSN_INX, INSN_INY, INSN_JMP, INSN_JSR, INSN_LDA, INSN_LDX, INSN_LDY,
        INSN_LSR, INSN_NOP, INSN_ORA, INSN_PHA, INSN_PHP, INSN_PLA, INSN_PLP, INSN_ROL,
        INSN_ROR, INSN_RTI, INSN_RTS, INSN_SBC, INSN_SEC, INSN_SED, INSN_SEI, INSN_STA,
        INSN_STX, INSN_STY, INSN_TAX, INSN_TAY, INSN_TSX, INSN_TXA, INSN_TXS, INSN_TYA,
        INSN_XXX,
    };

    const char* insn_name[] =
    {
        "adc", "and", "asl", "bcc", "bcs", "beq", "bit", "bmi",
        "bne", "bpl", "brk", "bvc", "bvs", "clc", "cld", "cli",
        "clv", "cmp", "cpx", "cpy", "dec", "dex", "dey", "eor",
        "inc", "inx", "iny", "jmp", "jsr", "lda", "ldx", "ldy",
        "lsr", "nop", "ora", "pha", "php", "pla", "plp", "rol",
        "ror", "rti", "rts", "sbc", "sec", "sed", "sei", "sta",
        "stx", "sty", "tax", "tay", "tsx", "txa", "txs", "tya",
        "???"
    };

    const uint8_t addr_mode_table[] =
    {
        //   00         01          02         03         04         05         06         07         08         09         0a         0b         0c         0d         0e         0f
        ADDR_IMPL, ADDR_INDX,  ADDR_IMPL, ADDR_IMPL, ADDR_IMPL, ADDR_ZPG,  ADDR_ZPG,  ADDR_IMPL, ADDR_IMPL, ADDR_IMM,  ADDR_ACC,  ADDR_IMPL, ADDR_IMPL, ADDR_ABS,  ADDR_ABS,  ADDR_IMPL, // 00
        ADDR_REL,  ADDR_INDY,  ADDR_IMPL, ADDR_IMPL, ADDR_IMPL, ADDR_ZPGX, ADDR_ZPGX, ADDR_IMPL, ADDR_IMPL, ADDR_ABSY, ADDR_IMPL, ADDR_IMPL, ADDR_IMPL, ADDR_ABSX, ADDR_ABSX, ADDR_IMPL, // 10
        ADDR_ABS,  ADDR_INDX,  ADDR_IMPL, ADDR_IMPL, ADDR_ZPG,  ADDR_ZPG,  ADDR_ZPG,  ADDR_IMPL, ADDR_IMPL, ADDR_IMM,  ADDR_ACC,  ADDR_IMPL, ADDR_ABS,  ADDR_ABS,  ADDR_ABS,  ADDR_IMPL, // 20
        ADDR_REL,  ADDR_INDY,  ADDR_IMPL, ADDR_IMPL, ADDR_IMPL, ADDR_ZPGX, ADDR_ZPGX, ADDR_IMPL, ADDR_IMPL, ADDR_ABSY, ADDR_IMPL, ADDR_IMPL, ADDR_IMPL, ADDR_ABSX, ADDR_ABSX, ADDR_IMPL, // 30
        ADDR_IMPL, ADDR_INDX,  ADDR_IMPL, ADDR_IMPL, ADDR_IMPL, ADDR_ZPG,  ADDR_ZPG,  ADDR_IMPL, ADDR_IMPL, ADDR_IMM,  ADDR_ACC,  ADDR_IMPL, ADDR_ABS,  ADDR_ABS,  ADDR_ABS,  ADDR_IMPL, // 40
        ADDR_REL,  ADDR_INDY,  ADDR_IMPL, ADDR_IMPL, ADDR_IMPL, ADDR_ZPGX, ADDR_ZPGX, ADDR_IMPL, ADDR_IMPL, ADDR_ABSY, ADDR_IMPL, ADDR_IMPL, ADDR_IMPL, ADDR_ABSX, ADDR_ABSX, ADDR_IMPL, // 50
        ADDR_IMPL, ADDR_INDX,  ADDR_IMPL, ADDR_IMPL, ADDR_IMPL, ADDR_ZPG,  ADDR_ZPG,  ADDR_IMPL, ADDR_IMPL, ADDR_IMM,  ADDR_ACC,  ADDR_IMPL, ADDR_IND,  ADDR_ABS,  ADDR_ABS,  ADDR_IMPL, // 60
        ADDR_REL,  ADDR_INDY,  ADDR_IMPL, ADDR_IMPL, ADDR_IMPL, ADDR_ZPGX, ADDR_ZPGX, ADDR_IMPL, ADDR_IMPL, ADDR_ABSY, ADDR_IMPL, ADDR_IMPL, ADDR_IMPL, ADDR_ABSX, ADDR_ABSX, ADDR_IMPL, // 70
        ADDR_IMPL, ADDR_INDX,  ADDR_IMPL, ADDR_IMPL, ADDR_ZPG,  ADDR_ZPG,  ADDR_ZPG,  ADDR_IMPL, ADDR_IMPL, ADDR_IMPL, ADDR_IMPL, ADDR_IMPL, ADDR_ABS,  ADDR_ABS,  ADDR_ABS,  ADDR_IMPL, // 80
        ADDR_REL,  ADDR_INDY,  ADDR_IMPL, ADDR_IMPL, ADDR_ZPGX, ADDR_ZPGX, ADDR_ZPGY, ADDR_IMPL, ADDR_IMPL, ADDR_ABSY, ADDR_IMPL, ADDR_IMPL, ADDR_IMPL, ADDR_ABSX, ADDR_IMPL, ADDR_IMPL, // 90
        ADDR_IMM,  ADDR_INDX,  ADDR_IMM,  ADDR_IMPL, ADDR_ZPG,  ADDR_ZPG,  ADDR_ZPG,  ADDR_IMPL, ADDR_IMPL, ADDR_IMM,  ADDR_IMPL, ADDR_IMPL, ADDR_ABS,  ADDR_ABS,  ADDR_ABS,  ADDR_IMPL, // a0
        ADDR_REL,  ADDR_INDY,  ADDR_IMPL, ADDR_IMPL, ADDR_ZPGX, ADDR_ZPGX, ADDR_ZPGY, ADDR_IMPL, ADDR_IMPL, ADDR_ABSY, ADDR_IMPL, ADDR_IMPL, ADDR_ABSX, ADDR_ABSX, ADDR_ABSY, ADDR_IMPL, // b0
        ADDR_IMM,  ADDR_INDX,  ADDR_IMPL, ADDR_IMPL, ADDR_ZPG,  ADDR_ZPG,  ADDR_ZPG,  ADDR_IMPL, ADDR_IMPL, ADDR_IMM,  ADDR_IMPL, ADDR_IMPL, ADDR_ABS,  ADDR_ABS,  ADDR_ABS,  ADDR_IMPL, // c0
        ADDR_REL,  ADDR_INDY,  ADDR_IMPL, ADDR_IMPL, ADDR_IMPL, ADDR_ZPGX, ADDR_ZPGX, ADDR_IMPL, ADDR_IMPL, ADDR_ABSY, ADDR_IMPL, ADDR_IMPL, ADDR_IMPL, ADDR_ABSX, ADDR_ABSX, ADDR_IMPL, // d0
        ADDR_IMM,  ADDR_INDX,  ADDR_IMPL, ADDR_IMPL, ADDR_ZPG,  ADDR_ZPG,  ADDR_ZPG,  ADDR_IMPL, ADDR_IMPL, ADDR_IMM,  ADDR_IMPL, ADDR_IMPL, ADDR_ABS,  ADDR_ABS,  ADDR_ABS,  ADDR_IMPL, // e0
        ADDR_REL,  ADDR_INDY,  ADDR_IMPL, ADDR_IMPL, ADDR_IMPL, ADDR_ZPGX, ADDR_ZPGX, ADDR_IMPL, ADDR_IMPL, ADDR_ABSY, ADDR_IMPL, ADDR_IMPL, ADDR_IMPL, ADDR_ABSX, ADDR_ABSX, ADDR_IMPL, // f0
    };

    const uint8_t insn_table[] =
    {
        //   00        01        02        03        04        05        06        07        08        09        0a        0b        0c        0d        0e        0f
        INSN_BRK, INSN_ORA, INSN_XXX, INSN_XXX, INSN_XXX, INSN_ORA, INSN_ASL, INSN_XXX, INSN_PHP, INSN_ORA, INSN_ASL, INSN_XXX, INSN_XXX, INSN_ORA, INSN_ASL, INSN_XXX, // 00
        INSN_BPL, INSN_ORA, INSN_XXX, INSN_XXX, INSN_XXX, INSN_ORA, INSN_ASL, INSN_XXX, INSN_CLC, INSN_ORA, INSN_XXX, INSN_XXX, INSN_XXX, INSN_ORA, INSN_ASL, INSN_XXX, // 10
        INSN_JSR, INSN_AND, INSN_XXX, INSN_XXX, INSN_BIT, INSN_AND, INSN_ROL, INSN_XXX, INSN_PLP, INSN_AND, INSN_ROL, INSN_XXX, INSN_BIT, INSN_AND, INSN_ROL, INSN_XXX, // 20
        INSN_BMI, INSN_AND, INSN_XXX, INSN_XXX, INSN_XXX, INSN_AND, INSN_ROL, INSN_XXX, INSN_SEC, INSN_AND, INSN_XXX, INSN_XXX, INSN_XXX, INSN_AND, INSN_ROL, INSN_XXX, // 30
        INSN_RTI, INSN_EOR, INSN_XXX, INSN_XXX, INSN_XXX, INSN_EOR, INSN_LSR, INSN_XXX, INSN_PHA, INSN_EOR, INSN_LSR, INSN_XXX, INSN_JMP, INSN_EOR, INSN_LSR, INSN_XXX, // 40
        INSN_BVC, INSN_EOR, INSN_XXX, INSN_XXX, INSN_XXX, INSN_EOR, INSN_LSR, INSN_XXX, INSN_CLI, INSN_EOR, INSN_XXX, INSN_XXX, INSN_XXX, INSN_EOR, INSN_LSR, INSN_XXX, // 50
        INSN_RTS, INSN_ADC, INSN_XXX, INSN_XXX, INSN_XXX, INSN_ADC, INSN_ROR, INSN_XXX, INSN_PLA, INSN_ADC, INSN_ROR, INSN_XXX, INSN_JMP, INSN_ADC, INSN_ROR, INSN_XXX, // 60
        INSN_BVS, INSN_ADC, INSN_XXX, INSN_XXX, INSN_XXX, INSN_ADC, INSN_ROR, INSN_XXX, INSN_SEI, INSN_ADC, INSN_XXX, INSN_XXX, INSN_XXX, INSN_ADC, INSN_ROR, INSN_XXX, // 70
        INSN_XXX, INSN_STA, INSN_XXX, INSN_XXX, INSN_STY, INSN_STA, INSN_STX, INSN_XXX, INSN_DEY, INSN_XXX, INSN_TXA, INSN_XXX, INSN_STY, INSN_STA, INSN_STX, INSN_XXX, // 80
        INSN_BCC, INSN_STA, INSN_XXX, INSN_XXX, INSN_STY, INSN_STA, INSN_STX, INSN_XXX, INSN_TYA, INSN_STA, INSN_TXS, INSN_XXX, INSN_XXX, INSN_STA, INSN_XXX, INSN_XXX, // 90
        INSN_LDY, INSN_LDA, INSN_LDX, INSN_XXX, INSN_LDY, INSN_LDA, INSN_LDX, INSN_XXX, INSN_TAY, INSN_LDA, INSN_TAX, INSN_XXX, INSN_LDY, INSN_LDA, INSN_LDX, INSN_XXX, // a0
        INSN_BCS, INSN_LDA, INSN_XXX, INSN_XXX, INSN_LDY, INSN_LDA, INSN_LDX, INSN_XXX, INSN_CLV, INSN_LDA, INSN_TSX, INSN_XXX, INSN_LDY, INSN_LDA, INSN_LDX, INSN_XXX, // b0
        INSN_CPY, INSN_CMP, INSN_XXX, INSN_XXX, INSN_CPY, INSN_CMP, INSN_DEC, INSN_XXX, INSN_INY, INSN_CMP, INSN_DEX, INSN_XXX, INSN_CPY, INSN_CMP, INSN_DEC, INSN_XXX, // c0
        INSN_BNE, INSN_CMP, INSN_XXX, INSN_XXX, INSN_XXX, INSN_CMP, INSN_DEC, INSN_XXX, INSN_CLD, INSN_CMP, INSN_XXX, INSN_XXX, INSN_XXX, INSN_CMP, INSN_DEC, INSN_XXX, // d0
        INSN_CPX, INSN_SBC, INSN_XXX, INSN_XXX, INSN_CPX, INSN_SBC, INSN_INC, INSN_XXX, INSN_INX, INSN_SBC, INSN_NOP, INSN_XXX, INSN_CPX, INSN_SBC, INSN_INC, INSN_XXX, // e0
        INSN_BEQ, INSN_SBC, INSN_XXX, INSN_XXX, INSN_XXX, INSN_SBC, INSN_INC, INSN_XXX, INSN_SED, INSN_SBC, INSN_XXX, INSN_XXX, INSN_XXX, INSN_SBC, INSN_INC, INSN_XXX, // f0
    };

    const uint8_t insn_ticks_cpu[] =
    {
    //  00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f
        7, 6, 0, 0, 0, 3, 5, 0, 3, 2, 2, 0, 0, 4, 6, 0, // 00
        2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0, // 10
        6, 6, 0, 0, 3, 3, 5, 0, 4, 2, 2, 0, 4, 4, 6, 0, // 20
        2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0, // 30
        4, 6, 0, 0, 0, 3, 5, 0, 3, 2, 2, 0, 3, 6, 6, 0, // 40
        2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0, // 50
        6, 6, 0, 0, 0, 3, 5, 0, 4, 2, 2, 0, 5, 4, 6, 0, // 60
        2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0, // 70
        0, 6, 0, 0, 3, 3, 3, 0, 2, 0, 2, 0, 4, 4, 4, 0, // 80
        2, 6, 0, 0, 4, 4, 4, 0, 2, 5, 2, 0, 0, 5, 0, 0, // 90
        2, 6, 2, 0, 3, 3, 3, 0, 2, 2, 2, 0, 4, 4, 4, 0, // a0
        2, 5, 0, 0, 4, 4, 4, 0, 2, 4, 2, 0, 4, 4, 4, 0, // b0
        2, 6, 0, 0, 3, 3, 5, 0, 2, 2, 2, 0, 4, 4, 6, 0, // c0
        2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0, // d0
        2, 6, 0, 0, 3, 3, 5, 0, 2, 2, 2, 0, 4, 4, 6, 0, // e0
        2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0, // f0
    };

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
        return memory_bus_read8(*state.bus, state.executed_ticks, addr);
    }

    inline void write8(CPU_STATE& state, uint16_t addr, uint8_t value)
    {
        memory_bus_write8(*state.bus, addr, state.executed_ticks, value);
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

    uint8_t peek8(CPU_STATE& state, uint16_t& addr)
    {
        return read8(state, addr++);
    }

    uint16_t peek16(CPU_STATE& state, uint16_t& addr)
    {
        uint8_t lo = peek8(state, addr);
        uint8_t hi = peek8(state, addr);
        uint16_t value = static_cast<uint16_t>(lo) | (static_cast<uint16_t>(hi) << 8);
        return value;
    }

    uint16_t disassemble(CPU_STATE& state, uint16_t pc, char* dest, size_t size)
    {
        uint8_t insn = peek8(state, pc);
        INSN_TYPE insn_type = static_cast<INSN_TYPE>(insn_table[insn]);
        ADDR_MODE addr_mode = static_cast<ADDR_MODE>(addr_mode_table[insn]);
        const char* opcode = insn_name[insn_type];
        char temp[32];
        switch (addr_mode)
        {
        case ADDR_IMPL: sprintf(temp, "%s", opcode); break;
        case ADDR_IMM:  sprintf(temp, "%s     #$%02x", opcode, peek8(state, pc)); break;
        //case ADDR_ACC:  sprintf(temp, "%s     a", opcode); break;
        case ADDR_REL:  sprintf(temp, "%s     $%04x", opcode, static_cast<uint16_t>(static_cast<int8_t>(peek8(state, pc)) + pc)); break;
        case ADDR_ABS:  sprintf(temp, "%s     $%04x", opcode, peek16(state, pc)); break;
        //case ADDR_ZPG:  sprintf(temp, "%s     $%02x", opcode, peek8(state, pc)); break;
        //case ADDR_ABSX: sprintf(temp, "%s     $%04x, x", opcode, peek16(state, pc)); break;
        //case ADDR_ABSY: sprintf(temp, "%s     $%04x, y", opcode, peek16(state, pc)); break;
        //case ADDR_ZPGX: sprintf(temp, "%s     $%02x, x", opcode, peek8(state, pc)); break;
        //case ADDR_ZPGY: sprintf(temp, "%s     $%02x, y", opcode, peek8(state, pc)); break;
        //case ADDR_IND:  sprintf(temp, "%s     ($%04x)", opcode, peek16(state, pc)); break;
        //case ADDR_INDX: sprintf(temp, "%s     ($%02x, x)", opcode, peek8(state, pc)); break;
        //case ADDR_INDY: sprintf(temp, "%s     ($%02x), y", opcode, peek8(state, pc)); break;
        default:
            assert(0);
        }
        if (size-- > 0)
            strncpy(dest, temp, size);
        dest[size] = 0;
        return pc;
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

    inline uint16_t addr_absx(CPU_STATE& state, int32_t extra_ticks = 0)
    {
        NOT_IMPLEMENTED_ADDR_MODE("absx");
        uint16_t base_addr = fetch16(state);
        uint16_t addr = base_addr + static_cast<uint16_t>(state.x);
        if ((base_addr & 0xff00) != (addr & 0xff00))
            state.executed_ticks += extra_ticks;
        return addr;
    }

    inline uint16_t addr_absy(CPU_STATE& state, int32_t extra_ticks = 0)
    {
        NOT_IMPLEMENTED_ADDR_MODE("absy");
        uint16_t base_addr = fetch16(state);
        uint16_t addr = base_addr + static_cast<uint16_t>(state.y);
        if ((base_addr & 0xff00) != (addr & 0xff00))
            state.executed_ticks += extra_ticks;
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

    inline uint16_t addr_indy(CPU_STATE& state, int32_t extra_ticks = 0)
    {
        NOT_IMPLEMENTED_ADDR_MODE("indy");
        uint16_t addr_lo = static_cast<uint16_t>(fetch8(state));
        uint16_t addr_hi = static_cast<uint16_t>(fetch8(state) + 1);
        uint8_t lo = read8(state, addr_lo);
        uint8_t hi = read8(state, addr_hi);
        uint16_t base_addr = (static_cast<uint16_t>(lo) | (static_cast<uint16_t>(hi) << 8));
        uint16_t addr = base_addr + state.y;
        if ((base_addr & 0xff00) != (addr & 0xff00))
            state.executed_ticks += extra_ticks;
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

    inline uint8_t read8_absx(CPU_STATE& state, int32_t extra_ticks = 0)
    {
        return read8(state, addr_absx(state, extra_ticks));
    }

    inline uint8_t read8_absy(CPU_STATE& state, int32_t extra_ticks = 0)
    {
        return read8(state, addr_absy(state, extra_ticks));
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

    inline uint8_t read8_indy(CPU_STATE& state, int32_t extra_ticks = 0)
    {
        return read8(state, addr_indy(state, extra_ticks));
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

    ///////////////////////////////////////////////////////////////////////////

    inline void insn_branch(CPU_STATE& state, bool branch)
    {
        int16_t offset = static_cast<int16_t>(static_cast<int8_t>(fetch8(state)));
        uint16_t pc = state.pc;
        uint16_t addr = pc + offset;
        if (branch)
        {
            state.pc = addr;
            state.executed_ticks += state.master_clock_divider;
            if ((pc & 0xff00) != (addr & 0xff00))
                state.executed_ticks += state.master_clock_divider;
        }
    }

    inline void insn_adc(CPU_STATE& state, uint8_t src)
    {
        NOT_IMPLEMENTED("adc");
    }

    inline void insn_and(CPU_STATE& state, uint8_t src)
    {
        NOT_IMPLEMENTED("and");
    }

    inline void insn_asl(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("asl a");
    }

    inline void insn_asl(CPU_STATE& state, uint16_t addr)
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

    inline void insn_bit(CPU_STATE& state, uint8_t src)
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
        insn_branch(state, !state.flag_n);
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
    }

    inline void insn_cli(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("cli");
    }

    inline void insn_clv(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("clv");
    }

    inline void insn_cmp(CPU_STATE& state, uint8_t src)
    {
        NOT_IMPLEMENTED("cmp");
    }

    inline void insn_cpx(CPU_STATE& state, uint8_t src)
    {
        NOT_IMPLEMENTED("cpx");
    }

    inline void insn_cpy(CPU_STATE& state, uint8_t src)
    {
        NOT_IMPLEMENTED("cpy");
    }

    inline void insn_dec(CPU_STATE& state, uint16_t addr)
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

    inline void insn_eor(CPU_STATE& state, uint8_t src)
    {
        NOT_IMPLEMENTED("eor");
    }

    inline void insn_inc(CPU_STATE& state, uint16_t addr)
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

    inline void insn_jmp(CPU_STATE& state, uint16_t addr)
    {
        NOT_IMPLEMENTED("jmp");
    }

    inline void insn_jsr(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("jsr");
    }

    inline void insn_lda(CPU_STATE& state, uint8_t src)
    {
        state.flag_z = state.flag_n = state.a = src;
    }

    inline void insn_ldx(CPU_STATE& state, uint8_t src)
    {
        state.flag_z = state.flag_n = state.x = src;
    }

    inline void insn_ldy(CPU_STATE& state, uint8_t src)
    {
        NOT_IMPLEMENTED("ldy");
    }

    inline void insn_lsr(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("lsr a");
    }

    inline void insn_lsr(CPU_STATE& state, uint8_t src)
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

    inline void insn_ora(CPU_STATE& state, uint8_t src)
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

    inline void insn_rol(CPU_STATE& state, uint8_t src)
    {
        NOT_IMPLEMENTED("rol");
    }

    inline void insn_ror(CPU_STATE& state)
    {
        NOT_IMPLEMENTED("ror a");
    }

    inline void insn_ror(CPU_STATE& state, uint8_t src)
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

    inline void insn_sbc(CPU_STATE& state, uint8_t src)
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
    }

    inline void insn_sta(CPU_STATE& state, uint16_t addr)
    {
        write8(state, addr, state.a);
    }

    inline void insn_stx(CPU_STATE& state, uint16_t addr)
    {
        NOT_IMPLEMENTED("stx");
    }

    inline void insn_sty(CPU_STATE& state, uint16_t addr)
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
    |  Absolute      |   STA Oper            |    8D   |    3    |    4     |
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

    void executeDummyTimerEvent(void* context, int32_t ticks)
    {
    }
}

void cpu_initialize(CPU_STATE& cpu)
{
    memset(&cpu, 0, sizeof(cpu));
    cpu.sr = 0x34;
    cpu.a = cpu.x = cpu.y = 0;
    cpu.sp = 0xff;
}

bool cpu_create(CPU_STATE& cpu, MEMORY_BUS& bus, uint32_t master_clock_divider)
{
    cpu.bus = &bus;

    // Define master clock cycles
    cpu.master_clock_divider = master_clock_divider;
    for (uint32_t insn_index = 0; insn_index < 256; ++insn_index)
        cpu.insn_ticks[insn_index] = insn_ticks_cpu[insn_index] * master_clock_divider;

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

void cpu_execute(CPU_STATE& state, int32_t num_ticks)
{
    state.desired_ticks += num_ticks;
    while (state.executed_ticks < state.desired_ticks)
    {
#if 1
        char temp[32];
        uint16_t pc = state.pc;
        disassemble(state, pc, temp, sizeof(temp));
        printf("$%04x:  %s\n", state.pc, temp);
#endif
        uint8_t insn = fetch8(state);
        uint32_t insn_ticks = state.insn_ticks[insn];
        state.executed_ticks += insn_ticks;
        switch (insn)
        {
        //case 0x69: insn_adc(state, read8_imm(state)); break;
        //case 0x65: insn_adc(state, read8_zpg(state)); break;
        //case 0x75: insn_adc(state, read8_zpgx(state)); break;
        //case 0x6D: insn_adc(state, read8_abs(state)); break;
        //case 0x7D: insn_adc(state, read8_absx(state, state.master_clock_divider)); break;
        //case 0x79: insn_adc(state, read8_absy(state, state.master_clock_divider)); break;
        //case 0x61: insn_adc(state, read8_indx(state)); break;
        //case 0x71: insn_adc(state, read8_indy(state, state.master_clock_divider)); break;
        //case 0x29: insn_and(state, read8_imm(state)); break;
        //case 0x25: insn_and(state, read8_zpg(state)); break;
        //case 0x35: insn_and(state, read8_zpgx(state)); break;
        //case 0x2D: insn_and(state, read8_abs(state)); break;
        //case 0x3D: insn_and(state, read8_absx(state, state.master_clock_divider)); break;
        //case 0x39: insn_and(state, read8_absy(state, state.master_clock_divider)); break;
        //case 0x21: insn_and(state, read8_indx(state)); break;
        //case 0x31: insn_and(state, read8_indy(state)); break;
        //case 0x0A: insn_asl(state); break;
        //case 0x06: insn_asl(state, addr_zpg(state)); break;
        //case 0x16: insn_asl(state, addr_zpgx(state)); break;
        //case 0x0E: insn_asl(state, addr_abs(state)); break;
        //case 0x1E: insn_asl(state, addr_absx(state)); break;
        //case 0x90: insn_bcc(state); break;
        //case 0xB0: insn_bcs(state); break;
        //case 0xF0: insn_beq(state); break;
        //case 0x24: insn_bit(state, read8_zpg(state)); break;
        //case 0x2C: insn_bit(state, read8_abs(state)); break;
        //case 0x30: insn_bmi(state); break;
        //case 0xD0: insn_bmi(state); break;
        case 0x10: insn_bpl(state); break;
        //case 0x00: insn_brk(state); break;
        //case 0x50: insn_bvc(state); break;
        //case 0x70: insn_bvs(state); break;
        //case 0x18: insn_clc(state); break;
        case 0xD8: insn_cld(state); break;
        //case 0x58: insn_cli(state); break;
        //case 0xB8: insn_clv(state); break;
        //case 0xC9: insn_cmp(state, read8_imm(state)); break;
        //case 0xC5: insn_cmp(state, read8_zpg(state)); break;
        //case 0xD5: insn_cmp(state, read8_zpgx(state)); break;
        //case 0xCD: insn_cmp(state, read8_abs(state)); break;
        //case 0xDD: insn_cmp(state, read8_absx(state, state.master_clock_divider)); break;
        //case 0xD9: insn_cmp(state, read8_absy(state, state.master_clock_divider)); break;
        //case 0xC1: insn_cmp(state, read8_indx(state)); break;
        //case 0xD1: insn_cmp(state, read8_indy(state, state.master_clock_divider)); break;
        //case 0xE0: insn_cpx(state, read8_imm(state)); break;
        //case 0xE4: insn_cpx(state, read8_zpg(state)); break;
        //case 0xEC: insn_cpx(state, read8_abs(state)); break;
        //case 0xC0: insn_cpy(state, read8_imm(state)); break;
        //case 0xC4: insn_cpy(state, read8_zpg(state)); break;
        //case 0xCC: insn_cpy(state, read8_abs(state)); break;
        //case 0xC6: insn_dec(state, addr_zpg(state)); break;
        //case 0xD6: insn_dec(state, addr_zpgx(state)); break;
        //case 0xCE: insn_dec(state, addr_abs(state)); break;
        //case 0xDE: insn_dec(state, addr_absx(state)); break;
        //case 0xCA: insn_dex(state); break;
        //case 0x88: insn_dey(state); break;
        //case 0x49: insn_eor(state, read8_imm(state)); break;
        //case 0x45: insn_eor(state, read8_zpg(state)); break;
        //case 0x55: insn_eor(state, read8_zpgx(state)); break;
        //case 0x40: insn_eor(state, read8_abs(state)); break;
        //case 0x5D: insn_eor(state, read8_absx(state, state.master_clock_divider)); break;
        //case 0x59: insn_eor(state, read8_absy(state, state.master_clock_divider)); break;
        //case 0x41: insn_eor(state, read8_indx(state)); break;
        //case 0x51: insn_eor(state, read8_indy(state, state.master_clock_divider)); break;
        //case 0xE6: insn_inc(state, addr_zpg(state)); break;
        //case 0xF6: insn_inc(state, addr_zpgx(state)); break;
        //case 0xEE: insn_inc(state, addr_abs(state)); break;
        //case 0xFE: insn_inc(state, addr_absx(state)); break;
        //case 0xE8: insn_inx(state); break;
        //case 0xC8: insn_iny(state); break;
        //case 0x4C: insn_jmp(state, addr_abs(state)); break;
        //case 0x6C: insn_jmp(state, addr_ind(state)); break;
        //case 0x20: insn_jsr(state); break;
        case 0xA9: insn_lda(state, read8_imm(state)); break;
        //case 0xA5: insn_lda(state, read8_zpg(state)); break;
        //case 0xB5: insn_lda(state, read8_zpgx(state)); break;
        case 0xAD: insn_lda(state, read8_abs(state)); break;
        //case 0xBD: insn_lda(state, read8_absx(state, state.master_clock_divider)); break;
        //case 0xB9: insn_lda(state, read8_absy(state, state.master_clock_divider)); break;
        //case 0xA1: insn_lda(state, read8_indx(state)); break;
        //case 0xB1: insn_lda(state, read8_indy(state, state.master_clock_divider)); break;
        case 0xA2: insn_ldx(state, read8_imm(state)); break;
        //case 0xA6: insn_ldx(state, read8_zpg(state)); break;
        //case 0xB6: insn_ldx(state, read8_zpgy(state)); break;
        //case 0xAE: insn_ldx(state, read8_abs(state)); break;
        //case 0xBE: insn_ldx(state, read8_absx(state, state.master_clock_divider)); break;
        //case 0xA0: insn_ldy(state, read8_imm(state)); break;
        //case 0xA4: insn_ldy(state, read8_zpg(state)); break;
        //case 0xB4: insn_ldy(state, read8_zpgx(state)); break;
        //case 0xAC: insn_ldy(state, read8_abs(state)); break;
        //case 0xBC: insn_ldy(state, read8_absx(state, state.master_clock_divider)); break;
        //case 0x4A: insn_lsr(state); break;
        //case 0x46: insn_lsr(state, read8_zpg(state)); break;
        //case 0x56: insn_lsr(state, read8_zpgx(state)); break;
        //case 0x4E: insn_lsr(state, read8_abs(state)); break;
        //case 0x5E: insn_lsr(state, read8_absx(state)); break;
        //case 0xEA: insn_nop(state); break;
        //case 0x09: insn_ora(state, read8_imm(state)); break;
        //case 0x05: insn_ora(state, read8_zpg(state)); break;
        //case 0x15: insn_ora(state, read8_zpgx(state)); break;
        //case 0x0D: insn_ora(state, read8_abs(state)); break;
        //case 0x1D: insn_ora(state, read8_absx(state, state.master_clock_divider)); break;
        //case 0x19: insn_ora(state, read8_absy(state, state.master_clock_divider)); break;
        //case 0x01: insn_ora(state, read8_indx(state)); break;
        //case 0x11: insn_ora(state, read8_indy(state)); break;
        //case 0x48: insn_pha(state); break;
        //case 0x08: insn_php(state); break;
        //case 0x68: insn_pla(state); break;
        //case 0x28: insn_plp(state); break;
        //case 0x2A: insn_rol(state); break;
        //case 0x26: insn_rol(state, read8_zpg(state)); break;
        //case 0x36: insn_rol(state, read8_zpgx(state)); break;
        //case 0x2E: insn_rol(state, read8_abs(state)); break;
        //case 0x3E: insn_rol(state, read8_absx(state)); break;
        //case 0x6A: insn_ror(state); break;
        //case 0x66: insn_ror(state, read8_zpg(state)); break;
        //case 0x76: insn_ror(state, read8_zpgx(state)); break;
        //case 0x6E: insn_ror(state, read8_abs(state)); break;
        //case 0x7E: insn_ror(state, read8_absx(state)); break;
        //case 0x4D: insn_rti(state); break;
        //case 0x60: insn_rts(state); break;
        //case 0xE9: insn_sbc(state, read8_imm(state)); break;
        //case 0xE5: insn_sbc(state, read8_zpg(state)); break;
        //case 0xF5: insn_sbc(state, read8_zpgx(state)); break;
        //case 0xED: insn_sbc(state, read8_abs(state)); break;
        //case 0xFD: insn_sbc(state, read8_absx(state, state.master_clock_divider)); break;
        //case 0xF9: insn_sbc(state, read8_absy(state, state.master_clock_divider)); break;
        //case 0xE1: insn_sbc(state, read8_indx(state)); break;
        //case 0xF1: insn_sbc(state, read8_indy(state)); break;
        //case 0x38: insn_sec(state); break;
        //case 0xF8: insn_sed(state); break;
        case 0x78: insn_sei(state); break;
        //case 0x85: insn_sta(state, addr_zpg(state)); break;
        //case 0x95: insn_sta(state, addr_zpgx(state)); break;
        case 0x8D: insn_sta(state, addr_abs(state)); break;
        //case 0x9D: insn_sta(state, addr_absx(state)); break;
        //case 0x99: insn_sta(state, addr_absy(state)); break;
        //case 0x81: insn_sta(state, addr_indx(state)); break;
        //case 0x91: insn_sta(state, addr_indy(state)); break;
        //case 0x86: insn_stx(state, addr_zpg(state)); break;
        //case 0x96: insn_stx(state, addr_zpgy(state)); break;
        //case 0x8E: insn_stx(state, addr_abs(state)); break;
        //case 0x84: insn_sty(state, addr_zpg(state)); break;
        //case 0x94: insn_sty(state, addr_zpgx(state)); break;
        //case 0x8C: insn_sty(state, addr_abs(state)); break;
        //case 0xAA: insn_tax(state); break;
        //case 0xA8: insn_tay(state); break;
        //case 0xBA: insn_tsx(state); break;
        //case 0x8A: insn_txa(state); break;
        case 0x9A: insn_txs(state); break;
        //case 0x98: insn_tya(state); break;
        default:
            NOT_IMPLEMENTED("???");
        }
    }
}

Cpu6502::Cpu6502()
    : mDesiredTicks(0)
    , mExecutedTicks(0)
{
    cpu_initialize(mState);
}

Cpu6502::~Cpu6502()
{
    destroy();
}

bool Cpu6502::create(MEMORY_BUS& bus, uint32_t master_clock_divider)
{
    if (!cpu_create(mState, bus, master_clock_divider))
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

void Cpu6502::execute(int32_t numTicks)
{
    // Register timer event for end of execution
    addTimedEvent(executeDummyTimerEvent, nullptr, numTicks);

    // Execute as long as the targer number of ticks is not reached
    do
    {
        // Execute until the first timer event
        int32_t nextDesiredTicks = mTimers.begin()->first;
        int32_t executingTicks = nextDesiredTicks - mDesiredTicks;
        mDesiredTicks = nextDesiredTicks;

        // Let the CPU run until an event occurs or until
        // the desired number of cycles has executed
        cpu_execute(mState, executingTicks);
        mExecutedTicks = mDesiredTicks;

        // Signal events that are ready
        while (!mTimers.empty() && (mTimers.begin()->first <= mDesiredTicks))
        {
            const auto& timerEvent = mTimers.begin()->second;
            timerEvent.callback(timerEvent.context, mTimers.begin()->first);
            mTimers.erase(mTimers.begin());
        }
    } while (mExecutedTicks < numTicks);

    // Advance reference time for next execution
    TimerQueue oldTimers = mTimers;
    mTimers.clear();
    for (auto timerEvent : oldTimers)
    {
        mTimers.insert(std::pair<int32_t, TimerEvent>(timerEvent.first, timerEvent.second));
    }
    mExecutedTicks -= mDesiredTicks;
    mDesiredTicks = 0;
}

uint16_t Cpu6502::disassemble(char* buffer, size_t size, uint16_t addr)
{
    return ::disassemble(mState, addr, buffer, size);
}

void Cpu6502::addTimedEvent(TimerCallback callback, void* context, int32_t ticks)
{
    // Add event to queue
    TimerEvent timerEvent;
    timerEvent.callback = callback;
    timerEvent.context = context;
    mTimers.insert(std::pair<int32_t, TimerEvent>(ticks, timerEvent));

    // Adjust desired ticks
    if (mDesiredTicks > ticks)
    {
        mDesiredTicks = ticks;
        mState.desired_ticks = mDesiredTicks;
    }
}
