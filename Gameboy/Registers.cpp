#include <Core/MemoryBus.h>
#include <Core/Serialization.h>
#include "Registers.h"
#include <memory.h>
#include <stdio.h>

namespace
{
    enum ACCESS
    {
        ACCESS_NONE,
        ACCESS_R,
        ACCESS_W,
        ACCESS_RW,
    };

    struct RegisterInfo
    {
        ACCESS          access;
        const char*     name;
        const char*     description;
    };

    static RegisterInfo registerInfoTable[gb::Registers::Count + 1];

    void defineRegister(gb::REG reg, ACCESS access, const char* name, const char* description)
    {
        auto index = gb::Registers::getRegisterIndex(reg);
        registerInfoTable[index] = { access, name, description };
    }

    RegisterInfo* initialize()
    {
        for (uint32_t index = 0; index < gb::Registers::Count; ++index)
            registerInfoTable[index] = { ACCESS_NONE, "???", "Invalid register" };

        defineRegister(gb::REG::JOYP,   ACCESS_RW,  "JOYP",     "Joypad");
        defineRegister(gb::REG::SB,     ACCESS_RW,  "SB",       "Serial transfer data");
        defineRegister(gb::REG::SC,     ACCESS_RW,  "SC",       "Serial Transfer Control");
        defineRegister(gb::REG::DIV,    ACCESS_RW,  "DIV",      "Divider Register");
        defineRegister(gb::REG::TIMA,   ACCESS_RW,  "TIMA",     "Timer counter");
        defineRegister(gb::REG::TMA,    ACCESS_RW,  "TMA",      "Timer Modulo");
        defineRegister(gb::REG::TAC,    ACCESS_RW,  "TAC",      "Timer Control");
        defineRegister(gb::REG::IF,     ACCESS_RW,  "IF",       "Interrupt Flag");
        defineRegister(gb::REG::NR10,   ACCESS_RW,  "NR10",     "Channel 1 Sweep register");
        defineRegister(gb::REG::NR11,   ACCESS_RW,  "NR11",     "Channel 1 Sound length/Wave pattern duty");
        defineRegister(gb::REG::NR12,   ACCESS_RW,  "NR12",     "Channel 1 Volume Envelope");
        defineRegister(gb::REG::NR13,   ACCESS_W,   "NR13",     "Channel 1 Frequency lo");
        defineRegister(gb::REG::NR14,   ACCESS_RW,  "NR14",     "Channel 1 Frequency hi");
        defineRegister(gb::REG::NR21,   ACCESS_RW,  "NR21",     "Channel 2 Sound Length/Wave Pattern Duty");
        defineRegister(gb::REG::NR22,   ACCESS_RW,  "NR22",     "Channel 2 Volume Envelope");
        defineRegister(gb::REG::NR23,   ACCESS_W,   "NR23",     "Channel 2 Frequency lo data");
        defineRegister(gb::REG::NR24,   ACCESS_RW,  "NR24",     "Channel 2 Frequency hi data");
        defineRegister(gb::REG::NR30,   ACCESS_RW,  "NR30",     "Channel 3 Sound on/off");
        defineRegister(gb::REG::NR31,   ACCESS_RW,  "NR31",     "Channel 3 Sound Length");
        defineRegister(gb::REG::NR32,   ACCESS_RW,  "NR32",     "Channel 3 Select output level");
        defineRegister(gb::REG::NR33,   ACCESS_W,   "NR33",     "Channel 3 Frequency's lower data");
        defineRegister(gb::REG::NR34,   ACCESS_RW,  "NR34",     "Channel 3 Frequency's higher data");
        defineRegister(gb::REG::NR41,   ACCESS_RW,  "NR41",     "Channel 4 Sound Length");
        defineRegister(gb::REG::NR42,   ACCESS_RW,  "NR42",     "Channel 4 Volume Envelope");
        defineRegister(gb::REG::NR43,   ACCESS_RW,  "NR43",     "Channel 4 Polynomial Counter");
        defineRegister(gb::REG::NR44,   ACCESS_RW,  "NR44",     "Channel 4 Counter/consecutive; Inital");
        defineRegister(gb::REG::NR50,   ACCESS_RW,  "NR50",     "Channel control / ON-OFF / Volume");
        defineRegister(gb::REG::NR51,   ACCESS_RW,  "NR51",     "Selection of Sound output terminal");
        defineRegister(gb::REG::NR52,   ACCESS_RW,  "NR52",     "Sound on/off");
        defineRegister(gb::REG::LCDC,   ACCESS_RW,  "LCDC",     "LCD Control");
        defineRegister(gb::REG::STAT,   ACCESS_RW,  "STAT",     "LCDC Status");
        defineRegister(gb::REG::SCY,    ACCESS_RW,  "SCY",      "Scroll Y");
        defineRegister(gb::REG::SCX,    ACCESS_RW,  "SCX",      "Scroll X");
        defineRegister(gb::REG::LY,     ACCESS_R,   "LY",       "LCDC Y-Coordinate");
        defineRegister(gb::REG::LYC,    ACCESS_RW,  "LYC",      "LY Compare");
        defineRegister(gb::REG::DMA,    ACCESS_W,   "DMA",      "DMA Transfer and Start Address");
        defineRegister(gb::REG::BGP,    ACCESS_RW,  "BGP",      "BG Palette Data");
        defineRegister(gb::REG::OBP0,   ACCESS_RW,  "OBP0",     "Object Palette 0 Data");
        defineRegister(gb::REG::OBP1,   ACCESS_RW,  "OBP1",     "Object Palette 1 Data");
        defineRegister(gb::REG::WY,     ACCESS_RW,  "WY",       "Window Y Position");
        defineRegister(gb::REG::WX,     ACCESS_RW,  "WX",       "Window X Position minus 7");
        defineRegister(gb::REG::KEY1,   ACCESS_RW,  "KEY1",     "CGB Mode Only - Prepare Speed Switch");
        defineRegister(gb::REG::VBK,    ACCESS_RW,  "VBK",      "CGB Mode Only - VRAM Bank");
        defineRegister(gb::REG::HDMA1,  ACCESS_RW,  "HDMA1",    "CGB Mode Only - New DMA Source, High");
        defineRegister(gb::REG::HDMA2,  ACCESS_RW,  "HDMA2",    "CGB Mode Only - New DMA Source, Low");
        defineRegister(gb::REG::HDMA3,  ACCESS_RW,  "HDMA3",    "CGB Mode Only - New DMA Destination, High");
        defineRegister(gb::REG::HDMA4,  ACCESS_RW,  "HDMA4",    "CGB Mode Only - New DMA Destination, Low");
        defineRegister(gb::REG::HDMA5,  ACCESS_RW,  "HDMA5",    "CGB Mode Only - New DMA Length/Mode/Start");
        defineRegister(gb::REG::RP,     ACCESS_RW,  "RP",       "CGB Mode Only - Infrared Communications Port");
        defineRegister(gb::REG::BGPI,   ACCESS_RW,  "BGPI",     "CGB Mode Only - Background Palette Index");
        defineRegister(gb::REG::BGPD,   ACCESS_RW,  "BGPD",     "CGB Mode Only - Background Palette Data");
        defineRegister(gb::REG::OBPI,   ACCESS_RW,  "OBPI",     "CGB Mode Only - Sprite Palette Index");
        defineRegister(gb::REG::OBPD,   ACCESS_RW,  "OBPD",     "CGB Mode Only - Sprite Palette Data");
        defineRegister(gb::REG::UND6C,  ACCESS_RW,  "UND6C",    "Bit 0: Read/Write - CGB Mode Only");
        defineRegister(gb::REG::SVBK,   ACCESS_RW,  "SVBK",     "CGB Mode Only - WRAM Bank");
        defineRegister(gb::REG::UND72,  ACCESS_RW,  "UND72",    "Bit 0-7: Read/Write");
        defineRegister(gb::REG::UND73,  ACCESS_RW,  "UND73",    "Bit 0-7: Read/Write");
        defineRegister(gb::REG::UND74,  ACCESS_RW,  "UND74",    "Bit 0-7: Read/Write - CGB Mode Only");
        defineRegister(gb::REG::UND75,  ACCESS_RW,  "UND75",    "Bit 4-6: Read/Write");
        defineRegister(gb::REG::UND76,  ACCESS_R,   "UND76",    "Always 00h");
        defineRegister(gb::REG::UND77,  ACCESS_R,   "UND77",    "Always 00h");
        defineRegister(gb::REG::IE,     ACCESS_RW,  "IE",       "Interrupt Enable");
        return registerInfoTable;
    }

    const RegisterInfo& getRegisterInfo(uint32_t index)
    {
        static RegisterInfo* table = initialize();
        return table[index];        
    }
}

namespace gb
{
    void Registers::reset(bool isSGB)
    {
        memset(values, 0, sizeof(values));

        values[0x05] = 0x00;
        values[0x06] = 0x00;
        values[0x07] = 0x00;
        values[0x10] = 0x80;
        values[0x11] = 0xBF;
        values[0x12] = 0xF3;
        values[0x14] = 0xBF;
        values[0x16] = 0x3F;
        values[0x17] = 0x00;
        values[0x19] = 0xBF;
        values[0x1A] = 0x7F;
        values[0x1B] = 0xFF;
        values[0x1C] = 0x9F;
        values[0x1E] = 0xBF;
        values[0x20] = 0xFF;
        values[0x21] = 0x00;
        values[0x22] = 0x00;
        values[0x23] = 0xBF;
        values[0x24] = 0x77;
        values[0x25] = 0xF3;
        values[0x26] = !isSGB ? 0xF1 : 0xF0;
        values[0x40] = 0x91;
        values[0x42] = 0x00;
        values[0x43] = 0x00;
        values[0x45] = 0x00;
        values[0x47] = 0xFC;
        values[0x48] = 0xFF;
        values[0x49] = 0xFF;
        values[0x4A] = 0x00;
        values[0x4B] = 0x00;
        values[0xFF] = 0x00;
    }

    void Registers::serialize(emu::ISerializer& serializer)
    {
        serializer.serialize(values, EMU_ARRAY_SIZE(values));
    }

    uint32_t Registers::getRegisterIndex(REG reg)
    {
        uint32_t index = static_cast<uint32_t>(reg);
        if (index == 0xff)
            index = gb::Registers::Count - 1;
        return index;
    }

    const char* Registers::getName(REG reg)
    {
        auto index = getRegisterIndex(reg);
        return getRegisterInfo(index).name;
    }

    const char* Registers::getDescription(REG reg)
    {
        auto index = getRegisterIndex(reg);
        return getRegisterInfo(index).description;
    }

    uint8_t Registers::readIO(int32_t ticks, uint32_t addr)
    {
        return values[addr & 0x7f];
    }

    void Registers::writeIO(int32_t ticks, uint32_t addr, uint8_t value)
    {
        values[addr & 0x7f] = value;
    }

    uint8_t Registers::readIE(int32_t ticks)
    {
        return values[0x80];
    }

    void Registers::writeIE(int32_t ticks, uint8_t value)
    {
        values[0x80] = value;
    }
}
