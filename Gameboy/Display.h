#pragma once

#include <Core/Clock.h>
#include <Core/Core.h>
#include <Core/RegisterBank.h>
#include "GB.h"

namespace gb
{
    class Interrupts;

    class Display
    {
    public:
        struct Config
        {
            Config();

            Model   model;
            bool    fastDma;
            bool    fastLineRendering;
        };

        Display();
        ~Display();
        bool create(Config& config, emu::Clock& clock, uint32_t master_clock_divider, emu::MemoryBus& memory, Interrupts& interrupts, emu::RegisterBank& registers);
        void destroy();
        void reset();
        void serialize(emu::ISerializer& serializer);
        void beginFrame();
        void setRenderSurface(void* surface, size_t pitch);

    private:
        class ClockListener : public emu::Clock::IListener
        {
        public:
            ClockListener();
            ~ClockListener();
            void initialize();
            bool create(emu::Clock& clock, Display& display);
            void destroy();
            virtual void execute() override;
            virtual void resetClock() override;
            virtual void advanceClock(int32_t tick) override;
            virtual void setDesiredTicks(int32_t tick) override;

        private:
            emu::Clock*     mClock;
            Display*        mDisplay;
        };

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

        void initialize();
        void execute();
        void resetClock();
        void advanceClock(int32_t tick);
        void setDesiredTicks(int32_t tick);

        uint8_t readLCDC(int32_t tick, uint16_t addr);
        void writeLCDC(int32_t tick, uint16_t addr, uint8_t value);
        uint8_t readSTAT(int32_t tick, uint16_t addr);
        void writeSTAT(int32_t tick, uint16_t addr, uint8_t value);
        uint8_t readSCY(int32_t tick, uint16_t addr);
        void writeSCY(int32_t tick, uint16_t addr, uint8_t value);
        uint8_t readSCX(int32_t tick, uint16_t addr);
        void writeSCX(int32_t tick, uint16_t addr, uint8_t value);
        uint8_t readLY(int32_t tick, uint16_t addr);
        void writeLY(int32_t tick, uint16_t addr, uint8_t value);
        uint8_t readLYC(int32_t tick, uint16_t addr);
        void writeLYC(int32_t tick, uint16_t addr, uint8_t value);
        void writeDMA(int32_t tick, uint16_t addr, uint8_t value);
        uint8_t readBGP(int32_t tick, uint16_t addr);
        void writeBGP(int32_t tick, uint16_t addr, uint8_t value);
        uint8_t readOBP0(int32_t tick, uint16_t addr);
        void writeOBP0(int32_t tick, uint16_t addr, uint8_t value);
        uint8_t readOBP1(int32_t tick, uint16_t addr);
        void writeOBP1(int32_t tick, uint16_t addr, uint8_t value);
        uint8_t readWY(int32_t tick, uint16_t addr);
        void writeWY(int32_t tick, uint16_t addr, uint8_t value);
        uint8_t readWX(int32_t tick, uint16_t addr);
        void writeWX(int32_t tick, uint16_t addr, uint8_t value);
        void writeOAM(int32_t tick, uint16_t addr, uint8_t value);

        static void onWriteOAM(void* context, int32_t tick, uint32_t addr, uint8_t value)
        {
            static_cast<Display*>(context)->writeOAM(tick, addr, value);
        }

        static void onWriteNotUsable(void* context, int32_t tick, uint32_t addr, uint8_t value)
        {
        }

        void onVBlankStart(int32_t tick);
        void updateRasterPos(int32_t tick);
        uint8_t getMode(int32_t tick);
        void updateLineInterrupt(int32_t tick);
        void updateMonoPalette(uint32_t base, uint8_t value);
        void sortMonoSprites();
        void fetchTileRow(uint8_t* dest, const uint8_t* map, uint32_t tileX, uint32_t tileY, uint8_t tileOffset, uint32_t count);
        void drawTiles(uint8_t* dest, const uint8_t* tiles, const uint8_t* attributes, const uint8_t* patterns, uint16_t count);
        void drawSpritesMono(uint8_t* dest, uint8_t line, uint8_t spriteSizeY);
        void blitLine(uint32_t* dest, uint8_t* src, uint32_t count);
        void renderLinesMono(uint32_t firstLine, uint32_t lastLine);
        void render(int32_t tick);

        static void onVBlankStart(void* context, int32_t tick)
        {
            static_cast<Display*>(context)->onVBlankStart(tick);
        }

        emu::Clock*             mClock;
        MEMORY_BUS*             mMemory;
        Interrupts*             mInterrupts;
        Config                  mConfig;
        ClockListener           mClockListener;
        RegisterAccessors       mRegisterAccessors;
        MEM_ACCESS_READ_WRITE   mMemoryVRAM;
        MEM_ACCESS_READ_WRITE   mMemoryOAM;
        MEM_ACCESS_READ_WRITE   mMemoryNotUsable;
        std::vector<uint8_t>    mVRAM;
        std::vector<uint8_t>    mOAM;
        std::vector<uint8_t>    mOAMOrder;
        std::vector<uint32_t>   mPalette;
        uint8_t*                mSurface;
        size_t                  mPitch;
        int32_t                 mSimulatedTick;
        uint8_t                 mRenderedLine;
        uint8_t                 mRenderedLineFirstTick;
        int32_t                 mRenderedTick;
        int32_t                 mDesiredTick;
        int32_t                 mTicksPerLine;
        int32_t                 mVBlankStartTick;
        int32_t                 mMode0StartTick;
        int32_t                 mMode3StartTick;
        int32_t                 mUpdateRasterPosFast;
        int32_t                 mLineFirstTick;
        int32_t                 mLineTick;
        int32_t                 mLineIntTick;
        uint8_t                 mRasterLine;
        uint8_t                 mBankVRAM;
        uint8_t                 mRegLCDC;
        uint8_t                 mRegSTAT;
        uint8_t                 mRegSCY;
        uint8_t                 mRegSCX;
        uint8_t                 mRegLY;
        uint8_t                 mRegLYC;
        uint8_t                 mRegDMA;
        uint8_t                 mRegBGP;
        uint8_t                 mRegOBP0;
        uint8_t                 mRegOBP1;
        uint8_t                 mRegWY;
        uint8_t                 mRegWX;
        uint8_t                 mActiveSprites;
        uint8_t                 mLineIntLastLY;
        uint8_t                 mLineIntLastLYC;
        bool                    mSortedSprites;
        bool                    mLineIntLastEnabled;
    };
}
