#include "PPU.h"
#include <assert.h>
#include <vector>

namespace
{
    static const uint32_t MEM_SIZE_LOG2 = 14;
    static const uint32_t MEM_SIZE = 1 << MEM_SIZE_LOG2;
    static const uint32_t MEM_MASK = MEM_SIZE - 1;
    static const uint32_t MEM_PAGE_SIZE_LOG2 = 10;
    static const uint32_t MEM_PAGE_SIZE = 1 << MEM_PAGE_SIZE_LOG2;

    static const uint32_t VRAM_PATTERN_TABLE_SIZE = 0x1000;
    static const uint32_t VRAM_PATTERN_TABLE_ADDRESS0 = 0x0000;
    static const uint32_t VRAM_PATTERN_TABLE_ADDRESS1 = 0x1000;

    static const uint32_t VRAM_NAMETABLE_BASE_ADDRESS = 0x2000;
    static const uint32_t VRAM_NAMETABLE_SIZE = 0x400;
    static const uint32_t VRAM_NAMETABLE_OFFSET0 = 0x000;
    static const uint32_t VRAM_NAMETABLE_OFFSET1 = 0x400;
    static const uint32_t VRAM_NAMETABLE_OFFSET2 = 0x800;
    static const uint32_t VRAM_NAMETABLE_OFFSET3 = 0xc00;

    static const uint32_t PALETTE_RAM_BASE_ADDRESS = 0x3f00;
    static const uint32_t PALETTE_RAM_END_ADDRESS = 0x3fff;
    static const uint32_t PALETTE_RAM_SIZE = 0x20;
    static const uint32_t PALETTE_RAM_MASK = PALETTE_RAM_SIZE - 1;

    static const uint32_t OAM_SIZE = 0x100;

    static const uint8_t PPU_CONTROL_NAMETABLE_PAGE_X = 0x01;
    static const uint8_t PPU_CONTROL_NAMETABLE_PAGE_Y = 0x02;
    static const uint8_t PPU_CONTROL_VERTICAL_INCREMENT = 0x04;
    static const uint8_t PPU_CONTROL_BACKGROUND_PATTERN_TABLE = 0x10;
    static const uint8_t PPU_CONTROL_SPRITE_SIZE = 0x20;
    static const uint8_t PPU_CONTROL_MASTER_SLAVE = 0x40;
    static const uint8_t PPU_CONTROL_VBLANK_OUTPUT = 0x80;

    static const uint8_t PPU_STATUS_VBLANK = 0x80;
    static const uint8_t PPU_STATUS_HIT_TEST = 0x40;

    static const uint32_t PPU_TICKS_PER_LINE = 341;
    static const uint32_t PPU_VISIBLE_LINES = 240;
    static const uint32_t PPU_VISIBLE_COLUMNS = 256;
    static const uint32_t PPU_VISIBLE_START_LINE = 1;
    static const uint32_t PPU_VISIBLE_START_COLUMN = 4;
    static const uint32_t PPU_VBLANK_START_LINE = PPU_VISIBLE_START_LINE + PPU_VISIBLE_LINES;
    static const uint32_t PPU_VBLANK_END_LINE = 0;
    static const uint32_t PPU_VBLANK_START_TICKS = PPU_VBLANK_START_LINE * PPU_TICKS_PER_LINE;
    static const uint32_t PPU_VBLANK_END_TICKS = PPU_VBLANK_END_LINE * PPU_TICKS_PER_LINE;

    static const uint32_t SCANLINE_NAME_ALIGNMENT = 8;
    static const uint32_t SCANLINE_ATTRIBUTE_ALIGNMENT = 32;
    static const uint32_t SCANLINE_ATTRIBUTE_REPETITION = 8;
    static const uint32_t SCANLINE_PIXEL_CAPACITY = PPU_VISIBLE_COLUMNS + SCANLINE_ATTRIBUTE_ALIGNMENT;
    static const uint32_t SCANLINE_NAME_COUNT = SCANLINE_PIXEL_CAPACITY / SCANLINE_NAME_ALIGNMENT;
    static const uint32_t SCANLINE_ATTRIBUTE_COUNT = SCANLINE_PIXEL_CAPACITY / SCANLINE_ATTRIBUTE_ALIGNMENT;

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

    void NOT_IMPLEMENTED()
    {
        printf("Feature not implemented\n");
        assert(false);
    }

    uint8_t patternTableNotImplementedRead(void* contex, int32_t ticks, uint32_t addr)
    {
        //NOT_IMPLEMENTED();
        return 0;
    }

    void patternTableNotImplementedWrite(void* contex, int32_t ticks, uint32_t addr, uint8_t value)
    {
        //NOT_IMPLEMENTED();
    }
}

namespace NES
{
    PPU::PPU()
        : mClock(nullptr)
        , mVBlankStartTicks(0)
        , mVBlankEndTicks(0)
        , mTicksPerLine(0)
        , mSprite0StartTick(0)
        , mSprite0EndTick(0)
        , mVisibleLines(0)
        , mAddressLow(false)
        , mVisibleArea(false)
        , mCheckHitTest(false)
        , mScrollIndex(0)
        , mAddress(0)
        , mDataReadBuffer(0)
        , mSurface(nullptr)
        , mPitch(0)
        , mLastTickRendered(0)
    {
        memset(mRegister, 0, sizeof(mRegister));
    }

    PPU::~PPU()
    {
        destroy();
    }

    bool PPU::create(Clock& clock, uint32_t masterClockDivider, uint32_t createFlags, uint32_t visibleLines)
    {
        mClock = &clock;
        mClock->addListener(*this);
        mMasterClockDivider = masterClockDivider;
        mVBlankStartTicks = PPU_VBLANK_START_TICKS * masterClockDivider;
        mVBlankEndTicks = PPU_VBLANK_END_TICKS * masterClockDivider;
        mTicksPerLine = PPU_TICKS_PER_LINE * masterClockDivider;
        mVisibleLines = visibleLines;

        if (!mMemory.create(MEM_SIZE_LOG2, MEM_PAGE_SIZE_LOG2))
            return false;

        // Initialize pattern table
        mPatternTableRead[0].setReadMethod(patternTableNotImplementedRead, nullptr);
        mPatternTableRead[1].setReadMethod(patternTableNotImplementedRead, nullptr);
        mPatternTableWrite[0].setWriteMethod(patternTableNotImplementedWrite, nullptr);
        mPatternTableWrite[1].setWriteMethod(patternTableNotImplementedWrite, nullptr);
        mMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_READ, VRAM_PATTERN_TABLE_ADDRESS0, VRAM_PATTERN_TABLE_ADDRESS0 + VRAM_PATTERN_TABLE_SIZE - 1, mPatternTableRead[0]);
        mMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_READ, VRAM_PATTERN_TABLE_ADDRESS1, VRAM_PATTERN_TABLE_ADDRESS1 + VRAM_PATTERN_TABLE_SIZE - 1, mPatternTableRead[1]);
        mMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_WRITE, VRAM_PATTERN_TABLE_ADDRESS0, VRAM_PATTERN_TABLE_ADDRESS0 + VRAM_PATTERN_TABLE_SIZE - 1, mPatternTableWrite[0]);
        mMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_WRITE, VRAM_PATTERN_TABLE_ADDRESS1, VRAM_PATTERN_TABLE_ADDRESS1 + VRAM_PATTERN_TABLE_SIZE - 1, mPatternTableWrite[1]);

        // Initialize name tables
        uint32_t vramFlags = createFlags & CREATE_VRAM_MASK;
        uint32_t vramSize = 0;
        if (vramFlags == CREATE_VRAM_ONE_SCREEN)
        {
            mNameTableRAM.resize(VRAM_NAMETABLE_SIZE);
            mNameTableRead[0].setReadMemory(&mNameTableRAM[VRAM_NAMETABLE_OFFSET0]);
            mNameTableRead[1].setReadMemory(&mNameTableRAM[VRAM_NAMETABLE_OFFSET0]);
            mNameTableRead[2].setReadMemory(&mNameTableRAM[VRAM_NAMETABLE_OFFSET0]);
            mNameTableRead[3].setReadMemory(&mNameTableRAM[VRAM_NAMETABLE_OFFSET0]);
            mNameTableWrite[0].setWriteMemory(&mNameTableRAM[VRAM_NAMETABLE_OFFSET0]);
            mNameTableWrite[1].setWriteMemory(&mNameTableRAM[VRAM_NAMETABLE_OFFSET0]);
            mNameTableWrite[2].setWriteMemory(&mNameTableRAM[VRAM_NAMETABLE_OFFSET0]);
            mNameTableWrite[3].setWriteMemory(&mNameTableRAM[VRAM_NAMETABLE_OFFSET0]);
        }
        else if (vramFlags == CREATE_VRAM_VERTICAL_MIRROR)
        {
            mNameTableRAM.resize(VRAM_NAMETABLE_SIZE * 2);
            mNameTableRead[0].setReadMemory(&mNameTableRAM[VRAM_NAMETABLE_OFFSET0]);
            mNameTableRead[1].setReadMemory(&mNameTableRAM[VRAM_NAMETABLE_OFFSET1]);
            mNameTableRead[2].setReadMemory(&mNameTableRAM[VRAM_NAMETABLE_OFFSET0]);
            mNameTableRead[3].setReadMemory(&mNameTableRAM[VRAM_NAMETABLE_OFFSET1]);
            mNameTableWrite[0].setWriteMemory(&mNameTableRAM[VRAM_NAMETABLE_OFFSET0]);
            mNameTableWrite[1].setWriteMemory(&mNameTableRAM[VRAM_NAMETABLE_OFFSET1]);
            mNameTableWrite[2].setWriteMemory(&mNameTableRAM[VRAM_NAMETABLE_OFFSET0]);
            mNameTableWrite[3].setWriteMemory(&mNameTableRAM[VRAM_NAMETABLE_OFFSET1]);
        }
        else if (vramFlags == CREATE_VRAM_HORIZONTAL_MIRROR)
        {
            mNameTableRAM.resize(VRAM_NAMETABLE_SIZE * 2);
            mNameTableRead[0].setReadMemory(&mNameTableRAM[VRAM_NAMETABLE_OFFSET0]);
            mNameTableRead[1].setReadMemory(&mNameTableRAM[VRAM_NAMETABLE_OFFSET0]);
            mNameTableRead[2].setReadMemory(&mNameTableRAM[VRAM_NAMETABLE_OFFSET1]);
            mNameTableRead[3].setReadMemory(&mNameTableRAM[VRAM_NAMETABLE_OFFSET1]);
            mNameTableWrite[0].setWriteMemory(&mNameTableRAM[VRAM_NAMETABLE_OFFSET0]);
            mNameTableWrite[1].setWriteMemory(&mNameTableRAM[VRAM_NAMETABLE_OFFSET0]);
            mNameTableWrite[2].setWriteMemory(&mNameTableRAM[VRAM_NAMETABLE_OFFSET1]);
            mNameTableWrite[3].setWriteMemory(&mNameTableRAM[VRAM_NAMETABLE_OFFSET1]);
        }
        else
        {
            mNameTableRAM.resize(VRAM_NAMETABLE_SIZE * 4);
            mNameTableRead[0].setReadMemory(&mNameTableRAM[VRAM_NAMETABLE_OFFSET0]);
            mNameTableRead[1].setReadMemory(&mNameTableRAM[VRAM_NAMETABLE_OFFSET1]);
            mNameTableRead[2].setReadMemory(&mNameTableRAM[VRAM_NAMETABLE_OFFSET2]);
            mNameTableRead[3].setReadMemory(&mNameTableRAM[VRAM_NAMETABLE_OFFSET3]);
            mNameTableWrite[0].setWriteMemory(&mNameTableRAM[VRAM_NAMETABLE_OFFSET0]);
            mNameTableWrite[1].setWriteMemory(&mNameTableRAM[VRAM_NAMETABLE_OFFSET1]);
            mNameTableWrite[2].setWriteMemory(&mNameTableRAM[VRAM_NAMETABLE_OFFSET2]);
            mNameTableWrite[3].setWriteMemory(&mNameTableRAM[VRAM_NAMETABLE_OFFSET3]);
        }
        for (uint16_t mirror = 0; mirror < 2; ++mirror)
        {
            uint16_t addrStart = VRAM_NAMETABLE_BASE_ADDRESS + mirror * 0x1000;
            uint16_t addrEnd = addrStart + 0x03ff;
            for (uint16_t page = 0; page < 4; ++page)
            {
                mMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_READ, addrStart, addrEnd, mNameTableRead[page]);
                mMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_WRITE, addrStart, addrEnd, mNameTableWrite[page]);
                addrStart += 0x0400;
                addrEnd += 0x0400;
                if (addrEnd >= PALETTE_RAM_BASE_ADDRESS)
                    addrEnd = PALETTE_RAM_BASE_ADDRESS - 1;
            }
        }

        // Initialize palette RAM
        mPaletteRAM.resize(PALETTE_RAM_SIZE);
        mPaletteRead.setReadMethod(paletteRead, this);
        mPaletteWrite.setWriteMethod(paletteWrite, this);
        mMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_READ, PALETTE_RAM_BASE_ADDRESS, PALETTE_RAM_END_ADDRESS, mPaletteRead);
        mMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_WRITE, PALETTE_RAM_BASE_ADDRESS, PALETTE_RAM_END_ADDRESS, mPaletteWrite);

        // Initialize OAM
        mOAM.resize(OAM_SIZE, 0);

        return true;
    }

    void PPU::destroy()
    {
        mMemory.destroy();
        if (mClock)
            mClock->removeListener(*this);
        mClock = nullptr;
    }

    void PPU::reset()
    {
        mSprite0StartTick = 0;
        mSprite0EndTick = 0;
        memset(&mOAM[0], 0, mOAM.size());
        memset(mRegister, 0, sizeof(mRegister));
        mRegister[PPU_REG_PPUSTATUS] |= PPU_STATUS_VBLANK;
        mClock->addEvent(onVBlankEnd, this, mVBlankEndTicks);
        mClock->addEvent(onVBlankStart, this, mVBlankStartTicks);
        mAddressLow = false;
        mVisibleArea = false;
        mCheckHitTest = false;
        mScrollIndex = 0;
        mAddress = 0;
        mDataReadBuffer = 0;
        mLastTickRendered = 0;
    }

    void PPU::execute()
    {
    }

    MEM_ACCESS* PPU::getPatternTableRead(uint32_t index)
    {
        return index < PATTERN_TABLE_COUNT ? &mPatternTableRead[index] : nullptr;
    }

    MEM_ACCESS* PPU::getPatternTableWrite(uint32_t index)
    {
        return index < PATTERN_TABLE_COUNT ? &mPatternTableWrite[index] : nullptr;
    }

    MEM_ACCESS* PPU::getNameTableRead(uint32_t index)
    {
        return index < NAME_TABLE_COUNT ? &mNameTableRead[index] : nullptr;
    }

    MEM_ACCESS* PPU::getNameTableWrite(uint32_t index)
    {
        return index < NAME_TABLE_COUNT ? &mNameTableWrite[index] : nullptr;
    }

    void PPU::advanceClock(int32_t ticks)
    {
        mClock->addEvent(onVBlankEnd, this, mVBlankEndTicks);
        mClock->addEvent(onVBlankStart, this, mVBlankStartTicks);
    }

    void PPU::setDesiredTicks(int32_t ticks)
    {
    }

    uint8_t PPU::regRead(int32_t ticks, uint32_t addr)
    {
        addr = (addr & (PPU_REGISTER_COUNT - 1));
        switch (addr)
        {
            //case PPU_REG_PPUCTRL: break;
            //case PPU_REG_PPUMASK: break;

        case PPU_REG_PPUSTATUS:
        {
            uint8_t ppustatus = mRegister[PPU_REG_PPUSTATUS];
            mAddressLow = false;
            mScrollIndex = 0;
            endVBlank();
            checkHitTest(ticks);
            return ppustatus;
        }
            //case PPU_REG_OAMADDR: break;
            //case PPU_REG_OAMDATA: break;
            //case PPU_REG_PPUSCROLL: break;
            //case PPU_REG_PPUADDR: break;
        case PPU_REG_PPUDATA:
        {
            uint16_t address = mAddress & MEM_MASK;
            uint8_t data = memory_bus_read8(mMemory.getState(), 0, mAddress & MEM_MASK);
            uint8_t value = data;
            if (address < PALETTE_RAM_BASE_ADDRESS)
            {
                value = mDataReadBuffer;
                mDataReadBuffer = data;
            }
            uint16_t increment = (mRegister[PPU_REG_PPUCTRL] & PPU_CONTROL_VERTICAL_INCREMENT) ? 32 : 1;
            mAddress += increment;
            return value;
        }

        default:
            NOT_IMPLEMENTED();
        }
        uint8_t value = mRegister[addr];
        return value;
    }

    void PPU::regWrite(int32_t ticks, uint32_t addr, uint8_t value)
    {
        addr = (addr & (PPU_REGISTER_COUNT - 1));
        uint8_t before = mRegister[addr];
        uint8_t modified = before ^ value;
        mRegister[addr] = value;
        mRegister[PPU_REG_PPUSTATUS] = (mRegister[PPU_REG_PPUSTATUS] & ~0x1f) | (value & 0x1f);
        switch (addr)
        {
        case PPU_REG_PPUCTRL:
        {
            if (!(before & PPU_CONTROL_VBLANK_OUTPUT) &&
                (value & PPU_CONTROL_VBLANK_OUTPUT) &&
                (mRegister[PPU_REG_PPUSTATUS] & PPU_STATUS_VBLANK))
            {
                signalVBlankStart();
            }

            // Change in master/slave mode forces a change in the rendering
            if (modified & PPU_CONTROL_MASTER_SLAVE)
                render(ticks);

            // If sprite height changes, update hit test conditions
            if (modified & (PPU_CONTROL_SPRITE_SIZE | PPU_CONTROL_MASTER_SLAVE))
                updateSpriteHitTestConditions();
            break;
        }
        case PPU_REG_PPUMASK: break;
        //case PPU_REG_PPUSTATUS: break;
        case PPU_REG_OAMADDR:
            break;

        case PPU_REG_OAMDATA:
        {
            uint8_t addr = mRegister[PPU_REG_OAMADDR]++;
            mOAM[addr] = value;

            // If updating sprite 0, update hit test conditions
            if (addr < 4)
                updateSpriteHitTestConditions();
            break;
        }

        case PPU_REG_PPUSCROLL:
        {
            mScroll[mScrollIndex] = value;
            mScrollIndex = 1 - mScrollIndex;
            break;
        }

        case PPU_REG_PPUADDR:
        {
            if (mAddressLow)
                mAddress = ((mAddress & 0xff00) | (value & 0x00ff));
            else
                mAddress = ((mAddress & 0x00ff) | (value << 8));
            mAddressLow = !mAddressLow;
            break;
        }

        case PPU_REG_PPUDATA:
        {
            memory_bus_write8(mMemory.getState(), 0, mAddress & MEM_MASK, value);
            uint16_t increment = (mRegister[PPU_REG_PPUCTRL] & PPU_CONTROL_VERTICAL_INCREMENT) ? 32 : 1;
            mAddress += increment;
            break;
        }

        default:
            NOT_IMPLEMENTED();
        }
    }

    uint8_t PPU::paletteRead(int32_t ticks, uint32_t addr)
    {
        addr &= PALETTE_RAM_MASK;
        if ((addr & 3) == 0)
            addr &= 0xf;
        return mPaletteRAM[addr];
    }

    void PPU::paletteWrite(int32_t ticks, uint32_t addr, uint8_t value)
    {
        addr &= PALETTE_RAM_MASK;
        if ((addr & 3) == 0)
            addr &= 0xf;
        mPaletteRAM[addr] = value;
    }

    uint8_t PPU::paletteRead(void* context, int32_t ticks, uint32_t addr)
    {
        return static_cast<PPU*>(context)->paletteRead(ticks, addr);
    }

    void PPU::paletteWrite(void* context, int32_t ticks, uint32_t addr, uint8_t value)
    {
        static_cast<PPU*>(context)->paletteWrite(ticks, addr, value);
    }

    void PPU::startVBlank()
    {
        //printf("[VBLANK start]\n");
        uint8_t ppustatus = mRegister[PPU_REG_PPUSTATUS];
        uint8_t ppuctrl = mRegister[PPU_REG_PPUCTRL];
        if (!(ppustatus & PPU_STATUS_VBLANK))
        {
            mRegister[PPU_REG_PPUSTATUS] = ppustatus | PPU_STATUS_VBLANK;
            if (ppuctrl & PPU_CONTROL_VBLANK_OUTPUT)
                signalVBlankStart();
        }
    }

    void PPU::endVBlank()
    {
        uint8_t ppustatus = mRegister[PPU_REG_PPUSTATUS];
        //if (ppustatus & PPU_STATUS_VBLANK)
        //    printf("[VBLANK end]\n");
        mRegister[PPU_REG_PPUSTATUS] = ppustatus & ~PPU_STATUS_VBLANK;
    }

    void PPU::signalVBlankStart()
    {
        for (auto listener : mListeners)
            listener->onVBlankStart();
        //printf("[VBLANK interrupt]\n");
    }

    void PPU::onVBlankStart(int32_t ticks)
    {
        mVisibleArea = false;
        mCheckHitTest = false;
        startVBlank();
        render(ticks);
    }

    void PPU::onVBlankEnd()
    {
        endVBlank();
        mLastTickRendered = 0;
        mRegister[PPU_REG_PPUSTATUS] &= ~PPU_STATUS_HIT_TEST;
        mVisibleArea = true;

        // Refresh hit test conditions for this frame
        mCheckHitTest = true;
        updateSpriteHitTestConditions();
    }

    void PPU::onVBlankStart(void* context, int32_t ticks)
    {
        static_cast<PPU*>(context)->onVBlankStart(ticks);
    }

    void PPU::onVBlankEnd(void* context, int32_t ticks)
    {
        static_cast<PPU*>(context)->onVBlankEnd();
    }

    void PPU::addListener(IListener& listener)
    {
        mListeners.push_back(&listener);
    }

    void PPU::removeListener(IListener& listener)
    {
        auto item = std::find(mListeners.begin(), mListeners.end(), &listener);
        mListeners.erase(item);
    }

    void PPU::setRenderSurface(void* surface, size_t pitch)
    {
        mSurface = static_cast<uint8_t*>(surface);
        mPitch = pitch;
    }

    void PPU::getRasterPosition(int32_t tick, int32_t& x, int32_t& y)
    {
        x = (tick % mTicksPerLine);
        y = (tick / mTicksPerLine);
        x = (x / mMasterClockDivider) - PPU_VISIBLE_START_COLUMN;
        y = y - PPU_VISIBLE_START_LINE;
    }

    static uint32_t paletteIndex = 0;
    static uint32_t attributeIndex = 0;
    static uint32_t nameIndex = 0;

    void PPU::fetchPalette(uint8_t* dest)
    {
        for (uint32_t index = 0; index < PALETTE_RAM_SIZE; ++index)
        {
            //uint8_t value = paletteIndex++;
            uint8_t value = mPaletteRAM[index];
            if (value && !(value & 3))
                value = 0;  // Force background color if transparent
            value &= 63; // The last 2 bits should be zero
            dest[index] = value;
        }
    }

    void PPU::fetchAttributes(uint8_t* dest1, uint8_t* dest2, uint16_t base, uint16_t size)
    {
        MEMORY_BUS& memory = mMemory.getState();
        for (uint16_t index = 0; index < size; ++index)
        {
            // Read attribute
            //uint8_t attribute = attributeIndex++;
            uint8_t attribute = memory_bus_read8(memory, 0, base + index);

            // Store attributes in bit 2 and 3 of each channel
            dest1[0] = dest1[1] = (attribute << 2) & 0x0c;
            dest1[2] = dest1[3] = (attribute << 0) & 0x0c;
            dest2[0] = dest2[1] = (attribute >> 2) & 0x0c;
            dest2[2] = dest2[3] = (attribute >> 4) & 0x0c;
            dest1 += 4;
            dest2 += 4;
        }
    }

    void PPU::fetchNames(uint8_t* dest, uint16_t base, uint16_t size)
    {
        MEMORY_BUS& memory = mMemory.getState();
        for (uint16_t index = 0; index < size; ++index)
        {
            //uint8_t name = nameIndex++;
            uint8_t name = memory_bus_read8(memory, 0, base + index);
            dest[index] = name;
        }
    }

    void PPU::fetchBackground(uint8_t* dest, const uint8_t* names, const uint8_t* attributes, uint16_t base, uint16_t size)
    {
        MEMORY_BUS& memory = mMemory.getState();
        uint32_t base0 = base + 0;
        uint32_t base1 = base + 8;
        for (uint16_t index = 0; index < size; ++index)
        {
            // Fetch pattern
            uint32_t name = names[index];
            uint32_t offset = name * 16;
            uint32_t pattern0 = memory_bus_read8(memory, 0, base0 + offset);
            uint32_t pattern1 = memory_bus_read8(memory, 0, base1 + offset);
            const uint8_t* patternMask0 = &patternMask[pattern0][0];
            const uint8_t* patternMask1 = &patternMask[pattern1][0];

            // Transform color and attribute into palette entry
            uint8_t attribute = attributes[index];
            for (uint32_t bit = 0; bit < 8; ++bit)
            {
                uint8_t combined = 0;
                combined |= patternMask0[bit] & 1;
                combined |= patternMask1[bit] & 2;
                combined |= attribute;
                dest[bit] = combined;
            }
            dest += 8;
        }
    }

    void PPU::renderLine(uint8_t* dest, const uint8_t* names, const uint8_t* attributes, const uint8_t* palette, uint16_t patternBase, uint32_t count)
    {
        fetchBackground(dest, names, attributes, patternBase, count);
        count <<= 3;
        for (uint32_t index = 0; index < count; ++index)
        {
            dest[index] = palette[dest[index]];
        }
    }

    void PPU::render(int32_t lastTick)
    {
        if (!mSurface)
            return;

        if (lastTick <= mLastTickRendered)
            return;

        int32_t firstTick = mLastTickRendered;
        mLastTickRendered = lastTick;

        int32_t x0, y0;
        int32_t x1, y1;
        getRasterPosition(firstTick, x0, y0);
        getRasterPosition(lastTick, x1, y1);

        // Adjust starting position
        if (x0 < 0)
            x0 = 0;
        if (y0 < 0)
            y0 = 0;
        if (x1 < 0)
        {
            x1 = PPU_VISIBLE_COLUMNS - 1;
            --y1;
        }
        if (x0 > PPU_VISIBLE_COLUMNS)
        {
            x0 = 0;
            ++y0;
        }

        // Adjust ending position
        if (y1 >= static_cast<int32_t>(mVisibleLines))
        {
            y1 = mVisibleLines - 1;
            x1 = PPU_VISIBLE_COLUMNS - 1;
        }

        // Get scrolling position
        uint8_t ppuctrl = mRegister[PPU_REG_PPUCTRL];
        uint32_t scrollX = mScroll[0];
        uint32_t scrollY = mScroll[1];
        scrollX += ((ppuctrl & PPU_CONTROL_NAMETABLE_PAGE_X) ? 256 : 0);
        scrollY += ((ppuctrl & PPU_CONTROL_NAMETABLE_PAGE_Y) ? 240 : 0);
        scrollY += y0;
        if (scrollY >= 480)
            scrollY -= 480;

        // Find location of starting position
        uint32_t page1 = 0;
        uint32_t page2 = 1;
        if (scrollY >= 240)
        {
            scrollY -= 240;
            page1 ^= 2;
            page2 ^= 2;
        }
        if (scrollX >= 256)
        {
            scrollX -= 256;
            page1 ^= 1;
            page2 ^= 1;
        }
        uint32_t baseAttributeX = scrollX >> 5;
        uint32_t baseAttributeY = scrollY >> 5;
        uint32_t baseNameX = baseAttributeX << 2; // Must snap to attribute
        uint32_t baseNameY = scrollY >> 3;
        uint32_t fineX = scrollX - (baseNameX << 3);

        // Assign left and right regions
        static const uint32_t addrAttribute[] = { 0x23c0, 0x27c0, 0x2bc0, 0x2fc0 };
        static const uint32_t addrName[] = { 0x2000, 0x2400, 0x2800, 0x2c00 };
        uint32_t addrAttribute1 = addrAttribute[page1] + baseAttributeY * 8 + baseAttributeX;
        uint32_t addrAttribute2 = addrAttribute[page2] + baseAttributeY * 8;
        uint32_t addrAttribute3 = addrAttribute[page1 ^ 2] + baseAttributeY * 8 + baseAttributeX;
        uint32_t addrAttribute4 = addrAttribute[page2 ^ 2] + baseAttributeY * 8;
        uint32_t sizeAttribute1 = 8 - baseAttributeX;
        uint32_t sizeAttribute2 = baseAttributeX + 1;
        uint32_t addrName1 = addrName[page1] + baseNameY * 32 + baseNameX;
        uint32_t addrName2 = addrName[page2] + baseNameY * 32;
        uint32_t addrName3 = addrName[page1 ^ 2] + baseNameY * 32 + baseNameX;
        uint32_t addrName4 = addrName[page2 ^ 2] + baseNameY * 32;
        uint32_t sizeName1 = 32 - baseNameX;
        uint32_t sizeName2 = baseNameX + 4;

        paletteIndex = 0;
        attributeIndex = 0;
        nameIndex = 0;

        uint8_t palette[32];
        uint8_t attributes1[36];
        uint8_t attributes2[36];
        uint8_t names[36];
        uint8_t work[SCANLINE_PIXEL_CAPACITY];
        uint8_t* surface = mSurface + y0 * mPitch;
        uint8_t* attributes = attributes1;

        uint16_t patternTableBase = (mRegister[PPU_REG_PPUCTRL] & PPU_CONTROL_BACKGROUND_PATTERN_TABLE) ? 0x1000 : 0x0000;
        uint16_t patternTable = patternTableBase + (scrollY & 7);

        memset(palette, 0xff, sizeof(palette));
        memset(attributes1, 0xff, sizeof(attributes1));
        memset(attributes2, 0xff, sizeof(attributes2));
        memset(names, 0xff, sizeof(names));
        memset(work, 0xff, sizeof(work));

        fetchPalette(palette);
        fetchAttributes(attributes1 + 0, attributes2 + 0, addrAttribute1, sizeAttribute1);
        fetchAttributes(attributes1 + sizeName1, attributes2 + sizeName1, addrAttribute2, sizeAttribute2);
        fetchNames(names + 0, addrName1, sizeName1);
        fetchNames(names + sizeName1, addrName2, sizeName2);

        int32_t y = y0;
        uint32_t copySize = PPU_VISIBLE_COLUMNS - x0;
        if (y == y1)
            copySize = x1 - x0;
        while (y <= y1)
        {
            renderLine(work, names, attributes, palette, patternTable, 36);
            memcpy(surface + x0, work + fineX + x0, copySize);

            surface += mPitch;

            ++patternTable;
            ++y;
            x0 = 0;
            copySize = PPU_VISIBLE_COLUMNS;
            if (y == y1)
                copySize = x1;
            if (++scrollY == 240)
            {
                // Change in name table
                addrAttribute1 = addrAttribute3;
                addrAttribute2 = addrAttribute4;
                addrName1 = addrName3;
                addrName2 = addrName4;
            }
            else
            {
                if ((scrollY & 7) == 0)
                {
                    patternTable = patternTableBase;
                    addrName1 += 32;
                    addrName2 += 32;
                    fetchNames(names + 0, addrName1, sizeName1);
                    fetchNames(names + sizeName1, addrName2, sizeName2);
                }
                if ((scrollY & 15) == 0)
                {
                    attributes = attributes2;
                }
                if ((scrollY & 31) == 0)
                {
                    addrAttribute1 += 8;
                    addrAttribute2 += 8;
                    fetchAttributes(attributes1 + 0, attributes2 + 0, addrAttribute1, sizeAttribute1);
                    fetchAttributes(attributes1 + sizeName1, attributes2 + sizeName1, addrAttribute2, sizeAttribute2);
                    attributes = attributes1;
                }
            }
        }
    }

    void PPU::updateSpriteHitTestConditions()
    {
        mSprite0StartTick = 0;
        mSprite0EndTick = 0;
        uint32_t y0 = mOAM[0];
        if (y0 < 240)
        {
            uint8_t ppuctrl = mRegister[PPU_REG_PPUCTRL];
            uint32_t y1 = y0 + ((ppuctrl & PPU_CONTROL_SPRITE_SIZE) ? 15 : 7) + 1;
            uint32_t x0 = mOAM[3];
            uint32_t x1 = x0 + 8;
            if (x1 > 256)
                x1 = 256;
            mSprite0StartTick = y0 * mTicksPerLine + x0;
            mSprite0EndTick = y1 * mTicksPerLine + x1;
        }
    }

    void PPU::checkHitTest(int32_t tick)
    {
        if (!mCheckHitTest)
            return;

        //if (tick >= mSprite0StartTick)
        if (tick >= mSprite0EndTick) // Better for smb1
        {
            // TODO: Accurate hit test
            mCheckHitTest = false;
            if (mSprite0StartTick >= mTicksPerLine)
            {
                mRegister[PPU_REG_PPUSTATUS] |= PPU_STATUS_HIT_TEST;
                render(tick);
            }
        }
    }
}
