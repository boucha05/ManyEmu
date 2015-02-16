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

    static const int8_t kPulseDuty[4][8] =
    {
        { -1, +1, -1, -1, -1, -1, -1, -1 },
        { -1, +1, +1, -1, -1, -1, -1, -1 },
        { -1, +1, +1, +1, +1, -1, -1, -1 },
        { +1, -1, -1, +1, +1, +1, +1, +1 },
    };
}

namespace NES
{
    APU::APU()
    {
        initialize();
    }

    APU::~APU()
    {
        destroy();
    }

    void APU::initialize()
    {
        mClock = nullptr;
        mMemory = nullptr;
        mMasterClockDivider = 0;
        mFrameCountTicks = 0;
        memset(mRegister, 0, sizeof(mRegister));
        memset(mController, 0, sizeof(mController));
        memset(mShifter, 0, sizeof(mShifter));
        mSoundBuffer = nullptr;
        mSoundBufferSize = 0;
        mSampleBuffer.clear();
        mBufferTick = 0;
        mPulse[0].reset();
        mPulse[1].reset();
        mMode5Step = false;
        mIRQ = false;
    }

    bool APU::create(Clock& clock, MemoryBus& memory, uint32_t masterClockDivider, uint32_t masterClockFrequency)
    {
        mClock = &clock;
        mClock->addListener(*this);
        mMemory = &memory;
        mMasterClockDivider = masterClockDivider;
        mFrameCountTicks = masterClockDivider / 240;
        return true;
    }

    void APU::destroy()
    {
        if (mClock)
            mClock->removeListener(*this);
        initialize();
    }

    void APU::reset()
    {
        memset(mRegister, 0, sizeof(mRegister));
        mPulse[0].reset();
        mPulse[1].reset();
    }

    void APU::execute()
    {
    }

    void APU::advanceClock(int32_t ticks)
    {
        advanceBuffer(ticks);
        mBufferTick -= ticks;
    }

    void APU::setDesiredTicks(int32_t ticks)
    {
        advanceBuffer(ticks);
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
            NOT_IMPLEMENTED("Register read");
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
            case APU_REG_SQ1_VOL:
                advanceBuffer(ticks);
                mPulse[0].duty = (value >> 6) & 0x03;
                mPulse[0].loop = (value & 0x20) != 0;
                mPulse[0].constant = (value & 0x10) != 0;
                mPulse[0].volume = (value & 0x0f);
                break;

            case APU_REG_SQ1_SWEEP:
                advanceBuffer(ticks);
                mPulse[0].sweepEnable = (value & 0x80) != 0;
                mPulse[0].sweepPeriod = (value >> 4) & 0x07;
                mPulse[0].sweepNegate = (value & 0x08) != 0;
                mPulse[0].sweepShift = (value & 0x07);
                break;

            case APU_REG_SQ1_LO:
                advanceBuffer(ticks);
                mPulse[0].timer = (mPulse[0].timer & ~0xff) | value;
                break;

            case APU_REG_SQ1_HI:
                advanceBuffer(ticks);
                mPulse[0].length = (value >> 3) & 0x1f;
                mPulse[0].timer = (mPulse[0].timer & 0xff) | ((value & 0x03) << 8);
                break;

            case APU_REG_SQ2_VOL:
                advanceBuffer(ticks);
                mPulse[1].duty = (value >> 6) & 0x03;
                mPulse[1].loop = (value & 0x20) != 0;
                mPulse[1].constant = (value & 0x10) != 0;
                mPulse[1].volume = (value & 0x0f);
                break;

            case APU_REG_SQ2_SWEEP:
                advanceBuffer(ticks);
                mPulse[1].sweepEnable = (value & 0x80) != 0;
                mPulse[1].sweepPeriod = (value >> 4) & 0x07;
                mPulse[1].sweepNegate = (value & 0x08) != 0;
                mPulse[1].sweepShift = (value & 0x07);
                break;

            case APU_REG_SQ2_LO:
                advanceBuffer(ticks);
                mPulse[1].timer = (mPulse[1].timer & ~0xff) | value;
                break;

            case APU_REG_SQ2_HI:
                advanceBuffer(ticks);
                mPulse[0].length = (value >> 3) & 0x1f;
                mPulse[0].timer = (mPulse[0].timer & 0xff) | ((value & 0x03) << 8);
                break;

            case APU_REG_TRI_LINEAR: break;
            case APU_REG_TRI_LO: break;
            case APU_REG_TRI_HI: break;
            case APU_REG_NOISE_VOL: break;
            case APU_REG_NOISE_LO: break;
            case APU_REG_NOISE_HI: break;
            case APU_REG_DMC_FREQ: break;
            case APU_REG_DMC_RAW: break;
            case APU_REG_DMC_START: break;
            case APU_REG_DMC_LEN: break;
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
                break;
            }

            case APU_REG_SND_CHN:
                advanceBuffer(ticks);
                if ((value & 0x01) == 0)
                    mPulse[0].disable();
                if ((value & 0x02) == 0)
                    mPulse[1].disable();
                break;

            case APU_REG_JOY1:
                // Controller strobe
                if (value & 1)
                {
                    for (uint32_t controller = 0; controller < 4; ++controller)
                        mShifter[controller] = mController[controller];
                }
                break;

            case APU_REG_JOY2:
                advanceBuffer(ticks);
                if ((value & 0x40) != 0)
                    mMode5Step = true;
                if ((value & 0x80) == 0)
                    mIRQ = true;
                break;
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

    void APU::setSoundBuffer(int16_t* buffer, size_t size)
    {
        mSoundBuffer = buffer;
        mSoundBufferSize = size;
    }

    void APU::advanceBuffer(int32_t tick)
    {
        if (mBufferTick >= tick)
            return;
        if (mSoundBufferSize)
        {
            uint32_t delta = tick - mBufferTick;
            // TODO: Advance sound samples
        }
        mBufferTick = tick;
    }

    ///////////////////////////////////////////////////////////////////////////

    void APU::Pulse::reset()
    {
        duty = 0;
        volume = 0;
        sweepPeriod = 0;
        sweepShift = 0;
        timer = 0;
        length = 0;
        loop = false;
        constant = false;
        sweepEnable = false;
        sweepNegate = false;
    }

    void APU::Pulse::disable()
    {
        length = 0;
    }
}
