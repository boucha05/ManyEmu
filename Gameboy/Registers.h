#pragma once

#include <Core/Clock.h>
#include <Core/Core.h>
#include <stdint.h>
#include <map>

namespace gb
{
    enum REG
    {
        JOYP  = 0x00,
        SB    = 0x01,
        SC    = 0x02,
        DIV   = 0x04,
        TIMA  = 0x05,
        TMA   = 0x06,
        TAC   = 0x07,
        IF    = 0x0F,
        NR10  = 0x10,
        NR11  = 0x11,
        NR12  = 0x12,
        NR13  = 0x13,
        NR14  = 0x14,
        NR21  = 0x16,
        NR22  = 0x17,
        NR23  = 0x18,
        NR24  = 0x19,
        NR30  = 0x1A,
        NR31  = 0x1B,
        NR32  = 0x1C,
        NR33  = 0x1D,
        NR34  = 0x1E,
        NR41  = 0x20,
        NR42  = 0x21,
        NR43  = 0x22,
        NR44  = 0x23,
        NR50  = 0x24,
        NR51  = 0x25,
        NR52  = 0x26,
        LCDC  = 0x40,
        STAT  = 0x41,
        SCY   = 0x42,
        SCX   = 0x43,
        LY    = 0x44,
        LYC   = 0x45,
        DMA   = 0x46,
        BGP   = 0x47,
        OBP0  = 0x48,
        OBP1  = 0x49,
        WY    = 0x4A,
        WX    = 0x4B,
        KEY1  = 0x4D,
        VBK   = 0x4F,
        HDMA1 = 0x51,
        HDMA2 = 0x52,
        HDMA3 = 0x53,
        HDMA4 = 0x54,
        HDMA5 = 0x55,
        RP    = 0x56,
        BGPI  = 0x68,
        BGPD  = 0x69,
        OBPI  = 0x6A,
        OBPD  = 0x6B,
        UND6C = 0x6C,
        SVBK  = 0x70,
        UND72 = 0x72,
        UND73 = 0x73,
        UND74 = 0x74,
        UND75 = 0x75,
        UND76 = 0x76,
        UND77 = 0x77,
        IE    = 0xFF,
    };

    struct Registers
    {
        static const uint32_t Count = 0x81;

        uint8_t     values[Count];

        void reset(bool isSGB);
        void serialize(emu::ISerializer& serializer);
        uint8_t readIO(int32_t ticks, uint32_t addr);
        void writeIO(int32_t ticks, uint32_t addr, uint8_t value);
        uint8_t readIE(int32_t ticks);
        void writeIE(int32_t ticks, uint8_t value);

        static uint32_t getRegisterIndex(REG reg);
        static const char* getName(REG reg);
        static const char* getDescription(REG reg);
    };
}
