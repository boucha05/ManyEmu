#include <Core/MemoryBus.h>
#include <Core/Serialization.h>
#include "APU.h"
#include "nes.h"
#include <vector>

namespace
{
    void NOT_IMPLEMENTED(const char* feature)
    {
        printf("APU: feature not implemented: %s\n", feature);
        EMU_ASSERT(false);
    }

    void dummyTimerCallback(void* context, int32_t ticks)
    {
    }

    static const uint32_t kLength[] =
    {
        0x0a, 0xfe, 0x14, 0x02, 0x28, 0x04, 0x50, 0x06, 0xa0, 0x08, 0x3c, 0x0a, 0x0e, 0x0c, 0x1a, 0x0e,
        0x0c, 0x10, 0x18, 0x12, 0x30, 0x14, 0x60, 0x16, 0xc0, 0x18, 0x48, 0x1a, 0x10, 0x1c, 0x20, 0x1e,
    };
}

namespace nes
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
        mBufferTick = 0;
        mSampleTick = 0;
        mSequenceTick = 0;
        mSequenceCount = 0;
        mPulse[0].reset(mMasterClockDivider * 2, 0);
        mPulse[1].reset(mMasterClockDivider * 2, 1);
        mTriangle.reset(mMasterClockDivider);
        mNoise.reset(mMasterClockDivider);
        mMode5Step = false;
        mIRQ = false;
    }

    bool APU::create(emu::Clock& clock, emu::MemoryBus& memory, uint32_t masterClockDivider, uint32_t masterClockFrequency)
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
        mPulse[0].reset(mMasterClockDivider * 2, 0);
        mPulse[1].reset(mMasterClockDivider * 2, 1);
        mTriangle.reset(mMasterClockDivider);
        mNoise.reset(mMasterClockDivider);
        mDMC.reset(*mClock, *mMemory, mMasterClockDivider);
    }

    void APU::beginFrame()
    {
        mClock->addEvent(onSequenceEvent, this, mSequenceTick);
        mDMC.beginFrame();
    }

    void APU::execute()
    {
    }

    void APU::advanceClock(int32_t ticks)
    {
        advanceBuffer(ticks);
        mSequenceTick -= ticks;
        mBufferTick = 0;
        mSampleTick = 0;
        if (mSoundBuffer)
        {
            EMU_ASSERT(mSoundBufferOffset == mSoundBufferSize - 1);
            while (mSoundBufferOffset < mSoundBufferSize)
            {
                mSoundBuffer[mSoundBufferOffset] = mSoundBuffer[mSoundBufferOffset - 1];
                ++mSoundBufferOffset;
            }
            mSoundBufferOffset -= mSoundBufferSize;
        }
        EMU_ASSERT(mBufferTick >= 0);
        EMU_ASSERT(mSequenceTick >= 0);
        EMU_ASSERT(mSampleTick >= 0);
        mDMC.advanceClock(ticks);
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

            case APU_REG_NOISE_VOL:
                mNoise.write(0, value);
                break;

            case APU_REG_NOISE_LO:
                mNoise.write(2, value);
                break;

            case APU_REG_NOISE_HI:
                mNoise.write(3, value);
                break;

            case APU_REG_DMC_FREQ:
                mDMC.write(0, value);
                break;

            case APU_REG_DMC_RAW:
                mDMC.write(1, value);
                break;

            case APU_REG_DMC_START:
                mDMC.write(2, value);
                break;

            case APU_REG_DMC_LEN:
                mDMC.write(3, value);
                break;

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
                mNoise.enable((value & 0x08) != 0);
                mDMC.enable(ticks, (value & 0x10) != 0);
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

        // Filter out invalid button combinations
        static const uint32_t leftRight = nes::Context::ButtonLeft | nes::Context::ButtonRight;
        static const uint32_t upDown = nes::Context::ButtonUp | nes::Context::ButtonDown;
        if ((buttons & leftRight) == leftRight)
            buttons &= ~leftRight;
        if ((buttons & upDown) == upDown)
            buttons &= ~upDown;

        mController[index] = buttons;
    }

    void APU::setSoundBuffer(int16_t* buffer, size_t size)
    {
        mSoundBuffer = buffer;
        mSoundBufferSize = static_cast<uint32_t>(size);
        mSoundBufferOffset = 0;
        mSampleSpeed = mMasterClockFrequency / (60 * static_cast<uint32_t>(size));
    }

    void APU::updateEnvelopesAndLinearCounter()
    {
        mPulse[0].updateEnvelope();
        mPulse[1].updateEnvelope();
        mTriangle.updateLinearCounter();
        mNoise.updateEnvelope();
    }

    void APU::updateLengthCountersAndSweepUnits()
    {
        mPulse[0].updateSweep();
        mPulse[0].updateLengthCounter();
        mPulse[1].updateSweep();
        mPulse[1].updateLengthCounter();
        mTriangle.updateLengthCounter();
        mNoise.updateLengthCounter();
    }

    void APU::advanceSamples(int32_t tick)
    {
        if (tick <= mBufferTick)
            return;

        while (mSampleTick <= tick)
        {
            int32_t updateTicks = mSampleTick - mBufferTick;
            mPulse[0].update(updateTicks);
            mPulse[1].update(updateTicks);
            mTriangle.update(updateTicks);
            mNoise.update(updateTicks);
            mDMC.update(updateTicks);
            if (mSoundBuffer && (mSoundBufferOffset < mSoundBufferSize))
            {
                float pulseOut = 0.00752f * (mPulse[0].level + mPulse[1].level);
                float tndOut = 0.00851f * mTriangle.level + 0.00494f * mNoise.level + 0.00355f * mDMC.level;
                float soundOut = pulseOut + tndOut;
                uint16_t value = static_cast<int16_t>(soundOut * 32767.0f);

                mSoundBuffer[mSoundBufferOffset++] = value;
            }
            mBufferTick = mSampleTick;
            mSampleTick += mSampleSpeed;
        }

        mBufferTick = tick;
    }

    void APU::advanceBuffer(int32_t tick)
    {
        if (tick <= mBufferTick)
            return;

        while (mSequenceTick <= tick)
        {
            advanceSamples(mSequenceTick);
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
                    EMU_ASSERT(false);
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
                    EMU_ASSERT(false);
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

    void APU::serialize(emu::ISerializer& serializer)
    {
        uint32_t version = 2;
        serializer.serialize(version);
        serializer.serialize(mRegister, APU_REGISTER_COUNT);
        serializer.serialize(mController, EMU_ARRAY_SIZE(mController));
        serializer.serialize(mShifter, EMU_ARRAY_SIZE(mShifter));
        serializer.serialize(mBufferTick);
        serializer.serialize(mSampleTick);
        serializer.serialize(mSequenceTick);
        serializer.serialize(mSequenceCount);
        mPulse[0].serialize(serializer);
        mPulse[1].serialize(serializer);
        mTriangle.serialize(serializer);
        mNoise.serialize(serializer);
        if (version >= 2)
            mDMC.serialize(serializer);
        serializer.serialize(mMode5Step);
        serializer.serialize(mIRQ);
    }

    ///////////////////////////////////////////////////////////////////////////

    void APU::Pulse::reset(uint32_t _masterClockDivider, uint32_t _sweepCarry)
    {
        masterClockDivider = _masterClockDivider;
        sweepCarry = _sweepCarry;

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
        enabled = false;

        period = 0;
        timerCount = 0;
        cycle = 0;
        level = 0;

        envelopeDivider = 0;
        envelopeCounter = 0;
        envelopeVolume = 0;

        sweepReload = false;
        sweepDivider = 0;
    }

    void APU::Pulse::enable(bool _enabled)
    {
        enabled = _enabled;
        if (!enabled)
        {
            level = 0;
            length = 0;
        }
    }

    void APU::Pulse::reload()
    {
        timerCount = period;
        cycle = 0;
        envelopeCounter = 15;
        envelopeDivider = envelope;
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
        level = (!length || (timer < 8)) ? 0 : kVolume[envelopeVolume][value];
    }

    void APU::Pulse::updatePeriod()
    {
        if (timer >= 8)
            period = (timer + 1) * masterClockDivider;
    }

    void APU::Pulse::updateEnvelope()
    {
        if (envelopeDivider-- == 0)
        {
            envelopeDivider = envelope;
            if (envelopeCounter)
                --envelopeCounter;
            else if (loop)
                envelopeCounter = 15;
        }
        envelopeVolume = constant ? envelope : envelopeCounter;
    }

    void APU::Pulse::updateLengthCounter()
    {
        if (!loop && length)
            --length;
    }

    void APU::Pulse::updateSweep()
    {
        bool adjustPeriod = false;
        if (sweepReload)
        {
            adjustPeriod = !sweepDivider && sweepEnable;
            sweepDivider = sweepPeriod + 1;
            sweepReload = false;
        }
        else
        {
            if (sweepDivider)
                --sweepDivider;
            else if (sweepEnable)
            {
                sweepDivider = sweepPeriod + 1;
                adjustPeriod = true;
            }
        }

        if (adjustPeriod)
        {
            if (timer)
                timer = timer;
            uint32_t timerShifted = timer >> sweepShift;
            uint32_t nextTimer = timer + (sweepNegate ? (sweepCarry - timerShifted) : timerShifted);
            if ((nextTimer > 0x7ff) || (nextTimer < 8))
            {
                level = 0;
                length = 0;
            }
            else
            {
                timer = nextTimer;
                updatePeriod();
            }
        }
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
            updatePeriod();
            break;

        case 3:
            timer = (timer & 0xff) | ((value & 0x07) << 8);
            if (enabled)
                length = kLength[(value >> 3) & 0x1f];
            updatePeriod();
            reload();
            break;

        default:
            EMU_ASSERT(false);
        }
    }

    void APU::Pulse::serialize(emu::ISerializer& serializer)
    {
        uint32_t version = 1;
        serializer.serialize(version);
        serializer.serialize(duty);
        serializer.serialize(loop);
        serializer.serialize(constant);
        serializer.serialize(envelope);
        serializer.serialize(sweepEnable);
        serializer.serialize(sweepPeriod);
        serializer.serialize(sweepNegate);
        serializer.serialize(sweepShift);
        serializer.serialize(timer);
        serializer.serialize(length);
        serializer.serialize(enabled);
        serializer.serialize(period);
        serializer.serialize(timerCount);
        serializer.serialize(cycle);
        serializer.serialize(level);
        serializer.serialize(envelopeDivider);
        serializer.serialize(envelopeCounter);
        serializer.serialize(envelopeVolume);
        serializer.serialize(sweepReload);
        serializer.serialize(sweepDivider);
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

        enabled = false;
        period = 0;
        timerCount = 0;
        linearCount = 0;
        sequence = 0;
        level = 0;
    }

    void APU::Triangle::enable(bool _enabled)
    {
        enabled = _enabled;
        if (!enabled)
        {
            level = 0;
            length = 0;
        }
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
            -15, -13, -11, -9, -7, -5, -3, -1, +1, +3, +5, +8, +9, +11, +13, +15,
            +15, +13, +11, +9, +7, +5, +3, +1, -1, -3, -5, -8, -9, -11, -13, -15,
        };
        uint32_t value = kLevel[sequence];
        level = (!linearCount || !length || (timer < 4)) ? 0 : value;
    }

    void APU::Triangle::updateLengthCounter()
    {
        if (length)
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
            EMU_ASSERT(false);
            break;

        case 2:
            timer = (timer & ~0xff) | (value & 0xff);
            period = (timer + 1) * masterClockDivider;
            break;

        case 3:
            timer = (timer & 0xff) | ((value & 0x07) << 8);
            if (enabled)
                length = kLength[(value >> 3) & 0x1f];
            period = (timer + 1) * masterClockDivider;
            reload = true;
            break;

        default:
            EMU_ASSERT(false);
        }
    }

    void APU::Triangle::serialize(emu::ISerializer& serializer)
    {
        uint32_t version = 1;
        serializer.serialize(version);
        serializer.serialize(control);
        serializer.serialize(linear);
        serializer.serialize(timer);
        serializer.serialize(length);
        serializer.serialize(reload);
        serializer.serialize(enabled);
        serializer.serialize(period);
        serializer.serialize(timerCount);
        serializer.serialize(linearCount);
        serializer.serialize(sequence);
        serializer.serialize(level);
    }

    ///////////////////////////////////////////////////////////////////////////

    void APU::Noise::reset(uint32_t _masterClockDivider)
    {
        masterClockDivider = _masterClockDivider;

        loop = false;
        constant = false;
        envelope = 0;
        shiftMode = 1;
        timer = 0;
        length = 0;
        enabled = false;

        period = 0;
        timerCount = 0;
        generator = 1;
        level = 0;

        envelopeDivider = 0;
        envelopeCounter = 0;
        envelopeVolume = 0;
    }

    void APU::Noise::enable(bool _enabled)
    {
        enabled = _enabled;
        if (!enabled)
        {
            level = 0;
            length = 0;
        }
    }

    void APU::Noise::reload()
    {
        timerCount = period;
        envelopeCounter = 15;
        envelopeDivider = envelope;
    }

    void APU::Noise::update(uint32_t ticks)
    {
        if (!period)
            return;

        // Update timer
        while (ticks > timerCount)
        {
            ticks -= timerCount;
            timerCount = period;
            uint32_t feedback = ((generator >> shiftMode) ^ generator) & 1;
            generator = (generator >> 1) | (feedback << 14);
        }
        timerCount -= ticks;

        // Update level
        static const int8_t kVolume[16][2] =
        {
            {  -0,  +0 }, {  -1,  +1 }, {  -2,  +2 }, {  -3,  +3 },
            {  -4,  +4 }, {  -5,  +5 }, {  -6,  +6 }, {  -7,  +7 },
            {  -8,  +8 }, {  -9,  +9 }, { -10, +10 }, { -11, +11 },
            { -12, +12 }, { -13, +13 }, { -14, +14 }, { -15, +15 },
        };
        uint32_t value = (generator & 1) ^ 1;
        level = !length ? 0 : kVolume[envelopeVolume][value];
    }

    void APU::Noise::updatePeriod()
    {
        period = (timer + 1) * masterClockDivider;
    }

    void APU::Noise::updateEnvelope()
    {
        if (envelopeDivider-- == 0)
        {
            envelopeDivider = envelope;
            if (envelopeCounter)
                --envelopeCounter;
            else if (loop)
                envelopeCounter = 15;
        }
        envelopeVolume = constant ? envelope : envelopeCounter;
    }

    void APU::Noise::updateLengthCounter()
    {
        if (!loop && length)
            --length;
    }

    void APU::Noise::write(uint32_t index, uint32_t value)
    {
        static const uint32_t kTimer[] =
        {
            0x004, 0x008, 0x010, 0x020,
            0x040, 0x060, 0x080, 0x0A0,
            0x0CA, 0x0FE, 0x17C, 0x1FC,
            0x2FA, 0x3F8, 0x7F2, 0xFE4,
        };

        switch (index)
        {
        case 0:
            loop = (value & 0x20) != 0;
            constant = (value & 0x10) != 0;
            envelope = (value & 0x0f);
            break;

        case 1:
            EMU_ASSERT(false);
            break;

        case 2:
            shiftMode = (value & 80) ? 6 : 1;
            timer = kTimer[value & 0x0f];
            updatePeriod();
            break;

        case 3:
            if (enabled)
                length = kLength[(value >> 3) & 0x1f];
            reload();
            break;

        default:
            EMU_ASSERT(false);
        }
    }

    void APU::Noise::serialize(emu::ISerializer& serializer)
    {
        uint32_t version = 1;
        serializer.serialize(version);
        serializer.serialize(loop);
        serializer.serialize(constant);
        serializer.serialize(envelope);
        serializer.serialize(shiftMode);
        serializer.serialize(timer);
        serializer.serialize(length);
        serializer.serialize(enabled);
        serializer.serialize(period);
        serializer.serialize(timerCount);
        serializer.serialize(generator);
        serializer.serialize(level);
        serializer.serialize(envelopeDivider);
        serializer.serialize(envelopeCounter);
        serializer.serialize(envelopeVolume);
    }

    ///////////////////////////////////////////////////////////////////////////

    void APU::DMC::reset(emu::Clock& _clock, emu::MemoryBus& _memory, uint32_t _masterClockDivider)
    {
        clock = &_clock;
        memory = &_memory;
        masterClockDivider = _masterClockDivider;

        loop = false;
        period = 0x1ac * masterClockDivider;    // Initialize at the lowest frequency
        sampleAddress = 0xc000;
        sampleLength = 1;

        updateTick = 0;
        timerTick = 0;
        samplePos = 0;
        sampleCount = 0;
        sampleBuffer = 0;
        shift = 0;
        bit = 8;
        level = 0;
        available = false;
        silenced = true;
        irq = false;
    }

    void APU::DMC::enable(uint32_t tick, bool _enabled)
    {
        if (!_enabled)
        {
            sampleCount = 0;
            level = 0;
        }
        else
        {
            if (sampleCount == 0)
            {
                samplePos = sampleAddress;
                sampleCount = sampleLength;
                updateReader(tick);
            }
        }
        irq = false;
    }

    void APU::DMC::beginFrame()
    {
        prepareNextReaderTick();
    }

    void APU::DMC::advanceClock(int32_t ticks)
    {
        timerTick -= ticks;
        updateTick -= ticks;
    }

    void APU::DMC::update(uint32_t ticks)
    {
        int32_t lastTick = updateTick + ticks;
        while (lastTick > timerTick)
        {
            timerTick += period;
            if (!silenced)
            {
                bool increase = (shift & 1) != 0;
                if (increase)
                {
                    if (level <= 61)
                        level += 2;
                }
                else
                {
                    if (level >= -62)
                        level -= 2;
                }
                shift >>= 1;
            }
            if (--bit == 0)
            {
                bit = 8;
                silenced = !available;
                if (available)
                {
                    shift = sampleBuffer;
                    available = false;
                    updateReader(timerTick);
                }
                else
                {
                    level = 0;
                }
            }
        }
        updateTick = lastTick;
    }

    void APU::DMC::updateReader(uint32_t tick)
    {
        if (!sampleCount)
            return;
        sampleBuffer = memory_bus_read8(memory->getState(), tick, samplePos);
        samplePos = ((samplePos + 1) & 0xffff) | 0x8000;
        available = true;
        if (--sampleCount == 0)
        {
            if (loop)
            {
                samplePos = sampleAddress;
                sampleCount = sampleLength;
            }
            else
            {
                // TODO: Signal interrupt
                irq = true;
            }
        }

        prepareNextReaderTick();
    }

    void APU::DMC::prepareNextReaderTick()
    {
        // Next reader tick is when the shift buffer will be ready to fetch the next sample
        if (sampleCount)
        {
            uint32_t readerTick = timerTick + period * (bit - 1);
            //clock->addEvent(dummyTimerCallback, nullptr, readerTick);
        }
    }

    void APU::DMC::write(uint32_t index, uint32_t value)
    {
        static const uint32_t kTimer[] =
        {
            0x1ac, 0x17c, 0x154, 0x140,
            0x11e, 0x0fe, 0x0e2, 0x0d6,
            0x0be, 0x0a0, 0x08e, 0x080,
            0x06a, 0x054, 0x048, 0x036,
        };

        switch (index)
        {
        case 0:
            irq = (value & 0x40) != 0;
            loop = (value & 0x20) != 0;
            period = kTimer[value & 0x0f] * masterClockDivider;
            prepareNextReaderTick();
            break;

        case 1:
            level = (value & 0x7f) - 64;
            break;

        case 2:
            sampleAddress = ((value & 0xff) << 6) | 0xc000;
            break;

        case 3:
            sampleLength = ((value & 0xff) << 4) | 0x0001;
            break;

        default:
            EMU_ASSERT(false);
        }
    }

    void APU::DMC::serialize(emu::ISerializer& serializer)
    {
        uint32_t version = 1;
        serializer.serialize(version);
        serializer.serialize(loop);
        serializer.serialize(period);
        serializer.serialize(sampleAddress);
        serializer.serialize(sampleLength);
        serializer.serialize(updateTick);
        serializer.serialize(timerTick);
        serializer.serialize(samplePos);
        serializer.serialize(sampleCount);
        serializer.serialize(sampleBuffer);
        serializer.serialize(shift);
        serializer.serialize(bit);
        serializer.serialize(level);
        serializer.serialize(available);
        serializer.serialize(silenced);
        serializer.serialize(irq);
    }
}
