#pragma once

#include <Core/Core.h>
#include <Core/RegisterBank.h>
#include "GB.h"

namespace gb
{
    class Interrupts;

    class Display
    {
    public:
        Display();
        ~Display();
        bool create(Interrupts& interrupts, emu::RegisterBank& registers);
        void destroy();
        void reset();
        void execute();
        void serialize(emu::ISerializer& serializer);

    private:
        uint8_t readLCDC(int32_t ticks, uint16_t addr);
        void writeLCDC(int32_t ticks, uint16_t addr, uint8_t value);
        uint8_t readSTAT(int32_t ticks, uint16_t addr);
        void writeSTAT(int32_t ticks, uint16_t addr, uint8_t value);
        uint8_t readSCY(int32_t ticks, uint16_t addr);
        void writeSCY(int32_t ticks, uint16_t addr, uint8_t value);
        uint8_t readSCX(int32_t ticks, uint16_t addr);
        void writeSCX(int32_t ticks, uint16_t addr, uint8_t value);
        uint8_t readLY(int32_t ticks, uint16_t addr);
        void writeLY(int32_t ticks, uint16_t addr, uint8_t value);
        uint8_t readLYC(int32_t ticks, uint16_t addr);
        void writeLYC(int32_t ticks, uint16_t addr, uint8_t value);
        void writeDMA(int32_t ticks, uint16_t addr, uint8_t value);
        uint8_t readBGP(int32_t ticks, uint16_t addr);
        void writeBGP(int32_t ticks, uint16_t addr, uint8_t value);
        uint8_t readOBP0(int32_t ticks, uint16_t addr);
        void writeOBP0(int32_t ticks, uint16_t addr, uint8_t value);
        uint8_t readOBP1(int32_t ticks, uint16_t addr);
        void writeOBP1(int32_t ticks, uint16_t addr, uint8_t value);
        uint8_t readWY(int32_t ticks, uint16_t addr);
        void writeWY(int32_t ticks, uint16_t addr, uint8_t value);
        uint8_t readWX(int32_t ticks, uint16_t addr);
        void writeWX(int32_t ticks, uint16_t addr, uint8_t value);

        struct RegisterAccessors
        {
            struct
            {
                emu::RegisterRead   LCDC;
                emu::RegisterRead   STAT;
                emu::RegisterRead   SCY;
                emu::RegisterRead   SCX;
                emu::RegisterRead   LY;
                emu::RegisterRead   LYC;
                emu::RegisterRead   BGP;
                emu::RegisterRead   OBP0;
                emu::RegisterRead   OBP1;
                emu::RegisterRead   WY;
                emu::RegisterRead   WX;
            }                       read;

            struct
            {
                emu::RegisterWrite  LCDC;
                emu::RegisterWrite  STAT;
                emu::RegisterWrite  SCY;
                emu::RegisterWrite  SCX;
                emu::RegisterWrite  LY;
                emu::RegisterWrite  LYC;
                emu::RegisterWrite  DMA;
                emu::RegisterWrite  BGP;
                emu::RegisterWrite  OBP0;
                emu::RegisterWrite  OBP1;
                emu::RegisterWrite  WY;
                emu::RegisterWrite  WX;
            }                       write;
        };

        RegisterAccessors   mRegisterAccessors;
        uint8_t             mRegLCDC;
        uint8_t             mRegSTAT;
        uint8_t             mRegSCY;
        uint8_t             mRegSCX;
        uint8_t             mRegLY;
        uint8_t             mRegLYC;
        uint8_t             mRegDMA;
        uint8_t             mRegBGP;
        uint8_t             mRegOBP0;
        uint8_t             mRegOBP1;
        uint8_t             mRegWY;
        uint8_t             mRegWX;
    };
}
