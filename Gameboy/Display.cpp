#include <Core/RegisterBank.h>
#include "Display.h"
#include "Interrupts.h"
#include <algorithm>

namespace
{
    static const uint32_t VRAM_BANK_SIZE = 0x2000;
    static const uint32_t VRAM_BANK_START = 0x8000;
    static const uint32_t VRAM_BANK_END = VRAM_BANK_START + VRAM_BANK_SIZE - 1;
    static const uint32_t VRAM_SIZE_GB = 0x2000;
    static const uint32_t VRAM_SIZE_GBC = 0x4000;

    static const uint32_t OAM_SIZE = 0xa0;

    static const uint8_t LCDC_LCD_ENABLE = 0x80;
    static const uint8_t LCDC_WINDOW_TILE_MAP = 0x40;
    static const uint8_t LCDC_WINDOW_ENABLE = 0x20;
    static const uint8_t LCDC_TILE_DATA = 0x10;
    static const uint8_t LCDC_BG_TILE_MAP = 0x08;
    static const uint8_t LCDC_SPRITES_SIZE = 0x04;
    static const uint8_t LCDC_SPRITES_ENABLE = 0x02;
    static const uint8_t LCDC_BG_ENABLE = 0x01;

    static const uint8_t STAT_LYC_LY_INT = 0x40;
    static const uint8_t STAT_OAM_INT = 0x20;
    static const uint8_t STAT_VBLANK_INT = 0x10;
    static const uint8_t STAT_HBLANK_INT = 0x08;
    static const uint8_t STAT_LYC_LY_STATUS = 0x04;
    static const uint8_t STAT_MODE_MASK = 0x03;
    static const uint8_t STAT_MODE_SHIFT = 0x00;
    static const uint8_t STAT_WRITE_MASK = STAT_LYC_LY_INT | STAT_OAM_INT | STAT_VBLANK_INT | STAT_HBLANK_INT;

    static const uint8_t DMA_SIZE = 0xa0;

    static const uint32_t TICKS_PER_LINE = 456;
    static const uint32_t UPDATE_RASTER_POS_FAST = TICKS_PER_LINE * 8;

    static const uint32_t DISPLAY_VBLANK_SIZE = 10;
    static const uint32_t DISPLAY_SIZE_X = 160;
    static const uint32_t DISPLAY_SIZE_Y = 144;
    static const uint32_t DISPLAY_LINE_COUNT = DISPLAY_SIZE_Y + DISPLAY_VBLANK_SIZE;

    static const uint32_t MODE0_START_TICK = 252;
    static const uint32_t MODE1_START_TICK = TICKS_PER_LINE * DISPLAY_SIZE_Y;
    static const uint32_t MODE2_START_TICK = 0;
    static const uint32_t MODE3_START_TICK = 80;

    static const uint32_t RENDER_ROW_STORAGE = DISPLAY_SIZE_X + 16;
    static const uint32_t RENDER_TILE_COUNT = DISPLAY_SIZE_X / 8;
    static const uint32_t RENDER_TILE_STORAGE = RENDER_TILE_COUNT + 1;

    static const uint32_t PALETTE_BASE_BGP = 0;
    static const uint32_t PALETTE_BASE_OBP = 0x20;
    static const uint32_t PALETTE_SIZE = 64;

    static const uint32_t PALETTE_MONO_BASE_BGP = PALETTE_BASE_BGP;
    static const uint32_t PALETTE_MONO_BASE_OBP0 = PALETTE_BASE_OBP;
    static const uint32_t PALETTE_MONO_BASE_OBP1 = PALETTE_BASE_OBP + 4;

    static const uint32_t SPRITE_LINE_LIMIT = 10;
    static const uint32_t SPRITE_CAPACITY = 40;
    static const uint32_t SPRITE_VISIBLE_X_BEGIN = 8;
    static const uint32_t SPRITE_VISIBLE_X_END = SPRITE_VISIBLE_X_BEGIN + DISPLAY_SIZE_X;
    static const uint32_t SPRITE_VISIBLE_Y_BEGIN = 16;
    static const uint32_t SPRITE_VISIBLE_Y_END = SPRITE_VISIBLE_Y_BEGIN + DISPLAY_SIZE_Y;
    static const uint8_t SPRITE_FLAG_BACKGROUND = 0x80;
    static const uint8_t SPRITE_FLAG_FLIP_Y = 0x40;
    static const uint8_t SPRITE_FLAG_FLIP_X = 0x20;
    static const uint8_t SPRITE_FLAG_MONO_PALETTE_MASK = 0x10;
    static const uint8_t SPRITE_FLAG_MONO_PALETTE_SHIFT = 4;
    static const uint8_t SPRITE_FLAG_COLOR_BANK_MASK = 0x08;
    static const uint8_t SPRITE_FLAG_COLOR_BANK_SHIFT = 3;
    static const uint8_t SPRITE_FLAG_COLOR_PALETTE_MASK = 0x07;
    static const uint8_t SPRITE_FLAG_COLOR_PALETTE_SHIFT = 0;

    static const uint8_t VBK_MASK = 0x01;

    static const uint8_t BGPI_INDEX_MASK = 0x3f;
    static const uint8_t BGPI_AUTO_INCR = 0x80;

    static const uint8_t OBPI_INDEX_MASK = 0x3f;
    static const uint8_t OBPI_AUTO_INCR = 0x80;

    static const uint8_t HDMA5_SIZE_MASK = 0x7f;
    static const uint8_t HDMA5_HBLANK = 0x80;
    static const uint8_t HDMA5_DONE = 0xff;
    static const uint16_t HDMA_DST_BASE = 0x8000;
    static const uint16_t HDMA_DST_MASK = 0x1ff0;
    static const uint16_t HDMA_DST_FINAL_MASK = 0x1fff;

    static const uint8_t INT_MODE_0 = 0x01;
    static const uint8_t INT_MODE_1 = 0x02;
    static const uint8_t INT_MODE_2 = 0x04;
    static const uint8_t INT_LINE = 0x08;
    static const uint8_t INT_VBLANK = 0x10;
    static const uint8_t INT_ALL = INT_MODE_0 | INT_MODE_1 | INT_MODE_2 | INT_LINE | INT_VBLANK;
    static const uint8_t INT_NONE = 0x00;

    static const uint8_t kMonoPalette[4][4] =
    {
        { 0xff, 0xff, 0xff, 0xff },
        { 0xbb, 0xbb, 0xbb, 0xff },
        { 0x55, 0x55, 0x55, 0xff },
        { 0x00, 0x00, 0x00, 0x00 },
    };

    static uint8_t patternMask[256][16];

    bool initializePatternTable()
    {
        for (uint32_t pattern = 0; pattern < 256; ++pattern)
        {
            for (uint32_t bit = 0; bit < 8; ++bit)
            {
                uint8_t mask = ((pattern << bit) & 0x80) ? 0xff : 0x00;
                patternMask[pattern][bit] = mask;
                patternMask[pattern][15 - bit] = mask;
            }
        }
        return true;
    }

    bool patternTableInitialized = initializePatternTable();

    float saturate(float value)
    {
        if (value < 0.0f)
            value = 0.0f;
        if (value > 1.0f)
            value = 1.0f;
        return value;
    }

    float smoothstep(float value)
    {
        value = saturate(value);
        auto value2 = value * value;
        auto value3 = value2 * value;
        auto result = 3 * value2 - 2 * value3;
        return result;
    }

    void adjust_saturation(float& r, float& g, float& b, float saturation)
    {
        float intensity = 0.2126f * r + 0.7152f * g + 0.0722f * b;
        float w0 = 1.0f - saturation;
        float w1 = saturation;
        r = w0 * intensity + w1 * r;
        g = w0 * intensity + w1 * g;
        b = w0 * intensity + w1 * b;
    }

    void adjust_gamma(float& value, float exponent)
    {
        value = powf(value, exponent);
    }

    uint32_t to_rgba(float r, float g, float b, float a)
    {
        emu::word32_t result;
        result.w8[0].u = static_cast<uint8_t>(saturate(r) * 255);
        result.w8[1].u = static_cast<uint8_t>(saturate(g) * 255);
        result.w8[2].u = static_cast<uint8_t>(saturate(b) * 255);
        result.w8[3].u = static_cast<uint8_t>(saturate(a) * 255);
        return result.u;
    }

    static uint32_t colorTable[32768];

    bool initializeColorTable()
    {
        for (uint32_t index = 0; index < EMU_ARRAY_SIZE(colorTable); ++index)
        {
            uint32_t r = (index >> 0) & 0x1f;
            uint32_t g = (index >> 5) & 0x1f;
            uint32_t b = (index >> 10) & 0x1f;

            float sr = smoothstep(r / 31.0f);
            float sg = smoothstep(g / 31.0f);
            float sb = smoothstep(b / 31.0f);
            float fr = sr;
            float fg = 0.05f * sr + 0.65f * sg + 0.30f * sb;
            float fb = sb;

            static const float saturation = 0.85f;
            adjust_saturation(fr, fg, fb, saturation);

            static const float gamma = 1.0f / 2.0f;
            adjust_gamma(fr, gamma);
            adjust_gamma(fg, gamma);
            adjust_gamma(fb, gamma);
            colorTable[index] = to_rgba(fr, fg, fb, 1.0f);
        }
        return true;
    }

    bool colorTableInitialized = initializeColorTable();
}

namespace gb
{
    Display::ClockListener::ClockListener()
    {
        initialize();
    }

    Display::ClockListener::~ClockListener()
    {
        destroy();
    }

    void Display::ClockListener::initialize()
    {
        mClock = nullptr;
        mDisplay = nullptr;
    }

    bool Display::ClockListener::create(emu::Clock& clock, Display& display)
    {
        mClock = &clock;
        mDisplay = &display;
        mClock->addListener(*this);
        return true;
    }

    void Display::ClockListener::destroy()
    {
        if (mClock)
            mClock->removeListener(*this);
        initialize();
    }

    void Display::ClockListener::execute()
    {
        mDisplay->execute();
    }

    void Display::ClockListener::resetClock()
    {
        mDisplay->resetClock();
    }

    void Display::ClockListener::advanceClock(int32_t tick)
    {
        mDisplay->advanceClock(tick);
    }

    void Display::ClockListener::setDesiredTicks(int32_t tick)
    {
        mDisplay->setDesiredTicks(tick);
    }

    ////////////////////////////////////////////////////////////////////////////

    Display::Config::Config()
        : model(Model::GBC)
        , fastDMA(true)
        , fastHDMA(true)
        , fastLineRendering(true)
    {
    }

    ////////////////////////////////////////////////////////////////////////////

    Display::Display()
    {
        initialize();
    }

    Display::~Display()
    {
        destroy();
    }

    void Display::initialize()
    {
        mClock                  = nullptr;
        mMemory                 = nullptr;
        mInterrupts             = nullptr;
        mSurface                = nullptr;
        mPitch                  = 0;
        mRenderedLine           = 0;
        mRenderedLineFirstTick  = 0;
        mRenderedTick           = 0;
        mTicksPerLine           = 0;
        mVBlankStartTick        = 0;
        mMode0StartTick         = 0;
        mMode3StartTick         = 0;
        mUpdateRasterPosFast    = 0;
        mRasterTick             = 0;
        mRasterLine             = 0;
        mBankVRAM               = 0;
        mRegLCDC                = 0x00;
        mRegSTAT                = 0x00;
        mRegSCY                 = 0x00;
        mRegSCX                 = 0x00;
        mRegLY                  = 0x00;
        mRegLYC                 = 0x00;
        mRegDMA                 = 0x00;
        mRegBGP                 = 0x00;
        mRegOBP0                = 0x00;
        mRegOBP1                = 0x00;
        mRegWY                  = 0x00;
        mRegWX                  = 0x00;
        mRegBGPI                = 0x00;
        memset(mRegBGPD, 0, sizeof(mRegBGPD));
        mRegOBPI                = 0x00;
        memset(mRegOBPD, 0, sizeof(mRegOBPD));
        mRegHDMA[0]             = 0x00;
        mRegHDMA[1]             = 0x00;
        mRegHDMA[2]             = 0x00;
        mRegHDMA[3]             = 0x00;
        mRegHDMA[4]             = 0x00;
        mActiveSprites          = 0;
        mSortedSprites = false;
        mCachedPalette = false;
        resetClock();
        mLineIntTick = 0;
        mLineIntLastLY = 0;
        mLineIntLastLYC = 0;
        mLineIntLastEnabled = false;

        mIntUpdate = INT_NONE;
        mIntEnabled = INT_NONE;
        mIntSync = INT_NONE;
        mIntPredictionMode0 = 0;
    }

    bool Display::create(Config& config, emu::Clock& clock, uint32_t master_clock_divider, emu::MemoryBus& memory, Interrupts& interrupts, emu::RegisterBank& registers)
    {
        mTicksPerLine = TICKS_PER_LINE * master_clock_divider;
        mVBlankStartTick = MODE1_START_TICK * master_clock_divider;
        mMode0StartTick = MODE0_START_TICK * master_clock_divider;
        mMode3StartTick = MODE3_START_TICK * master_clock_divider;
        mUpdateRasterPosFast = UPDATE_RASTER_POS_FAST * master_clock_divider;

        mClock = &clock;
        mMemory = &memory;
        mInterrupts = &interrupts;
        mConfig = config;

        EMU_VERIFY(mClockListener.create(clock, *this));

        bool isGBC = mConfig.model >= gb::Model::GBC;
        mVRAM.resize(isGBC ? VRAM_SIZE_GBC : VRAM_SIZE_GB, 0);
        EMU_VERIFY(updateMemoryMap());
        EMU_VERIFY(memory.addMemoryRange(VRAM_BANK_START, VRAM_BANK_END, mMemoryVRAM));

        mOAM.resize(OAM_SIZE, 0);
        mOAMOrder.resize(SPRITE_CAPACITY);
        mMemoryOAM.read.setReadMemory(mOAM.data());
        mMemoryOAM.write.setWriteMethod(&onWriteOAM, this);
        EMU_VERIFY(memory.addMemoryRange(0xfe00, 0xfe9f, mMemoryOAM));

        mMemoryNotUsable.write.setWriteMethod(&onWriteNotUsable, this, 0);
        EMU_VERIFY(memory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_WRITE, 0xfea0, 0xfeff, mMemoryNotUsable.write));

        mPalette.resize(PALETTE_SIZE, *reinterpret_cast<const uint32_t*>(&kMonoPalette[0]));

        EMU_VERIFY(mRegisterAccessors.read.LCDC.create(registers, 0x40, *this, &Display::readLCDC));
        EMU_VERIFY(mRegisterAccessors.read.STAT.create(registers, 0x41, *this, &Display::readSTAT));
        EMU_VERIFY(mRegisterAccessors.read.SCY.create(registers, 0x42, *this, &Display::readSCY));
        EMU_VERIFY(mRegisterAccessors.read.SCX.create(registers, 0x43, *this, &Display::readSCX));
        EMU_VERIFY(mRegisterAccessors.read.LY.create(registers, 0x44, *this, &Display::readLY));
        EMU_VERIFY(mRegisterAccessors.read.LYC.create(registers, 0x45, *this, &Display::readLYC));
        EMU_VERIFY(mRegisterAccessors.read.BGP.create(registers, 0x47, *this, &Display::readBGP));
        EMU_VERIFY(mRegisterAccessors.read.OBP0.create(registers, 0x48, *this, &Display::readOBP0));
        EMU_VERIFY(mRegisterAccessors.read.OBP1.create(registers, 0x49, *this, &Display::readOBP1));
        EMU_VERIFY(mRegisterAccessors.read.WY.create(registers, 0x4a, *this, &Display::readWY));
        EMU_VERIFY(mRegisterAccessors.read.WX.create(registers, 0x4b, *this, &Display::readWX));

        EMU_VERIFY(mRegisterAccessors.write.LCDC.create(registers, 0x40, *this, &Display::writeLCDC));
        EMU_VERIFY(mRegisterAccessors.write.STAT.create(registers, 0x41, *this, &Display::writeSTAT));
        EMU_VERIFY(mRegisterAccessors.write.SCY.create(registers, 0x42, *this, &Display::writeSCY));
        EMU_VERIFY(mRegisterAccessors.write.SCX.create(registers, 0x43, *this, &Display::writeSCX));
        EMU_VERIFY(mRegisterAccessors.write.LY.create(registers, 0x44, *this, &Display::writeLY));
        EMU_VERIFY(mRegisterAccessors.write.LYC.create(registers, 0x45, *this, &Display::writeLYC));
        EMU_VERIFY(mRegisterAccessors.write.DMA.create(registers, 0x46, *this, &Display::writeDMA));
        EMU_VERIFY(mRegisterAccessors.write.BGP.create(registers, 0x47, *this, &Display::writeBGP));
        EMU_VERIFY(mRegisterAccessors.write.OBP0.create(registers, 0x48, *this, &Display::writeOBP0));
        EMU_VERIFY(mRegisterAccessors.write.OBP1.create(registers, 0x49, *this, &Display::writeOBP1));
        EMU_VERIFY(mRegisterAccessors.write.WY.create(registers, 0x4a, *this, &Display::writeWY));
        EMU_VERIFY(mRegisterAccessors.write.WX.create(registers, 0x4b, *this, &Display::writeWX));

        if (mConfig.model >= gb::Model::GBC)
        {
            EMU_VERIFY(mRegisterAccessors.read.VBK.create(registers, 0x4f, *this, &Display::readVBK));
            EMU_VERIFY(mRegisterAccessors.read.BGPI.create(registers, 0x68, *this, &Display::readBGPI));
            EMU_VERIFY(mRegisterAccessors.read.BGPD.create(registers, 0x69, *this, &Display::readBGPD));
            EMU_VERIFY(mRegisterAccessors.read.OBPI.create(registers, 0x6a, *this, &Display::readOBPI));
            EMU_VERIFY(mRegisterAccessors.read.OBPD.create(registers, 0x6b, *this, &Display::readOBPD));

            EMU_VERIFY(mRegisterAccessors.write.VBK.create(registers, 0x4f, *this, &Display::writeVBK));
            EMU_VERIFY(mRegisterAccessors.write.BGPI.create(registers, 0x68, *this, &Display::writeBGPI));
            EMU_VERIFY(mRegisterAccessors.write.BGPD.create(registers, 0x69, *this, &Display::writeBGPD));
            EMU_VERIFY(mRegisterAccessors.write.OBPI.create(registers, 0x6a, *this, &Display::writeOBPI));
            EMU_VERIFY(mRegisterAccessors.write.OBPD.create(registers, 0x6b, *this, &Display::writeOBPD));

            for (uint32_t index = 0; index < 5; ++index)
            {
                EMU_VERIFY(mRegisterAccessors.read.HDMA[index].create(registers, 0x51 + index, *this, &Display::readHDMA));
                EMU_VERIFY(mRegisterAccessors.write.HDMA[index].create(registers, 0x51 + index, *this, &Display::writeHDMA));
            }
        }

        return true;
    }

    void Display::destroy()
    {
        mOAMOrder.clear();
        mOAM.clear();
        mVRAM.clear();
        mClockListener.destroy();
        mRegisterAccessors = RegisterAccessors();
        initialize();
    }

    void Display::execute()
    {
        if (mSimulatedTick < mDesiredTick)
        {
            updateRasterPos(mDesiredTick);
            mSimulatedTick = mDesiredTick;
        }
    }

    void Display::resetClock()
    {
        mSimulatedTick = 0;
        mRenderedLine = 0;
        mRenderedLineFirstTick = 0;
        mRenderedTick = 0;
        mDesiredTick = 0;
        mLineFirstTick = 0;
        mLineTick = 0;
    }

    void Display::advanceClock(int32_t tick)
    {
        render(tick);
        mSimulatedTick -= tick;
        mDesiredTick -= tick;
        mLineFirstTick -= tick;
        mLineTick -= tick;
        mRasterTick -= tick;
        mIntPredictionMode0 -= tick;
    }

    void Display::setDesiredTicks(int32_t tick)
    {
        mDesiredTick = tick;
    }

    bool Display::updateMemoryMap()
    {
        mMemoryVRAM.setReadWriteMemory(mVRAM.data() + mBankVRAM * VRAM_BANK_SIZE);
        return true;
    }

    void Display::beginFrame()
    {
        updateMemoryMap();
        mClock->addEvent(onVBlankStart, this, mVBlankStartTick);
        mRasterLine -= DISPLAY_LINE_COUNT;
        mRenderedLine = 0;
        mRenderedLineFirstTick = 0;
        mRenderedTick = 0;
        mLineIntLastEnabled = false;
        updateLineInterrupt(mSimulatedTick);

        mIntSync = INT_ALL;
        updateRasterPos(mSimulatedTick);
    }

    uint8_t Display::readLCDC(int32_t tick, uint16_t addr)
    {
        return mRegLCDC;
    }

    void Display::writeLCDC(int32_t tick, uint16_t addr, uint8_t value)
    {
        uint8_t modified = mRegLCDC ^ value;
        if (modified)
        {
            render(tick);
            bool spriteSizeChanged = ((mRegLCDC ^ value) & LCDC_SPRITES_SIZE) != 0;
            if (spriteSizeChanged)
                mSortedSprites = false;
            mRegLCDC = value;

            if (modified & LCDC_LCD_ENABLE)
            {
                if ((value & LCDC_LCD_ENABLE) == 0)
                {
                    if (tick < mVBlankStartTick)
                        mInterrupts->setInterrupt(tick, gb::Interrupts::Signal::VBlank);
                }

                updateLineInterrupt(tick);
                mIntUpdate = INT_ALL;
                updateRasterPos(tick);
            }
        }
    }

    uint8_t Display::readSTAT(int32_t tick, uint16_t addr)
    {
        uint8_t mode = getMode(tick);
        uint8_t coincidence = mRegLY == mRegLYC ? STAT_LYC_LY_STATUS : 0;
        uint8_t value = mRegSTAT | mode | coincidence;
        return value;
    }

    void Display::writeSTAT(int32_t tick, uint16_t addr, uint8_t value)
    {
        value &= STAT_WRITE_MASK;
        uint8_t modified = mRegSTAT ^ value;
        if (modified)
        {
            updateRasterPos(tick);
            mRegSTAT = value;
            if (modified & STAT_OAM_INT)
            {
                EMU_NOT_IMPLEMENTED();
            }
            if (modified & STAT_VBLANK_INT)
            {
                EMU_NOT_IMPLEMENTED();
            }
            if (modified & STAT_HBLANK_INT)
                mIntUpdate |= INT_MODE_0;
            if (modified & STAT_LYC_LY_INT)
                updateLineInterrupt(tick);
            updateRasterPos(tick);
        }
    }

    uint8_t Display::readSCY(int32_t tick, uint16_t addr)
    {
        return mRegSCY;
    }

    void Display::writeSCY(int32_t tick, uint16_t addr, uint8_t value)
    {
        if (mRegSCY != value)
        {
            render(tick);
            mRegSCY = value;
        }
    }

    uint8_t Display::readSCX(int32_t tick, uint16_t addr)
    {
        return mRegSCX;
    }

    void Display::writeSCX(int32_t tick, uint16_t addr, uint8_t value)
    {
        if (mRegSCX != value)
        {
            render(tick);
            mRegSCX = value;
        }
    }

    uint8_t Display::readLY(int32_t tick, uint16_t addr)
    {
        updateRasterPos(tick);
        auto value = mRegLY;
        if (value >= 153)
            value = 0;
        return value;
    }

    void Display::writeLY(int32_t tick, uint16_t addr, uint8_t value)
    {
        EMU_NOT_IMPLEMENTED();
        mRegLY = value;
    }

    uint8_t Display::readLYC(int32_t tick, uint16_t addr)
    {
        return mRegLYC;
    }

    void Display::writeLYC(int32_t tick, uint16_t addr, uint8_t value)
    {
        if (mRegLYC != value)
        {
            mRegLYC = value;
            updateLineInterrupt(tick);
        }
    }

    void Display::writeDMA(int32_t tick, uint16_t addr, uint8_t value)
    {
        if (!mConfig.fastDMA)
        {
            EMU_NOT_IMPLEMENTED();
        }

        // Fast DMA implementation: transfer all data in the same tick
        render(tick);
        uint16_t read_addr = value << 8;

        for (uint8_t index = 0; index < DMA_SIZE; ++index)
        {
            uint8_t dma_value = mMemory->read8(mMemoryDMAReadAccessor, tick, read_addr + index);
            mOAM[index] = dma_value;
        }
        mSortedSprites = false;
        mRegDMA = value;
    }

    uint8_t Display::readBGP(int32_t tick, uint16_t addr)
    {
        return mRegBGP;
    }

    void Display::writeBGP(int32_t tick, uint16_t addr, uint8_t value)
    {
        if (mRegBGP != value)
        {
            mCachedPalette = false;
            render(tick);
            mRegBGP = value;
        }
    }

    uint8_t Display::readOBP0(int32_t tick, uint16_t addr)
    {
        return mRegOBP0;
    }

    void Display::writeOBP0(int32_t tick, uint16_t addr, uint8_t value)
    {
        if (mRegOBP0 != value)
        {
            mCachedPalette = false;
            render(tick);
            mRegOBP0 = value;
        }
    }

    uint8_t Display::readOBP1(int32_t tick, uint16_t addr)
    {
        return mRegOBP1;
    }

    void Display::writeOBP1(int32_t tick, uint16_t addr, uint8_t value)
    {
        if (mRegOBP1 != value)
        {
            mCachedPalette = false;
            render(tick);
            mRegOBP1 = value;
        }
    }

    uint8_t Display::readWY(int32_t tick, uint16_t addr)
    {
        return mRegWY;
    }

    void Display::writeWY(int32_t tick, uint16_t addr, uint8_t value)
    {
        if (mRegWY != value)
        {
            render(tick);
            mRegWY = value;
        }
    }

    uint8_t Display::readWX(int32_t tick, uint16_t addr)
    {
        return mRegWX;
    }

    void Display::writeWX(int32_t tick, uint16_t addr, uint8_t value)
    {
        if (mRegWX != value)
        {
            render(tick);
            mRegWX = value;
        }
    }

    uint8_t Display::readVBK(int32_t tick, uint16_t addr)
    {
        return mBankVRAM;
    }

    void Display::writeVBK(int32_t tick, uint16_t addr, uint8_t value)
    {
        mBankVRAM = value & VBK_MASK;
        updateMemoryMap();
    }

    uint8_t Display::readBGPI(int32_t tick, uint16_t addr)
    {
        return mRegBGPI;
    }

    void Display::writeBGPI(int32_t tick, uint16_t addr, uint8_t value)
    {
        mRegBGPI = value;
    }

    uint8_t Display::readBGPD(int32_t tick, uint16_t addr)
    {
        uint8_t index = mRegBGPI & BGPI_INDEX_MASK;
        return mRegBGPD[index];
    }

    void Display::writeBGPD(int32_t tick, uint16_t addr, uint8_t value)
    {
        render(tick);
        auto index = mRegBGPI & BGPI_INDEX_MASK;
        mRegBGPD[index] = value;
        mCachedPalette = false;
        if (mRegBGPI & BGPI_AUTO_INCR)
        {
            ++index;
            mRegBGPI = (mRegBGPI & ~BGPI_INDEX_MASK) | (index & BGPI_INDEX_MASK);
        }
    }

    uint8_t Display::readOBPI(int32_t tick, uint16_t addr)
    {
        return mRegOBPI;
    }

    void Display::writeOBPI(int32_t tick, uint16_t addr, uint8_t value)
    {
        mRegOBPI = value;
    }

    uint8_t Display::readOBPD(int32_t tick, uint16_t addr)
    {
        uint8_t index = mRegOBPI & OBPI_INDEX_MASK;
        return mRegOBPD[index];
    }

    void Display::writeOBPD(int32_t tick, uint16_t addr, uint8_t value)
    {
        render(tick);
        auto index = mRegOBPI & OBPI_INDEX_MASK;
        mRegOBPD[index] = value;
        mCachedPalette = false;
        if (mRegOBPI & OBPI_AUTO_INCR)
        {
            ++index;
            mRegOBPI = (mRegOBPI & ~OBPI_INDEX_MASK) | (index & OBPI_INDEX_MASK);
        }
    }

    uint8_t Display::readHDMA(int32_t tick, uint16_t addr)
    {
        uint32_t index = addr - 0x51;
        return mRegHDMA[index];
    }

    void Display::writeHDMA(int32_t tick, uint16_t addr, uint8_t value)
    {
        uint32_t index = addr - 0x51;
        mRegHDMA[index] = value;
        if (index == 4)
        {
            if (!mConfig.fastHDMA)
            {
                EMU_NOT_IMPLEMENTED();
            }

            if (value & HDMA5_HBLANK)
            {
                EMU_NOT_IMPLEMENTED();
            }

            render(tick);

            uint32_t size = ((value & HDMA5_SIZE_MASK) + 1) << 4;
            uint16_t src = (mRegHDMA[0] << 8) | mRegHDMA[1];
            uint16_t dst = (mRegHDMA[2] << 8) | mRegHDMA[3];
            dst = (dst & HDMA_DST_MASK) + HDMA_DST_BASE;
            for (uint32_t pos = 0; pos < size; ++pos)
            {
                uint8_t value = mMemory->read8(mMemoryHDMAReadAccessor, tick, src + pos);
                auto dstFinal = ((dst + pos) & HDMA_DST_FINAL_MASK) + HDMA_DST_BASE;
                mMemory->write8(mMemoryHDMAWriteAccessor, tick, dstFinal, value);
            }
            mRegHDMA[4] = HDMA5_DONE;
        }
    }

    void Display::writeOAM(int32_t tick, uint16_t addr, uint8_t value)
    {
        if (mOAM[addr] != value)
        {
            render(tick);
            mOAM[addr] = value;
            mSortedSprites = false;
        }
    }

    void Display::reset()
    {
        mMemoryDMAReadAccessor.reset();
        mMemoryHDMAReadAccessor.reset();
        mMemoryHDMAWriteAccessor.reset();
        mRegLCDC = 0x91;
        mRegSTAT = 0x00;
        mRegSCY  = 0x00;
        mRegSCX  = 0x00;
        mRegLY   = 0x00;
        mRegLYC  = 0x00;
        mRegDMA  = 0x00;
        mRegBGP  = 0xfc;
        mRegOBP0 = 0xff;
        mRegOBP1 = 0xff;
        mRegWY   = 0x00;
        mRegWX   = 0x00;
        mRegHDMA[0] = 0x00;
        mRegHDMA[1] = 0x00;
        mRegHDMA[2] = 0x00;
        mRegHDMA[3] = 0x00;
        mRegHDMA[4] = 0xff;
        mSortedSprites = false;
        mCachedPalette = false;
        mLineIntTick = 0;
        mLineIntLastLY = 0;
        mLineIntLastLYC = 0;
        mLineIntLastEnabled = false;

        mIntUpdate = INT_NONE;
        mIntEnabled = INT_NONE;
        mIntSync = INT_NONE;
        mIntPredictionMode0 = 0;
    }

    void Display::serialize(emu::ISerializer& serializer)
    {
        serializer.serialize(mVRAM);
        serializer.serialize(mOAM);
        serializer.serialize(mSimulatedTick);
        serializer.serialize(mRenderedLine);
        serializer.serialize(mRenderedTick);
        serializer.serialize(mDesiredTick);
        serializer.serialize(mLineFirstTick);
        serializer.serialize(mLineTick);
        serializer.serialize(mLineIntTick);
        serializer.serialize(mRasterTick);
        serializer.serialize(mRasterLine);
        serializer.serialize(mBankVRAM);
        serializer.serialize(mRegLCDC);
        serializer.serialize(mRegSTAT);
        serializer.serialize(mRegSCY);
        serializer.serialize(mRegSCX);
        serializer.serialize(mRegLY);
        serializer.serialize(mRegLYC);
        serializer.serialize(mRegDMA);
        serializer.serialize(mRegBGP);
        serializer.serialize(mRegOBP0);
        serializer.serialize(mRegOBP1);
        serializer.serialize(mRegWY);
        serializer.serialize(mRegWX);
        serializer.serialize(mRegBGPI);
        serializer.serialize(mRegBGPD, EMU_ARRAY_SIZE(mRegBGPD));
        serializer.serialize(mRegOBPI);
        serializer.serialize(mRegOBPD, EMU_ARRAY_SIZE(mRegOBPD));
        serializer.serialize(mRegHDMA, EMU_ARRAY_SIZE(mRegHDMA));
        serializer.serialize(mLineIntLastLY);
        serializer.serialize(mLineIntLastLYC);
        serializer.serialize(mLineIntLastEnabled);
        serializer.serialize(mIntUpdate);
        serializer.serialize(mIntEnabled);
        serializer.serialize(mIntSync);
        serializer.serialize(mIntPredictionMode0);
        if (serializer.isReading())
        {
            mSortedSprites = false;
            mCachedPalette = false;
            mIntSync = INT_ALL;
        }
    }

    void Display::setRenderSurface(void* surface, size_t pitch)
    {
        mSurface = static_cast<uint8_t*>(surface);
        mPitch = pitch;
    }

    void Display::onVBlankStart(int32_t tick)
    {
        render(tick);
        if ((mRegLCDC & LCDC_LCD_ENABLE) != 0)
            mInterrupts->setInterrupt(tick, gb::Interrupts::Signal::VBlank);
    }

    void Display::updateRasterPos(int32_t tick)
    {
        if (mRasterTick < tick)
        {
            mRasterTick = tick;
            if (tick - mLineFirstTick <= mUpdateRasterPosFast)
            {
                while (tick - mLineFirstTick >= mTicksPerLine)
                {
                    ++mRasterLine;
                    ++mRegLY;
                    mLineFirstTick += mTicksPerLine;
                }
                mLineTick = tick - mLineFirstTick;
            }
            else
            {
                uint8_t rasterLine = tick / mTicksPerLine;
                uint8_t deltaLine = rasterLine - mRasterLine;
                mRasterLine = rasterLine;
                mRegLY = rasterLine;
                mLineTick = tick % mTicksPerLine;
                mLineFirstTick = tick - mLineTick;
            }
            if (mRegLY >= DISPLAY_LINE_COUNT)
                mRegLY -= DISPLAY_LINE_COUNT;
        }
        updateLineInterrupt(mRasterTick);
        updateInterrupts(mRasterTick);
    }

    uint8_t Display::getMode(int32_t tick)
    {
        updateRasterPos(tick);
        if (tick >= mVBlankStartTick)
            return 1;
        if (mLineTick < mMode3StartTick)
            return 2;
        if (mLineTick < mMode0StartTick)
            return 3;
        return 0;
    }

    void Display::updateInterrupts(int32_t tick)
    {
        // Process any signaled interrupt
        if (mIntEnabled && !mIntUpdate)
        {
            if ((mIntEnabled & INT_MODE_0) && (mIntPredictionMode0 <= tick))
            {
                mInterrupts->setInterrupt(tick, gb::Interrupts::Signal::LcdStat);
                mIntUpdate |= INT_MODE_0;
            }
        }

        // Update dirty predictions
        auto intEnabledOld = mIntEnabled;
        if (mIntUpdate)
        {
            if (mIntUpdate & INT_MODE_0)
            {
                bool enabled = (mRegLCDC & LCDC_LCD_ENABLE) && (mRegSTAT & STAT_HBLANK_INT);
                if (enabled)
                    mIntEnabled |= INT_MODE_0;
                else
                    mIntEnabled &= ~INT_MODE_0;
                if (enabled)
                {
                    mIntPredictionMode0 = mLineFirstTick + mMode0StartTick;
                    uint8_t line = mRasterLine;
                    while (true)
                    {
                        if (line >= DISPLAY_LINE_COUNT)
                            line -= DISPLAY_LINE_COUNT;
                        if ((line < DISPLAY_SIZE_Y) && (tick < mIntPredictionMode0))
                            break;
                        mIntPredictionMode0 += mTicksPerLine;
                        ++line;
                    }
                    mIntSync |= INT_MODE_0;
                }
            }
            mIntUpdate = 0;
        }

        if (mIntSync)
        {
            // Find next prediction to be true
            mIntSync &= mIntEnabled;
            int32_t prediction = INT32_MAX;
            if (mIntSync & INT_MODE_0)
                prediction = std::min(prediction, mIntPredictionMode0);
            mIntSync = 0;

            // Synchronize
            if (prediction < INT32_MAX)
                mClock->addSync(prediction);
        }
    }

    void Display::updateLineInterrupt(int32_t tick)
    {
        bool enabled = (mRegLCDC & LCDC_LCD_ENABLE) && (mRegSTAT & STAT_LYC_LY_INT) && (mRegLYC < DISPLAY_LINE_COUNT);
        bool modifiedEnabled = enabled && !mLineIntLastEnabled;

        bool modifiedLYC = mLineIntLastLYC != mRegLYC;
        if (modifiedLYC)
            mLineIntTick = mRegLYC * mTicksPerLine;

        bool lineIntTriggered = (mLineIntLastLY < mRegLYC) && (mRegLY >= mRegLYC);

        mLineIntLastLY = mRegLY;
        mLineIntLastLYC = mRegLYC;
        mLineIntLastEnabled = enabled;

        if (enabled)
        {
            if (modifiedEnabled | modifiedLYC)
            {
                if (mRegLY < mRegLYC)
                    mClock->addSync(mLineIntTick);
            }

            if (lineIntTriggered)
                mInterrupts->setInterrupt(tick, gb::Interrupts::Signal::LcdStat);
        }
    }

    void Display::updatePalette()
    {
        if (mConfig.model >= gb::Model::GBC)
        {
            for (uint32_t index = 0; index < PALETTE_SIZE; ++index)
            {
                emu::word16_t color;
                if (index < PALETTE_BASE_OBP)
                {
                    uint32_t base = index << 1;
                    color.w.l.u = mRegBGPD[base + 0];
                    color.w.h.u = mRegBGPD[base + 1];
                }
                else
                {
                    uint32_t base = (index - PALETTE_BASE_OBP) << 1;
                    color.w.l.u = mRegOBPD[base + 0];
                    color.w.h.u = mRegOBPD[base + 1];
                }
                mPalette[index] = colorTable[color.u & 0x7fff];
            }
        }
        else
        {
            static const uint32_t kIndexTable[] =
            {
                PALETTE_MONO_BASE_BGP,
                PALETTE_MONO_BASE_OBP0,
                PALETTE_MONO_BASE_OBP1,
            };
            uint8_t values[] =
            {
                mRegBGP,
                mRegOBP0,
                mRegOBP1,
            };
            for (uint32_t index = 0; index < EMU_ARRAY_SIZE(values); ++index)
            {
                auto value = values[index];
                auto base = kIndexTable[index];
                for (uint32_t subIndex = 0; subIndex < 4; ++subIndex)
                {
                    auto shade = value & 0x3;
                    value >>= 2;
                    mPalette[base + subIndex] = *reinterpret_cast<const uint32_t*>(kMonoPalette[shade]);
                }
            }
        }
        mCachedPalette = true;
    }

    void Display::sortSprites()
    {
        mActiveSprites = 0;
        uint8_t spriteSizeY = (mRegLCDC & LCDC_SPRITES_SIZE) ? 16 : 8;
        if (mConfig.model >= gb::Model::GBC)
        {
            for (uint32_t index = 0; index < SPRITE_CAPACITY; ++index)
            {
                // Check if sprite is active
                uint8_t spriteY = mOAM[index * 4 + 0];
                if ((spriteY >= SPRITE_VISIBLE_Y_END) || (spriteY + spriteSizeY <= SPRITE_VISIBLE_Y_BEGIN))
                    continue;

                mOAMOrder[mActiveSprites++] = index;
            }
        }
        else
        {
            uint16_t keys[SPRITE_CAPACITY];
            for (uint32_t index = 0; index < SPRITE_CAPACITY; ++index)
            {
                // Check if sprite is active
                uint8_t spriteY = mOAM[index * 4 + 0];
                if ((spriteY >= SPRITE_VISIBLE_Y_END) || (spriteY + spriteSizeY <= SPRITE_VISIBLE_Y_BEGIN))
                    continue;
                uint8_t spriteX = mOAM[index * 4 + 1];
                keys[mActiveSprites++] = (spriteX << 8) | (index & 0xff);
            }

            if (mActiveSprites)
            {
                // Sort sprites
                std::sort(keys, keys + mActiveSprites, [](int a, int b){ return a < b; });
                for (uint32_t index = 0; index < mActiveSprites; ++index)
                    mOAMOrder[index] = keys[index] & 0xff;
            }
        }
        mSortedSprites = true;
    }

    void Display::fetchTileRow(uint8_t* dest, const uint8_t* map, uint32_t tileX, uint32_t tileY, uint8_t tileOffset, uint32_t count)
    {
        uint32_t base = (tileY & 0x1f) << 5;
        for (uint32_t index = 0; index < count; ++index)
        {
            uint32_t offset = (tileX + index) & 0x1f;
            uint8_t value = map[base + offset];
            dest[index] = (value + tileOffset) & 0xff;
        }
    }

    void Display::fetchAttrRow(uint8_t* dest, const uint8_t* map, uint32_t tileX, uint32_t tileY, uint8_t tileOffset, uint32_t count)
    {
        fetchTileRow(dest, map + VRAM_BANK_SIZE, tileX, tileY, tileOffset, count);
    }

    void Display::drawTiles(uint8_t* dest, const uint8_t* tiles, const uint8_t* attributes, const uint8_t* patterns, uint8_t tileOffsetY, uint16_t count)
    {
        for (uint32_t index = 0; index < count; ++index)
        {
            uint8_t attr = attributes[index];

            uint8_t palette8 = ((attr & 0x07) << 2) + PALETTE_BASE_BGP;
            emu::word32_t palette32;
            palette32.w8[0].u = palette8;
            palette32.w8[1].u = palette8;
            palette32.w8[2].u = palette8;
            palette32.w8[3].u = palette8;
            uint32_t palette = palette32.u;

            uint32_t tile = tiles[index];
            uint32_t patternBank = (attr & 0x08) << 10; // Attribute bit is 0x08 and bank size is 0x2000
            uint8_t flipY = int8_t((attr & 0x40) << 1) >> 7; // Expand flip Y bit in the other 7 bits;
            uint8_t offsetY = (tileOffsetY ^ (flipY & 7)) << 1; // Flip Y position if the lower 3 bits of flipY are set
            uint32_t patternBase = (tile << 4) + patternBank + offsetY;
            uint32_t patternBias = (attr & 0x20) >> 2; // Horizontal flip is bit 5 in attrib and is stored 8 bytes later in pattern table

            auto pattern0 = reinterpret_cast<const uint32_t*>(&patternMask[patterns[patternBase + 0]][patternBias]);
            auto pattern1 = reinterpret_cast<const uint32_t*>(&patternMask[patterns[patternBase + 1]][patternBias]);

            auto value0 = (pattern0[0] & 0x01010101) + (pattern1[0] & 0x02020202) + palette;
            auto value1 = (pattern0[1] & 0x01010101) + (pattern1[1] & 0x02020202) + palette;
            reinterpret_cast<uint32_t*>(dest)[0] = value0;
            reinterpret_cast<uint32_t*>(dest)[1] = value1;
            dest += 8;
        }
    }

    void Display::drawSprites(uint8_t* dest, uint8_t line, uint8_t spriteSizeY, uint8_t paletteMask, uint8_t paletteShift, uint8_t bankMask)
    {
        // Find active sprites visible on this line (up to the line limit)
        uint8_t sprites[SPRITE_LINE_LIMIT];
        uint8_t active = 0;
        line += SPRITE_VISIBLE_Y_BEGIN;
        for (uint8_t key = 0; key < mActiveSprites; ++key)
        {
            uint8_t index = mOAMOrder[key];
            uint8_t spriteY = mOAM[index * 4 + 0];
            if ((spriteY > line) || (spriteY + spriteSizeY <= line))
                continue;
            sprites[active] = index;
            if (++active >= SPRITE_LINE_LIMIT)
                break;
        }

        // Render active sprites
        static const uint8_t spriteSizeX = 8;
        uint8_t tileMask = (spriteSizeY >> 4) & 1;
        for (uint8_t key = 0; key < active; ++key)
        {
            // Skip sprite if it is not visible horizontally
            uint8_t index = sprites[key];
            uint8_t spriteX = mOAM[index * 4 + 1];
            if ((spriteX >= SPRITE_VISIBLE_X_END) || (spriteX + spriteSizeX <= SPRITE_VISIBLE_X_BEGIN))
                continue;

            // Fetch attributes
            uint8_t spriteY = mOAM[index * 4 + 0];
            uint8_t flags = mOAM[index * 4 + 3];

            // Select line to render and adjust tile number
            uint8_t offsetY = line - spriteY;
            if ((flags & SPRITE_FLAG_FLIP_Y) != 0)
                offsetY = spriteSizeY - 1 - offsetY;

            // Get the tile number
            uint8_t tile = mOAM[index * 4 + 2];
            tile = (tile & ~tileMask) | ((offsetY >> 3) & tileMask);

            // Get the pattern
            uint8_t flipX = (flags & SPRITE_FLAG_FLIP_X) ? 7 : 0;
            uint32_t tileBank = (flags & bankMask) << 10;
            uint32_t tileAddr = (tile << 4) + ((offsetY & 7) << 1) + tileBank;
            uint8_t pattern0 = mVRAM[tileAddr + 0];
            uint8_t pattern1 = mVRAM[tileAddr + 1];
            const uint8_t* patternMask0 = patternMask[pattern0];
            const uint8_t* patternMask1 = patternMask[pattern1];

            // Get the palette
            uint8_t palette = (((flags & paletteMask) >> paletteShift) << 2) + PALETTE_BASE_OBP;

            // Render pixels
            bool backgroundSprite = (flags & SPRITE_FLAG_BACKGROUND) != 0;
            for (uint8_t bit = 0; bit < spriteSizeX; ++bit)
            {
                uint8_t offsetX = spriteX + (bit ^ flipX);
                uint8_t src = dest[offsetX];

                uint8_t value = (patternMask0[bit] & 0x01) | (patternMask1[bit] & 0x02);
                bool transparentBackground = !src;
                bool visible = value && (!backgroundSprite || transparentBackground) && ((src & PALETTE_BASE_OBP) == 0);
                value |= palette;
                value = visible ? value : src;
                dest[offsetX] = value;
            }
        }
    }

    void Display::blitLine(uint32_t* dest, uint8_t* src, uint32_t count)
    {
        for (uint32_t index = 0; index < count; ++index)
        {
            uint8_t value = src[index];
            EMU_ASSERT(value < mPalette.size());
            uint32_t color = mPalette[value];
            dest[index] = color;
        }
    }

    void Display::renderLines(uint32_t firstLine, uint32_t lastLine)
    {
        uint8_t rowStorage[RENDER_ROW_STORAGE];
        uint8_t bgTileRowStorage[RENDER_TILE_STORAGE];
        uint8_t bgAttrRowStorage[RENDER_TILE_STORAGE];
        uint8_t windowTileRowStorage[RENDER_TILE_STORAGE];
        uint8_t windowAttrRowStorage[RENDER_TILE_STORAGE];

        bool lcdEnabled = (mRegLCDC & LCDC_LCD_ENABLE) != 0;
        uint16_t windowTileMapOffset = (mRegLCDC & LCDC_WINDOW_TILE_MAP) ? 0x1c00 : 0x1800;
        bool windowEnabled = (mRegLCDC & LCDC_WINDOW_ENABLE) != 0;
        bool bgFirstTilePattern = (mRegLCDC & LCDC_TILE_DATA) != 0;
        uint16_t bgTileMapOffset = (mRegLCDC & LCDC_BG_TILE_MAP) ? 0x1c00 : 0x1800;
        bool bgEnabled = (mRegLCDC & LCDC_BG_ENABLE) != 0;
        uint8_t spriteSizeY = (mRegLCDC & LCDC_SPRITES_SIZE) ? 16 : 8;
        bool spritesEnabled = (mRegLCDC & LCDC_SPRITES_ENABLE) != 0;
        bool isColor = mConfig.model >= gb::Model::GBC;

        if (spritesEnabled && !mSortedSprites)
            sortSprites();

        if (lcdEnabled && !mCachedPalette)
            updatePalette();

        if (!lcdEnabled || !bgEnabled)
            memset(rowStorage, 0, RENDER_ROW_STORAGE);
        else if (!isColor)
        {
            memset(bgAttrRowStorage, 0, RENDER_TILE_STORAGE);
            memset(windowAttrRowStorage, 0, RENDER_TILE_STORAGE);
        }

        uint8_t spritePaletteMask = isColor ? SPRITE_FLAG_COLOR_PALETTE_MASK : SPRITE_FLAG_MONO_PALETTE_MASK;
        uint8_t spritePaletteShift = isColor ? SPRITE_FLAG_COLOR_PALETTE_SHIFT : SPRITE_FLAG_MONO_PALETTE_SHIFT;
        uint8_t spriteBankMask = isColor ? SPRITE_FLAG_COLOR_BANK_MASK : 0x00;
        uint8_t spritesTileBias = 0x00;
        auto spritesTilePatterns = mVRAM.data() + 0x0000;

        uint8_t windowTileBias = 0x80;
        auto windowTilePatterns = mVRAM.data() + 0x0800;

        uint8_t bgTileBias = bgFirstTilePattern ? spritesTileBias : windowTileBias;
        auto bgTilePatterns = bgFirstTilePattern ? spritesTilePatterns : windowTilePatterns;

        windowEnabled = windowEnabled && (mRegWX < DISPLAY_SIZE_X + 7);
        uint8_t windowTileCountX = windowEnabled ? (DISPLAY_SIZE_X + 7 - mRegWX + 7) >> 3 : 0;
        uint8_t windowPrevTileY = 0xff;
        auto windowTileMap = mVRAM.data() + windowTileMapOffset;
        auto windowTileOffset = 8 - 7 + mRegWX;
        auto windowTileDest = rowStorage + windowTileOffset;
        EMU_ASSERT(!windowEnabled || (windowTileCountX <= RENDER_TILE_STORAGE));
        EMU_ASSERT(!windowEnabled || (windowTileOffset + (windowTileCountX << 3) <= RENDER_ROW_STORAGE));

        uint8_t bgTileX = (mRegSCX >> 3) & 0xff;
        uint8_t bgTileOffsetX = mRegSCX & 0x07;
        uint8_t bgLineY = (mRegSCY + firstLine) & 0xff;
        uint8_t bgPrevTileY = 0xff;
        auto bgTileMap = mVRAM.data() + bgTileMapOffset;
        auto bgTileDest = rowStorage + 8 - bgTileOffsetX;

        static bool dumpVRAM = false;
        if (dumpVRAM)
        {
            dumpVRAM = false;
            auto file = fopen("..\\vram.bin", "wb");
            fwrite(mVRAM.data(), 1, mVRAM.size(), file);
            fclose(file);
        }

        auto dest = mSurface + mPitch * firstLine;
        for (uint32_t line = firstLine; line <= lastLine; ++line)
        {
            if (lcdEnabled)
            {
                bool hasWindow = windowEnabled && (line >= mRegWY);

                if (bgEnabled)
                {
                    uint8_t bgTileY = bgLineY >> 3;
                    uint8_t bgTileOffsetY = bgLineY & 0x07;
                    if (bgPrevTileY != bgTileY)
                    {
                        bgPrevTileY = bgTileY;
                        fetchTileRow(bgTileRowStorage, bgTileMap, bgTileX, bgTileY, bgTileBias, RENDER_TILE_STORAGE);
                        if (isColor)
                            fetchAttrRow(bgAttrRowStorage, bgTileMap, bgTileX, bgTileY, 0, RENDER_TILE_STORAGE);
                    }
                    drawTiles(bgTileDest, bgTileRowStorage, bgAttrRowStorage, bgTilePatterns, bgTileOffsetY, RENDER_TILE_STORAGE);
                    ++bgLineY;
                }

                if (hasWindow)
                {
                    uint8_t windowLineY = line - mRegWY;
                    uint8_t windowTileY = windowLineY >> 3;
                    uint8_t windowTileOffsetY = windowLineY & 0x07;
                    if (windowPrevTileY != windowTileY)
                    {
                        windowPrevTileY = windowTileY;
                        fetchTileRow(windowTileRowStorage, windowTileMap, 0, windowTileY, windowTileBias, windowTileCountX);
                        if (isColor)
                            fetchAttrRow(windowAttrRowStorage, windowTileMap, 0, windowTileY, windowTileBias, windowTileCountX);
                    }
                    drawTiles(windowTileDest, windowTileRowStorage, windowAttrRowStorage, windowTilePatterns, windowTileOffsetY, windowTileCountX);
                }

                if (spritesEnabled)
                    drawSprites(rowStorage, line, spriteSizeY, spritePaletteMask, spritePaletteShift, spriteBankMask);
            }
            blitLine(reinterpret_cast<uint32_t*>(dest), rowStorage + 8, DISPLAY_SIZE_X);
            dest += mPitch;
        }
    }

    void Display::render(int32_t tick)
    {
        if (mRenderedTick < tick)
        {
            if (!mConfig.fastLineRendering)
            {
                EMU_NOT_IMPLEMENTED();
            }

            // Find first line
            updateRasterPos(tick);
            int32_t firstLine = mRenderedLine;

            // Find last line
            int32_t lastLine = mRasterLine;
            if (tick < mLineFirstTick + mMode3StartTick)
                --lastLine;
            if (lastLine >= DISPLAY_SIZE_Y)
                lastLine = DISPLAY_SIZE_Y - 1;

            // Render new lines
            if (mSurface && (firstLine <= lastLine))
                renderLines(firstLine, lastLine);

            mRenderedLine = lastLine + 1;
            mRenderedTick = tick;
            mRenderedLineFirstTick = mLineFirstTick;
        }
    }
}
