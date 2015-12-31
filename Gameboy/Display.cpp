#include <Core/RegisterBank.h>
#include <Core/Serialization.h>
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

    static const uint32_t RENDER_ROW_STORAGE = DISPLAY_SIZE_X + 16;
    static const uint32_t RENDER_TILE_COUNT = DISPLAY_SIZE_X / 8;
    static const uint32_t RENDER_TILE_STORAGE = RENDER_TILE_COUNT + 1;

    static const uint32_t PALETTE_MONO_BASE_BGP = 0;
    static const uint32_t PALETTE_MONO_BASE_OBP0 = 4;
    static const uint32_t PALETTE_MONO_BASE_OBP1 = 8;
    static const uint32_t PALETTE_MONO_BASE_BGP_X4 = 0;
    static const uint32_t PALETTE_MONO_BASE_OBP0_X4 = 4;
    static const uint32_t PALETTE_MONO_BASE_OBP1_X4 = 8;
    static const uint32_t PALETTE_MONO_SIZE = 12;

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
    static const uint8_t SPRITE_FLAG_COLOR_PALETTE_MASK = 0x03;
    static const uint8_t SPRITE_FLAG_COLOR_PALETTE_SHIFT = 0;

    static const uint8_t kMonoPalette[4][4] =
    {
        { 0xff, 0xff, 0xff, 0xff },
        { 0xbb, 0xbb, 0xbb, 0xff },
        { 0x55, 0x55, 0x55, 0xff },
        { 0x00, 0x00, 0x00, 0x00 },
    };

    static uint8_t patternMask[256][8];

    bool initializePatternTable()
    {
        for (uint32_t pattern = 0; pattern < 256; ++pattern)
        {
            for (uint32_t bit = 0; bit < 8; ++bit)
            {
                uint8_t mask = ((pattern << bit) & 0x80) ? 0xff : 0x00;
                patternMask[pattern][bit] = mask;
            }
        }
        return true;
    }

    bool patternTableInitialized = initializePatternTable();
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
        , fastDma(true)
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
        mUpdateRasterPosFast    = 0;
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
        mActiveSprites          = 0;
        mSortedSprites          = false;
        resetClock();
    }

    bool Display::create(Config& config, emu::Clock& clock, uint32_t master_clock_divider, emu::MemoryBus& memory, Interrupts& interrupts, emu::RegisterBank& registers)
    {
        mTicksPerLine = TICKS_PER_LINE * master_clock_divider;
        mVBlankStartTick = DISPLAY_SIZE_Y * mTicksPerLine;
        mUpdateRasterPosFast = UPDATE_RASTER_POS_FAST * master_clock_divider;

        mClock = &clock;
        mMemory = &memory.getState();
        mInterrupts = &interrupts;
        mConfig = config;

        EMU_VERIFY(mClockListener.create(clock, *this));

        bool isGBC = mConfig.model >= gb::Model::GBC;
        mVRAM.resize(isGBC ? VRAM_SIZE_GBC : VRAM_SIZE_GB, 0);
        EMU_VERIFY(memory.addMemoryRange(VRAM_BANK_START, VRAM_BANK_END, mMemoryVRAM.setReadWriteMemory(mVRAM.data() + mBankVRAM * VRAM_BANK_SIZE)));

        mOAM.resize(OAM_SIZE, 0);
        mOAMOrder.resize(SPRITE_CAPACITY);
        mMemoryOAM.read.setReadMemory(mOAM.data());
        mMemoryOAM.write.setWriteMethod(&onWriteOAM, this);
        EMU_VERIFY(memory.addMemoryRange(0xfe00, 0xfe9f, mMemoryOAM));

        mMemoryNotUsable.write.setWriteMethod(&onWriteNotUsable, this, 0);
        EMU_VERIFY(memory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_WRITE, 0xfea0, 0xfeff, mMemoryNotUsable.write));

        if (mConfig.model >= gb::Model::GBC)
        {
            EMU_NOT_IMPLEMENTED();
        }
        mPalette.resize(PALETTE_MONO_SIZE, *reinterpret_cast<const uint32_t*>(&kMonoPalette[0]));
        
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
            mSimulatedTick = mDesiredTick;
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
    }

    void Display::setDesiredTicks(int32_t tick)
    {
        mDesiredTick = tick;
    }

    void Display::beginFrame()
    {
        mClock->addEvent(onVBlankStart, this, mVBlankStartTick);
        mRasterLine -= DISPLAY_LINE_COUNT;
        mRenderedLine = 0;
        mRenderedLineFirstTick = 0;
        mRenderedTick = 0;
    }

    uint8_t Display::readLCDC(int32_t tick, uint16_t addr)
    {
        return mRegLCDC;
    }

    void Display::writeLCDC(int32_t tick, uint16_t addr, uint8_t value)
    {
        if (mRegLCDC != value)
        {
            render(tick);
            bool spriteSizeChanged = ((mRegLCDC ^ value) & LCDC_SPRITES_SIZE) != 0;
            if (spriteSizeChanged)
                mSortedSprites = false;
            mRegLCDC = value;
        }
    }

    uint8_t Display::readSTAT(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegSTAT;
    }

    void Display::writeSTAT(int32_t tick, uint16_t addr, uint8_t value)
    {
        uint8_t masked = value & STAT_WRITE_MASK;
        uint8_t modified = mRegSTAT ^ masked;
        if (modified & masked & STAT_LYC_LY_INT)
        {
            EMU_NOT_IMPLEMENTED();
        }
        if (modified & masked & STAT_OAM_INT)
        {
            EMU_NOT_IMPLEMENTED();
        }
        if (modified & masked & STAT_VBLANK_INT)
        {
            EMU_NOT_IMPLEMENTED();
        }
        if (modified & masked & STAT_HBLANK_INT)
        {
            EMU_NOT_IMPLEMENTED();
        }
        mRegSTAT = masked;
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
        return mRegLY;
    }

    void Display::writeLY(int32_t tick, uint16_t addr, uint8_t value)
    {
        EMU_NOT_IMPLEMENTED();
        mRegLY = value;
    }

    uint8_t Display::readLYC(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegLYC;
    }

    void Display::writeLYC(int32_t tick, uint16_t addr, uint8_t value)
    {
        EMU_NOT_IMPLEMENTED();
        mRegLYC = value;
    }

    void Display::writeDMA(int32_t tick, uint16_t addr, uint8_t value)
    {
        if (!mConfig.fastDma)
        {
            EMU_NOT_IMPLEMENTED();
        }

        // Fast DMA implementation: transfer all data in the same tick
        render(tick);
        uint16_t read_addr = value << 8;

        for (uint8_t index = 0; index < DMA_SIZE; ++index)
        {
            uint8_t dma_value = memory_bus_read8(*mMemory, tick, read_addr + index);
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
            updateMonoPalette(PALETTE_MONO_BASE_BGP, value);
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
            updateMonoPalette(PALETTE_MONO_BASE_OBP0, value);
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
            updateMonoPalette(PALETTE_MONO_BASE_OBP1, value);
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
        updateMonoPalette(PALETTE_MONO_BASE_BGP, mRegBGP);
        updateMonoPalette(PALETTE_MONO_BASE_OBP0, mRegOBP0);
        updateMonoPalette(PALETTE_MONO_BASE_OBP1, mRegOBP1);
    }

    void Display::serialize(emu::ISerializer& serializer)
    {
        serializer.serialize(mVRAM);
        serializer.serialize(mOAM);
        serializer.serialize(mPalette);
        serializer.serialize(mSimulatedTick);
        serializer.serialize(mRenderedLine);
        serializer.serialize(mRenderedLineFirstTick);
        serializer.serialize(mRenderedTick);
        serializer.serialize(mDesiredTick);
        serializer.serialize(mTicksPerLine);
        serializer.serialize(mVBlankStartTick);
        serializer.serialize(mUpdateRasterPosFast);
        serializer.serialize(mLineFirstTick);
        serializer.serialize(mLineTick);
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
        if (serializer.isReading())
            mSortedSprites = false;
    }

    void Display::setRenderSurface(void* surface, size_t pitch)
    {
        mSurface = static_cast<uint8_t*>(surface);
        mPitch = pitch;
    }

    void Display::onVBlankStart(int32_t tick)
    {
        render(tick);
        mInterrupts->setInterrupt(tick, gb::Interrupts::Signal::VBlank);
    }

    void Display::updateRasterPos(int32_t tick)
    {
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

    void Display::updateMonoPalette(uint32_t base, uint8_t value)
    {
        if (mConfig.model == gb::Model::GB)
        {
            for (uint32_t index = 0; index < 4; ++index)
            {
                uint8_t shade = value & 0x3;
                value >>= 2;
                mPalette[base + index] = *reinterpret_cast<const uint32_t*>(kMonoPalette[shade]);
            }
        }
    }

    void Display::sortMonoSprites()
    {
        uint16_t keys[SPRITE_CAPACITY];

        mActiveSprites = 0;
        uint8_t spriteSizeY = (mRegLCDC & LCDC_SPRITES_SIZE) ? 16 : 8;
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

    void Display::drawTiles(uint8_t* dest, const uint8_t* tiles, const uint8_t* attributes, const uint8_t* patterns, uint16_t count)
    {
        for (uint32_t index = 0; index < count; ++index)
        {
            uint32_t tile = tiles[index];
            uint32_t patternBase = tile << 4;
            auto pattern0 = reinterpret_cast<const uint32_t*>(&patternMask[patterns[patternBase + 0]][0]);
            auto pattern1 = reinterpret_cast<const uint32_t*>(&patternMask[patterns[patternBase + 1]][0]);
            auto value0 = (pattern0[0] & 0x01010101) + (pattern1[0] & 0x02020202) + PALETTE_MONO_BASE_BGP_X4;
            auto value1 = (pattern0[1] & 0x01010101) + (pattern1[1] & 0x02020202) + PALETTE_MONO_BASE_BGP_X4;
            reinterpret_cast<uint32_t*>(dest)[0] = value0;
            reinterpret_cast<uint32_t*>(dest)[1] = value1;
            dest += 8;
        }
    }

    void Display::drawSpritesMono(uint8_t* dest, uint8_t line, uint8_t spriteSizeY)
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
            uint32_t tileAddr = (tile << 4) + (offsetY << 1);
            uint8_t pattern0 = mVRAM[tileAddr + 0];
            uint8_t pattern1 = mVRAM[tileAddr + 1];
            const uint8_t* patternMask0 = patternMask[pattern0];
            const uint8_t* patternMask1 = patternMask[pattern1];

            // Get the palette
            uint8_t palette = (((flags & SPRITE_FLAG_MONO_PALETTE_MASK) >> SPRITE_FLAG_MONO_PALETTE_SHIFT) << 2) + PALETTE_MONO_BASE_OBP0;

            // Render pixels
            bool backgroundSprite = (flags & SPRITE_FLAG_BACKGROUND) != 0;
            for (uint8_t bit = 0; bit < spriteSizeX; ++bit)
            {
                uint8_t offsetX = spriteX + (bit ^ flipX);
                uint8_t src = dest[offsetX];

                uint8_t value = (patternMask0[bit] & 0x01) | (patternMask1[bit] & 0x02);
                bool transparentBackground = !src;
                bool visible = value && (!backgroundSprite || transparentBackground);
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

    void Display::renderLinesMono(uint32_t firstLine, uint32_t lastLine)
    {
        uint8_t rowStorage[RENDER_ROW_STORAGE];
        uint8_t bgTileRowStorage[RENDER_TILE_STORAGE];
        uint8_t windowTileRowStorage[RENDER_TILE_STORAGE];

        bool lcdEnabled = (mRegLCDC & LCDC_LCD_ENABLE) != 0;
        uint16_t windowTileMapOffset = (mRegLCDC & LCDC_WINDOW_TILE_MAP) ? 0x1c00 : 0x1800;
        bool windowEnabled = (mRegLCDC & LCDC_WINDOW_ENABLE) != 0;
        bool firstTilePattern = (mRegLCDC & LCDC_TILE_DATA) == 0;
        uint16_t bgTileMapOffset = (mRegLCDC & LCDC_BG_TILE_MAP) ? 0x1c00 : 0x1800;
        bool bgEnabled = (mRegLCDC & LCDC_BG_ENABLE) != 0;
        uint8_t spriteSizeY = (mRegLCDC & LCDC_SPRITES_SIZE) ? 16 : 8;
        bool spritesEnabled = (mRegLCDC & LCDC_SPRITES_ENABLE) != 0;

        if (spritesEnabled && !mSortedSprites)
            sortMonoSprites();

        if (!lcdEnabled || !bgEnabled)
            memset(rowStorage, 0, RENDER_ROW_STORAGE);

        uint16_t tilePatternOffset = firstTilePattern ? 0x0800 : 0x0000;
        uint8_t tileBias = firstTilePattern ? 0x80 : 0x00;
        auto tilePatterns = mVRAM.data() + tilePatternOffset;

        uint8_t windowTileCountX = (windowEnabled && (mRegWX < DISPLAY_SIZE_X + 7)) ? (DISPLAY_SIZE_X + 7 - mRegWX + 7) >> 3 : 0;
        uint8_t windowPrevTileY = 0xff;
        auto windowTileMap = mVRAM.data() + windowTileMapOffset;
        auto windowTileOffset = 8 - 7 + (mRegWX & 0x07);
        auto windowTileDest = rowStorage + windowTileOffset;
        EMU_ASSERT(windowTileCountX <= RENDER_TILE_STORAGE);
        EMU_ASSERT(windowTileOffset + (windowTileCountX << 3) <= RENDER_ROW_STORAGE);

        uint8_t bgTileX = (mRegSCX >> 3) & 0xff;
        uint8_t bgTileOffsetX = mRegSCX & 0x07;
        uint8_t bgLineY = (mRegSCY + firstLine) & 0xff;
        uint8_t bgPrevTileY = 0xff;
        auto bgTileMap = mVRAM.data() + bgTileMapOffset;
        auto bgTileDest = rowStorage + 8 - bgTileOffsetX;

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
                        fetchTileRow(bgTileRowStorage, bgTileMap, bgTileX, bgTileY, tileBias, RENDER_TILE_STORAGE);
                    }
                    drawTiles(bgTileDest, bgTileRowStorage, nullptr, tilePatterns + (bgTileOffsetY << 1), RENDER_TILE_STORAGE);
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
                        fetchTileRow(windowTileRowStorage, windowTileMap, 0, windowTileY, tileBias, windowTileCountX);
                    }
                    drawTiles(windowTileDest, windowTileRowStorage, nullptr, tilePatterns + (windowTileOffsetY << 1), windowTileCountX);
                }

                if (spritesEnabled)
                    drawSpritesMono(rowStorage, line, spriteSizeY);
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

            updateRasterPos(tick);
            if (mSurface)
            {
                // Find first line
                uint32_t firstLine = mRenderedLine;
                if (mRenderedTick > mRenderedLineFirstTick)
                    ++firstLine;

                // Find last line
                uint32_t lastLine = mRasterLine;
                if (lastLine >= DISPLAY_SIZE_Y)
                    lastLine = DISPLAY_SIZE_Y - 1;

                if (firstLine <= lastLine)
                {
                    renderLinesMono(firstLine, lastLine);
                }
            }
            mRenderedLine = mRasterLine;
            mRenderedTick = tick;
            mRenderedLineFirstTick = mLineFirstTick;
        }
    }
}
