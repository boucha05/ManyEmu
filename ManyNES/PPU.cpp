#include "PPU.h"
#include <assert.h>
#include <vector>

namespace
{
    static const uint8_t PPU_CONTROL_VBLANK_OUTPUT = 0x80;

    static const uint8_t PPU_STATUS_VBLANK = 0x80;

    static const uint32_t PPU_TICKS_PER_LINE = 341;
    static const uint32_t PPU_VISIBLE_LINES = 240;
    static const uint32_t PPU_VISIBLE_START_LINE = 1;
    static const uint32_t PPU_VBLANK_START_LINE = PPU_VISIBLE_START_LINE + PPU_VISIBLE_LINES;
    static const uint32_t PPU_VBLANK_END_LINE = 0;
    static const uint32_t PPU_VBLANK_START_TICKS = PPU_VBLANK_START_LINE * PPU_TICKS_PER_LINE;
    static const uint32_t PPU_VBLANK_END_TICKS = PPU_VBLANK_END_LINE * PPU_TICKS_PER_LINE;

    void NOT_IMPLEMENTED()
    {
        //printf("Feature not implemented\n");
        //assert(false);
    }
}

namespace NES
{
    PPU::PPU()
        : mClock(nullptr)
        , mVBlankStartTicks(0)
        , mVBlankEndTicks(0)
    {
        memset(mRegister, 0, sizeof(mRegister));
    }

    PPU::~PPU()
    {
        destroy();
    }

    bool PPU::create(Clock& clock, uint32_t masterClockDivider)
    {
        mClock = &clock;
        mClock->addListener(*this);
        mMasterClockDivider = masterClockDivider;
        mVBlankStartTicks = PPU_VBLANK_START_TICKS * masterClockDivider;
        mVBlankEndTicks = PPU_VBLANK_END_TICKS * masterClockDivider;
        return true;
    }

    void PPU::destroy()
    {
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
    }

    void PPU::execute()
    {
    }

    void PPU::update(void* surface, uint32_t pitch)
    {
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
            endVBlank();
            return ppustatus;
        }
            //case PPU_REG_OAMADDR: break;
            //case PPU_REG_OAMDATA: break;
            //case PPU_REG_PPUSCROLL: break;
            //case PPU_REG_PPUADDR: break;
            //case PPU_REG_PPUDATA: break;
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
            //case PPU_REG_PPUMASK: break;
            //case PPU_REG_PPUSTATUS: break;
            //case PPU_REG_OAMADDR: break;
            //case PPU_REG_OAMDATA: break;
            //case PPU_REG_PPUSCROLL: break;
            //case PPU_REG_PPUADDR: break;
            //case PPU_REG_PPUDATA: break;
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
}
