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
        void serialize(emu::ISerializer& serializer);
        void setInterrupt(int tick, Signal signal);
        void clearInterrupt(int tick, Signal signal);

    private:
        uint8_t readIF(int32_t tick, uint16_t addr);
        void writeIF(int32_t tick, uint16_t addr, uint8_t value);
        uint8_t readIE(int32_t tick, uint16_t addr);
        void writeIE(int32_t tick, uint16_t addr, uint8_t value);

        static uint8_t readIF(void* context, int32_t tick, uint16_t addr)
        {
            return static_cast<Interrupts*>(context)->readIF(tick, addr);
        }

        static void writeIF(void* context, int32_t tick, uint16_t addr, uint8_t value)
        {
            static_cast<Interrupts*>(context)->writeIF(tick, addr, value);
        }

        static uint8_t readIE(void* context, int32_t tick, uint16_t addr)
        {
            return static_cast<Interrupts*>(context)->readIE(tick, addr);
        }

        static void writeIE(void* context, int32_t tick, uint16_t addr, uint8_t value)
        {
            static_cast<Interrupts*>(context)->writeIE(tick, addr, value);
        }

        void onCpuInterruptEnable(int32_t tick);
        void onCpuInterruptDisable(int32_t tick);
        void checkInterrupts(int32_t tick);

        class CpuInterruptListener;

        gb::CpuZ80*             mCpu;
        CpuInterruptListener*   mInterruptListener;
        uint8_t                 mMask;
        uint8_t                 mRegIE;
        uint8_t                 mRegIF;
    };
}
