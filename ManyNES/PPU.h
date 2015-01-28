#ifndef __PPU_H__
#define __PPU_H__

#include "Clock.h"
#include <stdint.h>

namespace NES
{
    class PPU : public Clock::IListener
    {
    public:
        PPU();
        ~PPU();
        bool create(uint32_t masterClockDivider);
        void destroy();
        void reset();
        void execute();
        void update(void* surface, uint32_t pitch);
        virtual void advanceClock(int32_t ticks);
        virtual void setDesiredTicks(int32_t ticks);
        uint8_t regRead(int32_t ticks, uint32_t addr);
        void regWrite(int32_t ticks, uint32_t addr, uint8_t value);
        void startVBlank();

    private:
        uint32_t    mMasterClockDivider;
        int32_t     mVBlankStartTicks;
        int32_t     mVBlankEndTicks;
        uint8_t     mRegPPUCTRL;
        uint8_t     mRegPPUSTATUS;
        bool        mVBlankStatusDirty;
    };
}

#endif
