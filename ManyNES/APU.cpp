#include "APU.h"
#include <assert.h>
#include <vector>

namespace
{
    void NOT_IMPLEMENTED(const char* feature)
    {
        printf("APU: feature not implemented: %s\n", feature);
        //assert(false);
    }
}

namespace NES
{
    APU::APU()
    {
        memset(mRegister, 0, sizeof(mRegister));
    }

    APU::~APU()
    {
        destroy();
    }

    bool APU::create()
    {
        return true;
    }

    void APU::destroy()
    {
    }

    void APU::reset()
    {
        memset(mRegister, 0, sizeof(mRegister));
    }

    void APU::execute()
    {
    }

    void APU::advanceClock(int32_t ticks)
    {
    }

    void APU::setDesiredTicks(int32_t ticks)
    {
    }

    uint8_t APU::regRead(int32_t ticks, uint32_t addr)
    {
        addr = (addr & (APU_REGISTER_COUNT - 1));
        switch (addr)
        {
        //case APU_REG_SQ1_VOL: break;
        //case APU_REG_SQ1_SWEEP: break;
        //case APU_REG_SQ1_LO: break;
        //case APU_REG_SQ1_HI: break;
        //case APU_REG_SQ2_VOL: break;
        //case APU_REG_SQ2_SWEEP: break;
        //case APU_REG_SQ2_LO: break;
        //case APU_REG_SQ2_HI: break;
        //case APU_REG_TRI_LINEAR: break;
        //case APU_REG_TRI_LO: break;
        //case APU_REG_TRI_HI: break;
        //case APU_REG_NOISE_VOL: break;
        //case APU_REG_NOISE_LO: break;
        //case APU_REG_NOISE_HI: break;
        //case APU_REG_DMC_FREQ: break;
        //case APU_REG_DMC_RAW: break;
        //case APU_REG_DMC_START: break;
        //case APU_REG_DMC_LEN: break;
        //case APU_REG_OAM_DMA: break;
        //case APU_REG_SND_CHN: break;
        //case APU_REG_JOY1: break;
        //case APU_REG_JOY2: break;
        default:
            NOT_IMPLEMENTED("Register write");
        }
        uint8_t value = mRegister[addr];
        return value;
    }

    void APU::regWrite(int32_t ticks, uint32_t addr, uint8_t value)
    {
        addr = (addr & (APU_REGISTER_COUNT - 1));
        mRegister[addr] = value;
        switch (addr)
        {
            //case APU_REG_SQ1_VOL: break;
            //case APU_REG_SQ1_SWEEP: break;
            //case APU_REG_SQ1_LO: break;
            //case APU_REG_SQ1_HI: break;
            //case APU_REG_SQ2_VOL: break;
            //case APU_REG_SQ2_SWEEP: break;
            //case APU_REG_SQ2_LO: break;
            //case APU_REG_SQ2_HI: break;
            //case APU_REG_TRI_LINEAR: break;
            //case APU_REG_TRI_LO: break;
            //case APU_REG_TRI_HI: break;
            //case APU_REG_NOISE_VOL: break;
            //case APU_REG_NOISE_LO: break;
            //case APU_REG_NOISE_HI: break;
            //case APU_REG_DMC_FREQ: break;
            //case APU_REG_DMC_RAW: break;
            //case APU_REG_DMC_START: break;
            //case APU_REG_DMC_LEN: break;
            //case APU_REG_OAM_DMA: break;
            //case APU_REG_SND_CHN: break;
            //case APU_REG_JOY1: break;
            //case APU_REG_JOY2: break;
        default:
            NOT_IMPLEMENTED("Register write");
        }
    }
}
