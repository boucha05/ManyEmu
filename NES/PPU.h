#ifndef __PPU_H__
#define __PPU_H__

#include <Core/Clock.h>
#include <Core/MemoryBus.h>
#include <stdint.h>

namespace emu
{
    class ISerializer;
}

namespace nes
{
    class PPU : public emu::Clock::IListener
    {
    public:
        class IListener
        {
        public:
            virtual void onVBlankStart() {}
            virtual void onVisibleLineStart(int32_t tick) { EMU_UNUSED(tick); }
        };

        PPU();
        ~PPU();
        bool create(emu::Clock& clock, uint32_t masterClockDivider, uint32_t createFlags, uint32_t visibleLines);
        void destroy();
        void reset();
        void beginFrame();
        virtual void execute() override;
        emu::MemoryBus& getMemory();
        uint8_t* getNameTableMemory();
        MEM_ACCESS* getPatternTableRead(uint32_t index);
        MEM_ACCESS* getPatternTableWrite(uint32_t index);
        MEM_ACCESS* getNameTableRead(uint32_t index);
        MEM_ACCESS* getNameTableWrite(uint32_t index);
        virtual void resetClock() override;
        virtual void advanceClock(int32_t ticks) override;
        virtual void setDesiredTicks(int32_t ticks) override;
        uint8_t regRead(int32_t ticks, uint32_t addr);
        void regWrite(int32_t ticks, uint32_t addr, uint8_t value);
        void startVBlank();
        void endVBlank();
        void addListener(IListener& listener);
        void removeListener(IListener& listener);
        void setRenderSurface(void* surface, size_t pitch);
        void startFrame();
        int32_t getTickCount(uint32_t lines, uint32_t dots);
        void serialize(emu::ISerializer& serializer);

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

        void initialize();
        uint8_t paletteRead(int32_t ticks, uint32_t addr);
        void paletteWrite(int32_t ticks, uint32_t addr, uint8_t value);
        static uint8_t paletteRead(void* context, int32_t ticks, uint32_t addr);
        static void paletteWrite(void* context, int32_t ticks, uint32_t addr, uint8_t value);
        void signalVBlankStart();
        void signalVisibleLineStart(int32_t tick);
        void onVBlankStart(int32_t ticks);
        void onVBlankEnd();
        static void onVBlankStart(void* context, int32_t ticks);
        static void onVBlankEnd(void* context, int32_t ticks);
        void getRasterPosition(int32_t tick, int32_t& x, int32_t& y);
        void fetchPalette(uint8_t* dest);
        void fetchAttributes(uint8_t* dest1, uint8_t* dest2, uint16_t base, uint16_t size);
        void fetchNames(uint8_t* dest, uint16_t base, uint16_t size);
        void drawBackground(uint8_t* dest, const uint8_t* names, const uint8_t* attributes, uint16_t base, uint16_t size);
        void drawSprites8(uint8_t* dest, uint32_t y);
        void drawSprites16(uint8_t* dest, uint32_t y);
        void applyPalette(uint8_t* dest, const uint8_t* palette, uint32_t count);
        void blitSurface(uint32_t* dest, const uint8_t* src, uint32_t count);
        void render(int32_t lastTick);
        void updateSpriteHitTestConditions();
        void checkHitTest(int32_t tick);
        void advanceFrame(int32_t tick);
        void addressDirty();

        struct ScanlineEvent
        {
            int32_t     offsetTick;
            uint32_t    action;

            static ScanlineEvent make(int32_t offsetTick, uint32_t action)
            {
                ScanlineEvent instance;
                instance.offsetTick = offsetTick;
                instance.action = action;
                return instance;
            }
        };

        typedef std::vector<ScanlineEvent> ScanlineEventTable;

        static const uint32_t SCANLINE_TYPE_PRESCAN = 0;
        static const uint32_t SCANLINE_TYPE_VISIBLE = 1;
        static const uint32_t SCANLINE_TYPE_VBLANK = 2;
        static const uint32_t SCANLINE_TYPE_COUNT = 3;

        emu::Clock*             mClock;
        uint32_t                mMasterClockDivider;
        int32_t                 mVBlankStartTicks;
        int32_t                 mVBlankEndTicks;
        int32_t                 mTicksPerLine;
        int32_t                 mSprite0StartTick;
        int32_t                 mSprite0EndTick;
        uint32_t                mVisibleLines;
        uint8_t                 mRegister[PPU_REGISTER_COUNT];
        uint8_t                 mDataReadBuffer;
        uint32_t                mInternalAddress;
        uint32_t                mScanlineAddress;
        uint32_t                mFineX;
        bool                    mWriteToggle;
        bool                    mVisibleArea;
        bool                    mCheckHitTest;
        ListenerQueue           mListeners;
        emu::MemoryBus          mMemory;
        MEM_ACCESS              mPatternTableRead[PATTERN_TABLE_COUNT];
        MEM_ACCESS              mPatternTableWrite[PATTERN_TABLE_COUNT];
        MEM_ACCESS              mNameTableRead[NAME_TABLE_COUNT];
        MEM_ACCESS              mNameTableWrite[NAME_TABLE_COUNT];
        MEM_ACCESS              mPaletteRead;
        MEM_ACCESS              mPaletteWrite;
        ScanlineEventTable      mScanlineEvents[SCANLINE_TYPE_COUNT];
        ScanlineEventTable      mScanlineEventsVisible;
        ScanlineEventTable      mScanlineEventsVBlank;
        emu::Buffer             mNameTableRAM;
        emu::Buffer             mPaletteRAM;
        emu::Buffer             mOAM;
        uint8_t*                mSurface;
        size_t                  mPitch;
        int32_t                 mLastTickRendered;
        int32_t                 mLastTickUpdated;
        int32_t                 mScanlineNumber;
        int32_t                 mScanlineBaseTick;
        int32_t                 mScanlineOffsetTick;
        uint32_t                mScanlineType;
        uint32_t                mScanlineEventIndex;
    };
}

#endif
