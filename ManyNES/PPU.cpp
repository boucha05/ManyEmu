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

    void NOT_IMPLEMENTED()
    {
        printf("Feature not implemented\n");
        assert(false);
    }
}

namespace NES
{
    PPU::PPU()
    {
    }

    PPU::~PPU()
    {
        destroy();
    }

    bool PPU::create()
    {
        return true;
    }

    void PPU::destroy()
    {
    }

    void PPU::reset()
    {
        mRegPPUCTRL = 0;
    }

    void PPU::update(void* surface, uint32_t pitch)
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
            return 0; // TODO: Implement status register and reset latches
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
