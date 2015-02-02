#ifndef __PPU_H__
#define __PPU_H__

#include "Clock.h"
#include "MemoryBus.h"
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
        bool create(NES::Clock& clock, uint32_t masterClockDivider, uint32_t createFlags);
        void destroy();
        void reset();
        void execute();
        MEM_ACCESS* getPatternTableRead(uint32_t index);
        MEM_ACCESS* getPatternTableWrite(uint32_t index);
        MEM_ACCESS* getNameTableRead(uint32_t index);
        MEM_ACCESS* getNameTableWrite(uint32_t index);
        virtual void advanceClock(int32_t ticks);
        virtual void setDesiredTicks(int32_t ticks);
        uint8_t regRead(int32_t ticks, uint32_t addr);
        void regWrite(int32_t ticks, uint32_t addr, uint8_t value);
        void startVBlank();
        void endVBlank();
        void addListener(IListener& listener);
        void removeListener(IListener& listener);
        void setRenderSurface(void* surface, size_t pitch);

        static const uint32_t CREATE_VRAM_ONE_SCREEN = 0x00000001;
        static const uint32_t CREATE_VRAM_HORIZONTAL_MIRROR = 0x00000002;
        static const uint32_t CREATE_VRAM_VERTICAL_MIRROR = 0x00000004;
        static const uint32_t CREATE_VRAM_FOUR_SCREEN = 0x00000008;
        static const uint32_t CREATE_VRAM_MASK = 0x0000000f;
        static const uint32_t CREATE_PATTERN_TABLE_RAM = 0x00000010;

        static const uint32_t PATTERN_TABLE_COUNT = 2;
        static const uint32_t NAME_TABLE_COUNT = 4;

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
        void render(int32_t lastTick);

        NES::Clock*             mClock;
        uint32_t                mMasterClockDivider;
        int32_t                 mVBlankStartTicks;
        int32_t                 mVBlankEndTicks;
        int32_t                 mTicksPerLine;
        uint8_t                 mRegister[PPU_REGISTER_COUNT];
        uint16_t                mAddress;
        bool                    mAddessLow;
        ListenerQueue           mListeners;
        NES::MemoryBus          mMemory;
        MEM_ACCESS              mPatternTableRead[PATTERN_TABLE_COUNT];
        MEM_ACCESS              mPatternTableWrite[PATTERN_TABLE_COUNT];
        MEM_ACCESS              mNameTableRead[NAME_TABLE_COUNT];
        MEM_ACCESS              mNameTableWrite[NAME_TABLE_COUNT];
        std::vector<uint8_t>    mNameTableRAM;
        std::vector<uint8_t>    mOAM;
        uint8_t*                mSurface;
        size_t                  mPitch;
        int32_t                 mLastTickRendered;
    };
}

#endif
