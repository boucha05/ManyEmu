#pragma once

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

    /*
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
    */
}
