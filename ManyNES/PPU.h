#ifndef __PPU_H__
#define __PPU_H__

#include "Clock.h"
#include <stdint.h>

namespace NES
{
    class PPU : public Clock::IListener
    {
    public:
        class IListener
        {
        public:
            virtual void onVBlankStart() {}
        };

        PPU();
        ~PPU();
        bool create(NES::Clock& clock, uint32_t masterClockDivider);
        void destroy();
        void reset();
        void execute();
        void update(void* surface, uint32_t pitch);
        virtual void advanceClock(int32_t ticks);
        virtual void setDesiredTicks(int32_t ticks);
        uint8_t regRead(int32_t ticks, uint32_t addr);
        void regWrite(int32_t ticks, uint32_t addr, uint8_t value);
        void startVBlank();
        void endVBlank();
        void addListener(IListener& listener);
        void removeListener(IListener& listener);

        static const uint32_t PPU_REGISTER_COUNT = 0x8;

        static const uint32_t PPU_REG_PPUCTRL = 0x0;
        static const uint32_t PPU_REG_PPUMASK = 0x1;
        static const uint32_t PPU_REG_PPUSTATUS = 0x2;
        static const uint32_t PPU_REG_OAMADDR = 0x3;
        static const uint32_t PPU_REG_OAMDATA = 0x4;
        static const uint32_t PPU_REG_PPUSCROLL = 0x5;
        static const uint32_t PPU_REG_PPUADDR = 0x6;
        static const uint32_t PPU_REG_PPUDATA = 0x7;

    private:
        typedef std::vector<IListener*> ListenerQueue;

        void signalVBlankStart();
        void onVBlankStart();
        void onVBlankEnd();
        static void onVBlankStart(void* context, int32_t ticks);
        static void onVBlankEnd(void* context, int32_t ticks);

        NES::Clock*     mClock;
        uint32_t        mMasterClockDivider;
        int32_t         mVBlankStartTicks;
        int32_t         mVBlankEndTicks;
        uint8_t         mRegister[PPU_REGISTER_COUNT];
        ListenerQueue   mListeners;
    };
}

#endif
