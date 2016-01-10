#pragma once

#include <Core/Core.h>
#include <Core/RegisterBank.h>
#include "GB.h"

namespace gb
{
    class CpuZ80;

    class Interrupts
    {
    public:
        // This enum defines the bit mapping in interrupt registers
        enum class Signal : uint32_t
        {
            VBlank,
            LcdStat,
            Timer,
            Serial,
            Joypad,
            INVALID
        };

        Interrupts();
        ~Interrupts();
        bool create(CpuZ80& cpu, emu::RegisterBank& registersIO, emu::RegisterBank& registersIE);
        void destroy();
        void reset();
        void beginFrame(int32_t tick);
        void serialize(emu::ISerializer& serializer);
        void setInterrupt(int32_t tick, Signal signal);
        void clearInterrupt(int32_t tick, Signal signal);

    private:
        uint8_t readIF(int32_t tick, uint16_t addr);
        void writeIF(int32_t tick, uint16_t addr, uint8_t value);
        uint8_t readIE(int32_t tick, uint16_t addr);
        void writeIE(int32_t tick, uint16_t addr, uint8_t value);

        void onCpuInterruptEnable(int32_t tick);
        void onCpuInterruptDisable(int32_t tick);
        void onCpuHaltStop(int32_t tick);
        void checkInterrupts(int32_t tick);

        class CpuInterruptListener;
        struct RegisterAccessors
        {
            struct
            {
                emu::RegisterRead   IE;
                emu::RegisterRead   IF;
            }                       read;
            struct
            {
                emu::RegisterWrite  IE;
                emu::RegisterWrite  IF;
            }                       write;
        };

        gb::CpuZ80*             mCpu;
        CpuInterruptListener*   mInterruptListener;
        RegisterAccessors       mRegisterAccessors;
        uint8_t                 mMask;
        uint8_t                 mRegIE;
        uint8_t                 mRegIF;
    };
}
