#include "APU.h"
#include "MemoryBus.h"
#include <assert.h>
#include <vector>

namespace
{
    void NOT_IMPLEMENTED(const char* feature)
    {
        //printf("APU: feature not implemented: %s\n", feature);
        //assert(false);
    }
}

namespace NES
{
    APU::APU()
        : mMemory(nullptr)
    {
        memset(mRegister, 0, sizeof(mRegister));
        memset(mController, 0, sizeof(mController));
        memset(mShifter, 0, sizeof(mShifter));
    }

    APU::~APU()
    {
        destroy();
    }

    bool APU::create(MemoryBus& memory)
    {
        mMemory = &memory;
        return true;
    }

    void APU::destroy()
    {
        mMemory = nullptr;
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
        case APU_REG_JOY1:
        {
            mRegister[APU_REG_JOY1] = 0x40 | (mShifter[0] & 1) | ((mShifter[2] & 1) << 1);
            mShifter[0] = (mShifter[0] >> 1) | 0x80;
            mShifter[2] = (mShifter[2] >> 1) | 0x80;
            break;
        }
        case APU_REG_JOY2:
        {
            mRegister[APU_REG_JOY2] = 0x40 | (mShifter[1] & 1) | ((mShifter[3] & 1) << 1);
            mShifter[1] = (mShifter[1] >> 1) | 0x80;
            mShifter[3] = (mShifter[3] >> 1) | 0x80;
            break;
        }
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
            case APU_REG_DMC_RAW: break;
            //case APU_REG_DMC_START: break;
            //case APU_REG_DMC_LEN: break;
            case APU_REG_OAM_DMA:
            {
                MEMORY_BUS& memory = mMemory->getState();
                uint16_t base = value << 8;
                for (uint16_t offset = 0; offset < 256; ++offset)
                {
                    uint16_t addr = base + offset;
                    uint8_t value = memory_bus_read8(memory, ticks, addr);
                    memory_bus_write8(memory, ticks, 0x2004, value);
                }
            }
            case APU_REG_SND_CHN: break;
            case APU_REG_JOY1:
                // Controller strobe
                if (value & 1)
                {
                    for (uint32_t controller = 0; controller < 4; ++controller)
                        mShifter[controller] = mController[controller];
                }
                break;
            //case APU_REG_JOY2: break;
        default:
            NOT_IMPLEMENTED("Register write");
        }
    }

    void APU::setController(uint32_t index, uint8_t buttons)
    {
        if (index > 4)
            return;
        mController[index] = buttons;
    }
}
