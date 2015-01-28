#include "PPU.h"
#include <assert.h>
#include <vector>

namespace
{
    static const uint32_t PPU_REG_PPUCTRL = 0x0;
    static const uint32_t PPU_REG_PPUMASK = 0x1;
    static const uint32_t PPU_REG_PPUSTATUS = 0x2;
    static const uint32_t PPU_REG_OAMADDR = 0x3;
    static const uint32_t PPU_REG_OAMDATA = 0x4;
    static const uint32_t PPU_REG_PPUSCROLL = 0x5;
    static const uint32_t PPU_REG_PPUADDR = 0x6;
    static const uint32_t PPU_REG_PPUDATA = 0x7;

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
        printf("Feature not implemented\n");
        //assert(false);
    }
}

namespace NES
{
    PPU::PPU()
        : mVBlankStartTicks(0)
        , mVBlankEndTicks(0)
        , mRegPPUCTRL(0)
        , mRegPPUSTATUS(0)
        , mVBlankStatusDirty(true)
    {
    }

    PPU::~PPU()
    {
        destroy();
    }

    bool PPU::create(uint32_t masterClockDivider)
    {
        mMasterClockDivider = masterClockDivider;
        mVBlankStartTicks = PPU_VBLANK_START_TICKS * masterClockDivider;
        mVBlankEndTicks = PPU_VBLANK_END_TICKS * masterClockDivider;
        return true;
    }

    void PPU::destroy()
    {
    }

    void PPU::reset()
    {
        mRegPPUCTRL = 0;
        mRegPPUSTATUS = 0;
        mVBlankStatusDirty = true;
    }

    void PPU::execute()
    {
    }

    void PPU::update(void* surface, uint32_t pitch)
    {
    }

    void PPU::advanceClock(int32_t ticks)
    {
        // New frame is being produced
        mVBlankStatusDirty = true;
    }

    void PPU::setDesiredTicks(int32_t ticks)
    {
    }

    uint8_t PPU::regRead(int32_t ticks, uint32_t addr)
    {
        addr = (addr & 7);
        switch (addr)
        {
            //case PPU_REG_PPUCTRL: break;
            //case PPU_REG_PPUMASK: break;

        case PPU_REG_PPUSTATUS:
        {
            bool vblank = (ticks <= mVBlankEndTicks) || (ticks >= mVBlankStartTicks);
            if (mVBlankStatusDirty)
            {
                mVBlankStatusDirty = false;
                mRegPPUSTATUS |= vblank ? PPU_STATUS_VBLANK : 0;
            }

            uint8_t ppustatus = mRegPPUSTATUS;

            // Clear after reading if outside the VBLANK
            if (!vblank)
            {
                mRegPPUSTATUS &= ~PPU_STATUS_VBLANK;
                mVBlankStatusDirty = true;
            }
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
        return 0;
    }

    void PPU::regWrite(int32_t ticks, uint32_t addr, uint8_t value)
    {
        addr = (addr & 7);
        switch (addr)
        {
        case PPU_REG_PPUCTRL:
            // TODO: Toggle rendering if sensitive state change are detected
            mRegPPUCTRL = value;
            break;
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
        printf("[VBLANK]\n");
    }
}
