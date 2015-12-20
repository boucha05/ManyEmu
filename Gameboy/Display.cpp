#include <Core/RegisterBank.h>
#include <Core/Serialization.h>
#include "Display.h"
#include "Interrupts.h"

namespace
{
    static const uint32_t VRAM_BANK_SIZE = 0x2000;
    static const uint32_t VRAM_BANK_START = 0x8000;
    static const uint32_t VRAM_BANK_END = VRAM_BANK_START + VRAM_BANK_SIZE - 1;
    static const uint32_t VRAM_SIZE_GB = 0x2000;
    static const uint32_t VRAM_SIZE_GBC = 0x4000;

    static const uint32_t OAM_SIZE = 0xa0;

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
        , useFastDma(true)
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
        mTicksPerLine           = 0;
        mUpdateRasterPosFast    = 0;
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
        resetClock();
    }

    bool Display::create(Config& config, emu::Clock& clock, uint32_t master_clock_divider, emu::MemoryBus& memory, Interrupts& interrupts, emu::RegisterBank& registers)
    {
        mTicksPerLine = TICKS_PER_LINE * master_clock_divider;
        mUpdateRasterPosFast = UPDATE_RASTER_POS_FAST * master_clock_divider;

        mClock = &clock;
        mMemory = &memory.getState();
        mConfig = config;

        EMU_VERIFY(mClockListener.create(clock, *this));

        bool isGBC = mConfig.model >= gb::Model::GBC;
        mVRAM.resize(isGBC ? VRAM_SIZE_GBC : VRAM_SIZE_GB, 0);
        EMU_VERIFY(memory.addMemoryRange(VRAM_BANK_START, VRAM_BANK_END, mMemoryVRAM.setReadWriteMemory(mVRAM.data() + mBankVRAM * VRAM_BANK_SIZE)));

        mOAM.resize(OAM_SIZE, 0);
        EMU_VERIFY(memory.addMemoryRange(0xfe00, 0xfe9f, mMemoryOAM.setReadWriteMemory(mOAM.data())));
        
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
        mRenderedTick = 0;
        mDesiredTick = 0;
        mLineFirstTick = 0;
        mLineTick = 0;
    }

    void Display::advanceClock(int32_t tick)
    {
        mSimulatedTick -= tick;
        mRenderedTick -= tick;
        mDesiredTick -= tick;
    }

    void Display::setDesiredTicks(int32_t tick)
    {
        mDesiredTick = tick;
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
        upateRasterPos(tick);
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
        if (!mConfig.useFastDma)
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

    void Display::reset()
    {
        mRegLCDC = 0x00;
        mRegSTAT = 0x00;
        mRegSCY  = 0x00;
        mRegSCX  = 0x00;
        mRegLY   = 0x00;
        mRegLYC  = 0x00;
        mRegDMA  = 0x00;
        mRegBGP  = 0x00;
        mRegOBP0 = 0x00;
        mRegOBP1 = 0x00;
        mRegWY   = 0x00;
        mRegWX   = 0x00;
    }

    void Display::serialize(emu::ISerializer& serializer)
    {
        serializer.serialize(mVRAM);
        serializer.serialize(mOAM);
        serializer.serialize(mSimulatedTick);
        serializer.serialize(mRenderedTick);
        serializer.serialize(mDesiredTick);
        serializer.serialize(mTicksPerLine);
        serializer.serialize(mUpdateRasterPosFast);
        serializer.serialize(mLineFirstTick);
        serializer.serialize(mLineTick);
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
    }

    void Display::upateRasterPos(int32_t tick)
    {
        if (tick - mLineFirstTick <= mUpdateRasterPosFast)
        {
            while (tick - mLineFirstTick >= mTicksPerLine)
            {
                ++mRegLY;
                mLineFirstTick += mTicksPerLine;
            }
            mLineTick = tick - mLineFirstTick;
        }
        else
        {
            mRegLY = tick / mTicksPerLine;
            mLineTick = tick % mTicksPerLine;
        }
    }

    void Display::render(int32_t tick)
    {
        if (mRenderedTick < tick)
        {
            mRenderedTick = tick;
        }
    }
}
