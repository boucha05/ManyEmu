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
    
    static const uint32_t VRAM_NAMETABLE_SIZE = 0x400;
    static const uint32_t VRAM_NAMETABLE_OFFSET0 = 0x000;
    static const uint32_t VRAM_NAMETABLE_OFFSET1 = 0x400;
    static const uint32_t VRAM_NAMETABLE_OFFSET2 = 0x800;
    static const uint32_t VRAM_NAMETABLE_OFFSET3 = 0xc00;

    static const uint32_t OAM_SIZE = 0x100;

    static const uint8_t PPU_CONTROL_VERTICAL_INCREMENT = 0x04;
    static const uint8_t PPU_CONTROL_VBLANK_OUTPUT = 0x80;

    static const uint8_t PPU_STATUS_VBLANK = 0x80;

    static const uint32_t PPU_TICKS_PER_LINE = 341;
    static const uint32_t PPU_VISIBLE_LINES = 240;
    static const uint32_t PPU_VISIBLE_COLUMNS = 256;
    static const uint32_t PPU_VISIBLE_START_LINE = 1;
    static const uint32_t PPU_VISIBLE_START_COLUMN = 4;
    static const uint32_t PPU_VBLANK_START_LINE = PPU_VISIBLE_START_LINE + PPU_VISIBLE_LINES;
    static const uint32_t PPU_VBLANK_END_LINE = 0;
    static const uint32_t PPU_VBLANK_START_TICKS = PPU_VBLANK_START_LINE * PPU_TICKS_PER_LINE;
    static const uint32_t PPU_VBLANK_END_TICKS = PPU_VBLANK_END_LINE * PPU_TICKS_PER_LINE;

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
        , mAddessLow(false)
        , mAddress(0)
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

    bool PPU::create(Clock& clock, uint32_t masterClockDivider, uint32_t createFlags)
    {
        mClock = &clock;
        mClock->addListener(*this);
        mMasterClockDivider = masterClockDivider;
        mVBlankStartTicks = PPU_VBLANK_START_TICKS * masterClockDivider;
        mVBlankEndTicks = PPU_VBLANK_END_TICKS * masterClockDivider;
        mTicksPerLine = PPU_TICKS_PER_LINE * masterClockDivider;

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
            uint16_t addrStart = 0x2000 + mirror * 0x1000;
            uint16_t addrEnd = addrStart + 0x03ff;
            for (uint16_t page = 0; page < 4; ++page)
            {
                mMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_READ, addrStart, addrEnd, mNameTableRead[page]);
                mMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_WRITE, addrStart, addrEnd, mNameTableWrite[page]);
                addrStart += 0x0400;
                addrEnd += 0x0400;
            }
        }

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
        memset(mRegister, 0, sizeof(mRegister));
        mRegister[PPU_REG_PPUSTATUS] |= PPU_STATUS_VBLANK;
        mClock->addEvent(onVBlankEnd, this, mVBlankEndTicks);
        mClock->addEvent(onVBlankStart, this, mVBlankStartTicks);
        mAddessLow = false;
        mAddress = 0;
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
        render(ticks);
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
            mAddessLow = false;
            endVBlank();
            return ppustatus;
        }
            //case PPU_REG_OAMADDR: break;
            //case PPU_REG_OAMDATA: break;
            //case PPU_REG_PPUSCROLL: break;
            //case PPU_REG_PPUADDR: break;
        case PPU_REG_PPUDATA:
        {
            uint8_t value = memory_bus_read8(mMemory.getState(), 0, mAddress);
            uint16_t increment = mRegister[PPU_REG_PPUCTRL] & PPU_CONTROL_VERTICAL_INCREMENT ? 32 : 1;
            mAddress = (mAddress + increment) & MEM_MASK;
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
        mRegister[addr] = value;
        switch (addr)
        {
        case PPU_REG_PPUCTRL:
        {
            uint8_t ppuctrl = mRegister[PPU_REG_PPUCTRL];
            mRegister[PPU_REG_PPUCTRL] = value;
            if (!(ppuctrl & PPU_CONTROL_VBLANK_OUTPUT) &&
                (value & PPU_CONTROL_VBLANK_OUTPUT) &&
                (mRegister[PPU_REG_PPUSTATUS] & PPU_STATUS_VBLANK))
            {
                signalVBlankStart();
            }
            break;
        }
        case PPU_REG_PPUMASK: break;
        //case PPU_REG_PPUSTATUS: break;
        case PPU_REG_OAMADDR: break;
        //case PPU_REG_OAMDATA: break;
        case PPU_REG_PPUSCROLL: break;

        case PPU_REG_PPUADDR:
        {
            if (mAddessLow)
                mAddress = ((mAddress & 0xff00) | (value | 0xff));
            else
                mAddress = ((mAddress & 0x00ff) | ((value << 8) | 0xff));
            mAddessLow = !mAddessLow;
            break;
        }

        case PPU_REG_PPUDATA:
        {
            memory_bus_write8(mMemory.getState(), 0, mAddress, value);
            uint16_t increment = mRegister[PPU_REG_PPUCTRL] & PPU_CONTROL_VERTICAL_INCREMENT ? 32 : 1;
            mAddress = (mAddress + increment) & MEM_MASK;
            break;
        }

        default:
            NOT_IMPLEMENTED();
        }
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
        mLastTickRendered = 0;
    }

    void PPU::signalVBlankStart()
    {
        for (auto listener : mListeners)
            listener->onVBlankStart();
        //printf("[VBLANK interrupt]\n");
    }

    void PPU::onVBlankStart()
    {
        startVBlank();
    }

    void PPU::onVBlankEnd()
    {
        endVBlank();
    }

    void PPU::onVBlankStart(void* context, int32_t ticks)
    {
        static_cast<PPU*>(context)->startVBlank();
    }

    void PPU::onVBlankEnd(void* context, int32_t ticks)
    {
        static_cast<PPU*>(context)->endVBlank();
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

    static int32_t pixelOffset = 0;

    void PPU::setRenderSurface(void* surface, size_t pitch)
    {
        mSurface = static_cast<uint8_t*>(surface);
        mPitch = pitch;
        ++pixelOffset;
    }

    void PPU::render(int32_t lastTick)
    {
        if (!mSurface)
            return;

        if (lastTick <= mLastTickRendered)
            return;

        int32_t firstTick = mLastTickRendered + 1;
        mLastTickRendered = firstTick;

        int32_t x0 = (firstTick % mTicksPerLine);
        int32_t y0 = (firstTick / mTicksPerLine);
        int32_t x1 = (lastTick % mTicksPerLine);
        int32_t y1 = (lastTick / mTicksPerLine);

        x0 = (x0 / mMasterClockDivider) - PPU_VISIBLE_START_COLUMN;
        x1 = (x1 / mMasterClockDivider) - PPU_VISIBLE_START_COLUMN;
        y0 = y0 - PPU_VISIBLE_START_LINE;
        y1 = y1 - PPU_VISIBLE_START_LINE;

        int32_t y = y0;
        int32_t x = x0;
        while ((x <= x1) || (y <= y1))
        {
            if ((x < PPU_VISIBLE_COLUMNS) && (y < PPU_VISIBLE_LINES))
            {
                mSurface[y * PPU_VISIBLE_COLUMNS + x] = ((x + pixelOffset) * y) & 0xff;
            }
            if (++x > PPU_TICKS_PER_LINE)
            {
                x = 0;
                ++y;
            }
        }
    }
}
