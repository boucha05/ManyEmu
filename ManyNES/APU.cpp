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

    static const uint32_t kLength[] =
    {
        0x0a, 0xfe, 0x14, 0x02, 0x28, 0x04, 0x50, 0x06, 0xa0, 0x08, 0x3c, 0x0a, 0x0e, 0x0c, 0x1a, 0x0e,
        0x0c, 0x10, 0x18, 0x12, 0x30, 0x14, 0x60, 0x16, 0xc0, 0x18, 0x48, 0x1a, 0x10, 0x1c, 0x20, 0x1e,
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
        mMasterClockFrequency = 0;
        mFrameCountTicks = 0;
        memset(mRegister, 0, sizeof(mRegister));
        memset(mController, 0, sizeof(mController));
        memset(mShifter, 0, sizeof(mShifter));
        mSoundBuffer = nullptr;
        mSoundBufferSize = 0;
        mSoundBufferOffset = 0;
        mSampleSpeed = 0;
        mSampleBuffer.clear();
        mBufferTick = 0;
        mSampleTick = 0;
        mSequenceTick = 0;
        mSequenceCount = 0;
        mPulse[0].reset(mMasterClockDivider * 2);
        mPulse[1].reset(mMasterClockDivider * 2);
        mTriangle.reset(mMasterClockDivider);
        mMode5Step = false;
        mIRQ = false;
    }

    bool APU::create(Clock& clock, MemoryBus& memory, uint32_t masterClockDivider, uint32_t masterClockFrequency)
    {
        mClock = &clock;
        mClock->addListener(*this);
        mMemory = &memory;
        mMasterClockDivider = masterClockDivider;
        mMasterClockFrequency = masterClockFrequency;
        mFrameCountTicks = masterClockFrequency / 240;
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
        mSoundBufferOffset = 0;
        mBufferTick = 0;
        mSequenceTick = 0;
        mSequenceCount = 0;
        mPulse[0].reset(mMasterClockDivider * 2);
        mPulse[1].reset(mMasterClockDivider * 2);
        mTriangle.reset(mMasterClockDivider);
        mClock->addEvent(onSequenceEvent, this, mSequenceTick);
    }

    void APU::execute()
    {
    }

    void APU::advanceClock(int32_t ticks)
    {
        advanceBuffer(ticks);
        mBufferTick -= ticks;
        mSequenceTick -= ticks;
        mSampleTick = 0;
        assert(mSoundBufferOffset == mSoundBufferSize - 1);
        mSoundBufferOffset = 0;
        assert(mBufferTick >= 0);
        assert(mSequenceTick >= 0);
        assert(mSampleTick >= 0);
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
        advanceBuffer(ticks);
        switch (addr)
        {
            case APU_REG_SQ1_VOL:
                mPulse[0].write(0, value);
                break;

            case APU_REG_SQ1_SWEEP:
                mPulse[0].write(1, value);
                break;

            case APU_REG_SQ1_LO:
                mPulse[0].write(2, value);
                break;

            case APU_REG_SQ1_HI:
                mPulse[0].write(3, value);
                break;

            case APU_REG_SQ2_VOL:
                mPulse[1].write(0, value);
                break;

            case APU_REG_SQ2_SWEEP:
                mPulse[1].write(1, value);
                break;

            case APU_REG_SQ2_LO:
                mPulse[1].write(2, value);
                break;

            case APU_REG_SQ2_HI:
                mPulse[1].write(3, value);
                break;

            case APU_REG_TRI_LINEAR:
                mTriangle.write(0, value);
                break;

            case APU_REG_TRI_LO:
                mTriangle.write(2, value);
                break;

            case APU_REG_TRI_HI:
                mTriangle.write(3, value);
                break;

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
                mPulse[0].enable((value & 0x01) != 0);
                mPulse[1].enable((value & 0x02) != 0);
                mTriangle.enable((value & 0x04) != 0);
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
                if ((value & 0x80) != 0)
                {
                    mSequenceCount = 0;
                    mClock->addEvent(onSequenceEvent, this, ticks);
                }
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
        mSoundBufferOffset = 0;
        mSampleSpeed = mMasterClockFrequency / (60 * size);
    }

    void APU::updateEnvelopesAndLinearCounter()
    {
        mPulse[0].updateEnvelope();
        mPulse[1].updateEnvelope();
        mTriangle.updateLinearCounter();
    }

    void APU::updateLengthCountersAndSweepUnits()
    {
        mPulse[0].updateLengthCounter();
        mPulse[0].updateSweep();
        mPulse[1].updateLengthCounter();
        mPulse[1].updateSweep();
        mTriangle.updateLengthCounter();
    }

    void APU::advanceSamples(int32_t tick)
    {
        if (tick <= mBufferTick)
            return;

        while (mSampleTick <= tick)
        {
            uint32_t updateTicks = mSampleTick - mBufferTick;
            mPulse[0].update(updateTicks);
            mPulse[1].update(updateTicks);
            mTriangle.update(updateTicks);
            if (mSoundBuffer)
            {
                union
                {
                    int8_t  channel[2];
                    int16_t merged;
                } value;

                value.channel[0] = mPulse[0].level + mPulse[1].level;
                value.channel[1] = mTriangle.level;

                mSoundBuffer[mSoundBufferOffset++] = value.merged;
            }
            mBufferTick = mSampleTick;
            mSampleTick += mSampleSpeed;
        }

        mBufferTick = tick;
    }

    void APU::advanceBuffer(int32_t tick)
    {
        if (mBufferTick >= tick)
            return;

        while (mSequenceTick <= tick)
        {
            advanceSamples(mSequenceTick);
            mBufferTick = mSequenceTick;
            mSequenceTick += mFrameCountTicks;
            if (mMode5Step)
            {
                switch (mSequenceCount++)
                {
                case 0:
                    updateEnvelopesAndLinearCounter();
                    updateLengthCountersAndSweepUnits();
                    break;

                case 1:
                    updateEnvelopesAndLinearCounter();
                    break;

                case 2:
                    updateEnvelopesAndLinearCounter();
                    updateLengthCountersAndSweepUnits();
                    break;

                case 3:
                    updateEnvelopesAndLinearCounter();
                    break;

                case 4:
                    mSequenceCount = 0;
                    break;

                default:
                    assert(false);
                }
            }
            else
            {
                switch (mSequenceCount++)
                {
                case 0:
                    updateEnvelopesAndLinearCounter();
                    break;

                case 1:
                    updateEnvelopesAndLinearCounter();
                    updateLengthCountersAndSweepUnits();
                    break;

                case 2:
                    updateEnvelopesAndLinearCounter();
                    break;

                case 3:
                    updateEnvelopesAndLinearCounter();
                    updateLengthCountersAndSweepUnits();
                    mSequenceCount = 0;
                    // TODO: Generate IRQ if bit 6 of $4017 is clear
                    break;

                default:
                    assert(false);
                }
            }
            mClock->addEvent(onSequenceEvent, this, mSequenceTick);
        }
        advanceSamples(tick);
    }

    void APU::onSequenceEvent(int32_t tick)
    {
        if (tick >= mSequenceTick)
            advanceBuffer(tick);
    }

    void APU::onSequenceEvent(void* context, int32_t tick)
    {
        static_cast<APU*>(context)->onSequenceEvent(tick);
    }

    ///////////////////////////////////////////////////////////////////////////

    void APU::Pulse::reset(uint32_t _masterClockDivider)
    {
        masterClockDivider = _masterClockDivider;

        duty = 0;
        loop = false;
        constant = false;
        envelope = 0;
        sweepEnable = false;
        sweepPeriod = 0;
        sweepNegate = false;
        sweepShift = 0;
        timer = 0;
        length = 0;

        halt = true;
        period = 0;
        timerCount = 0;
        cycle = 0;
        level = 0;

        divider = 0;
        counter = 0;
        volume = 0;
    }

    void APU::Pulse::enable(bool enabled)
    {
        if (!enabled)
        {
            level = 0;
            length = 0;
        }
        halt = !enabled;
    }

    void APU::Pulse::reload()
    {
        if (timer < 8)
        {
            enable(false);
        }
        else
        {
            timerCount = period;
            cycle = 0;
            counter = 15;
            divider = envelope;
        }
    }

    void APU::Pulse::update(uint32_t ticks)
    {
        if (!period)
            return;

        // Update timer
        while (ticks > timerCount)
        {
            ticks -= timerCount;
            timerCount = period;
            if (length)
                cycle = (cycle + 1) & 7;
        }
        timerCount -= ticks;

        // Update level
        static const uint8_t kLevel[4][8] =
        {
            { 0, 1, 0, 0, 0, 0, 0, 0 },
            { 0, 1, 1, 0, 0, 0, 0, 0 },
            { 0, 1, 1, 1, 1, 0, 0, 0 },
            { 1, 0, 0, 1, 1, 1, 1, 1 },
        };
        static const int8_t kVolume[16][2] =
        {
            {  -0,  +0 }, {  -1,  +1 }, {  -2,  +2 }, {  -3,  +3 },
            {  -4,  +4 }, {  -5,  +5 }, {  -6,  +6 }, {  -7,  +7 },
            {  -8,  +8 }, {  -9,  +9 }, { -10, +10 }, { -11, +11 },
            { -12, +12 }, { -13, +13 }, { -14, +14 }, { -15, +15 },
        };
        uint32_t value =  kLevel[duty][cycle];
        level = halt ? 0 : kVolume[volume][value];
    }

    void APU::Pulse::updateEnvelope()
    {
        if (divider-- == 0)
        {
            divider = 15;
            if (counter)
                --counter;
            else if (loop)
                counter = 15;
            if (!constant)
                volume = counter;
        }
    }

    void APU::Pulse::updateLengthCounter()
    {
        if (!loop && length)
        {
            if (--length == 0)
                volume = 0;
        }
    }

    void APU::Pulse::updateSweep()
    {
        // TODO: Update sweep
    }

    void APU::Pulse::write(uint32_t index, uint32_t value)
    {
        switch (index)
        {
        case 0:
            duty = (value >> 6) & 0x03;
            loop = (value & 0x20) != 0;
            constant = (value & 0x10) != 0;
            envelope = (value & 0x0f);
            break;

        case 1:
            sweepEnable = (value & 0x80) != 0;
            sweepPeriod = (value >> 4) & 0x07;
            sweepNegate = (value & 0x08) != 0;
            sweepShift = (value & 0x07);
            break;

        case 2:
            timer = (timer & ~0xff) | (value & 0xff);
            period = (timer + 1) * masterClockDivider;
            break;

        case 3:
            timer = (timer & 0xff) | ((value & 0x07) << 8);
            length = kLength[(value >> 3) & 0x1f];
            period = (timer + 1) * masterClockDivider;
            reload();
            break;

        default:
            assert(false);
        }
    }

    ///////////////////////////////////////////////////////////////////////////

    void APU::Triangle::reset(uint32_t _masterClockDivider)
    {
        masterClockDivider = _masterClockDivider;

        control = false;
        linear = 0;
        timer = 0;
        length = 0;
        reload = true;

        halt = true;
        period = 0;
        timerCount = 0;
        linearCount = 0;
        sequence = 0;
        level = 0;
    }

    void APU::Triangle::enable(bool enabled)
    {
        if (!enabled)
        {
            level = 0;
            length = 0;
        }
        halt = !enabled;
    }

    void APU::Triangle::update(uint32_t ticks)
    {
        if (!period)
            return;

        // Update timer
        while (ticks > timerCount)
        {
            ticks -= timerCount;
            timerCount = period;
            if (linearCount && length)
                sequence = (sequence + 1) & 31;
        }
        timerCount -= ticks;

        // Update level
        static const uint32_t kLevel[] =
        {
            -8, -7, -6, -5, -4, -3, -2, -1, +0, +1, +2, +3, +4, +5, +6, +7,
            +7, +6, +5, +4, +3, +2, +1, +0, -1, -2, -3, -4, -5, -6, -7, -8,
        };
        uint32_t value = kLevel[sequence];
        level = halt ? 0 : value;
    }

    void APU::Triangle::updateLengthCounter()
    {
        if (length && !halt)
            --length;
    }

    void APU::Triangle::updateLinearCounter()
    {
        if (reload)
            linearCount = linear;
        else if (linearCount)
            --linearCount;
        if (!control)
            reload = false;
    }

    void APU::Triangle::write(uint32_t index, uint32_t value)
    {
        switch (index)
        {
        case 0:
            control = (value & 0x80) != 0;
            linear = (value & 0x7f);
            break;

        case 1:
            assert(false);
            break;

        case 2:
            timer = (timer & ~0xff) | (value & 0xff);
            period = (timer + 1) * masterClockDivider;
            break;

        case 3:
            timer = (timer & 0xff) | ((value & 0x07) << 8);
            length = kLength[(value >> 3) & 0x1f];
            period = (timer + 1) * masterClockDivider;
            reload = true;
            break;

        default:
            assert(false);
        }
    }
}
