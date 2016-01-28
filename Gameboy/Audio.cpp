#include <Core/RegisterBank.h>
#include <Core/Serialization.h>
#include "Audio.h"
#include "Interrupts.h"

namespace
{
    static const uint32_t MASTER_CLOCK_FREQUENCY_GB = 4194304;
    static const uint32_t SEQUENCER_FREQUENCY = 512;
    static const uint32_t TICKS_PER_SEQUENCER_STEP = MASTER_CLOCK_FREQUENCY_GB / SEQUENCER_FREQUENCY;

    static const uint32_t OUTPUT_MASK_DISABLED = 0x00000000;
    static const uint32_t OUTPUT_MASK_ENABLED = 0xffffffff;

    static const int32_t FREQUENCY_PERIOD_LIMIT = 0x800;
    static const int32_t FREQUENCY_PERIOD_FAST = 4;

    static const int32_t SQUARE_CLOCK_SHIFT = 2;
    static const int32_t SQUARE_CYCLE_SIZE = 8;

    static const int32_t WAVE_CLOCK_SHIFT = 1;
    static const int32_t WAVE_CYCLE_SIZE = 32;

    static const int32_t NOISE_CLOCK_SHIFT = 0;
    static const int32_t NOISE_CYCLE_SIZE = 0x8000;

    static const uint32_t VOLUME_MIN = 0x00;
    static const uint32_t VOLUME_MAX = 0x0f;

    static const uint8_t NRx0_SWEEP_TIME_MASK = 0x70;
    static const uint8_t NRx0_SWEEP_TIME_SHIFT = 4;
    static const uint8_t NRx0_SWEEP_DECREASE = 0x08;
    static const uint8_t NRx0_SWEEP_SHIFT_MASK = 0x07;
    static const uint8_t NRx0_SWEEP_SHIFT_SHIFT = 0;

    static const uint8_t NRx1_DUTY_MASK = 0xc0;
    static const uint8_t NRx1_DUTY_SHIFT = 6;

    static const uint8_t NRx2_VOLUME_MASK = 0xf0;
    static const uint8_t NRx2_VOLUME_SHIFT = 4;
    static const uint8_t NRx2_INCREASE = 0x08;
    static const uint8_t NRx2_COUNTER_MASK = 0x07;
    static const uint8_t NRx2_COUNTER_SHIFT = 0;

    static const uint8_t NR32_OUTPUT_LEVEL_MASK = 0x60;
    static const uint8_t NR32_OUTPUT_LEVEL_SHIFT = 5;

    static const uint8_t NRx4_RELOAD = 0x80;
    static const uint8_t NRx4_DECREMENT = 0x40;
    static const uint8_t NRx4_PERIOD_HI_MASK = 0x7;
    static const uint8_t NRx4_PERIOD_HI_SHIFT = 0x0;

    static const int32_t kVolumeDAC[VOLUME_MAX + 1] =
    {
        -120, -104, -88, -72, -56, -40, -24, -8,
        8, 24, 40, 56, 72, 88, 104, 120,
    };
}

namespace gb
{
    Audio::ClockListener::ClockListener()
    {
        initialize();
    }

    Audio::ClockListener::~ClockListener()
    {
        destroy();
    }

    void Audio::ClockListener::initialize()
    {
        mClock = nullptr;
        mAudio = nullptr;
    }

    bool Audio::ClockListener::create(emu::Clock& clock, Audio& audio)
    {
        mClock = &clock;
        mAudio = &audio;
        mClock->addListener(*this);
        return true;
    }

    void Audio::ClockListener::destroy()
    {
        if (mClock)
            mClock->removeListener(*this);
        initialize();
    }

    void Audio::ClockListener::execute()
    {
        mAudio->execute();
    }

    void Audio::ClockListener::resetClock()
    {
        mAudio->resetClock();
    }

    void Audio::ClockListener::advanceClock(int32_t tick)
    {
        mAudio->advanceClock(tick);
    }

    void Audio::ClockListener::setDesiredTicks(int32_t tick)
    {
        mAudio->setDesiredTicks(tick);
    }

    ////////////////////////////////////////////////////////////////////////////

    Audio::Sweep::Sweep(const uint8_t& NRx0, const uint8_t& NRx4, Frequency& frequency)
        : mNRx0(NRx0)
        , mNRx4(NRx4)
        , mFrequency(frequency)
    {
        reset();
    }

    void Audio::Sweep::reset()
    {
        mEnabled = false;
        mPeriod = 0;
        mCounter = 0;
    }

    void Audio::Sweep::reload()
    {
        reloadSweep();
        auto shift = (mNRx0 & NRx0_SWEEP_SHIFT_MASK) >> NRx0_SWEEP_SHIFT_SHIFT;
        mPeriod = mFrequency.getPeriod();
        mEnabled = mCounter && shift;
    }

    void Audio::Sweep::onWriteNRx4()
    {
        if (mNRx4 & NRx4_RELOAD)
            reload();
    }

    void Audio::Sweep::reloadSweep()
    {
        mCounter = (mNRx0 & NRx0_SWEEP_TIME_MASK) >> NRx0_SWEEP_TIME_SHIFT;
    }

    void Audio::Sweep::step()
    {
        if (mEnabled)
        {
            if (--mCounter > 0)
                return;
            reloadSweep();
            auto shift = (mNRx0 & NRx0_SWEEP_SHIFT_MASK) >> NRx0_SWEEP_SHIFT_SHIFT;
            if (!mCounter || !shift)
                mEnabled = false;
            else
            {
                int32_t deltaPeriod = mPeriod >> shift;
                if (mNRx0 & NRx0_SWEEP_DECREASE)
                    deltaPeriod = -deltaPeriod;
                auto nextPeriod = mPeriod + deltaPeriod;
                if (nextPeriod >= FREQUENCY_PERIOD_LIMIT)
                    mEnabled = false;
                mPeriod = nextPeriod;
                mFrequency.setPeriod(nextPeriod);
            }
        }
    }

    void Audio::Sweep::serialize(emu::ISerializer& serializer)
    {
        serializer.serialize(mEnabled);
        serializer.serialize(mPeriod);
        serializer.serialize(mCounter);
    }

    ////////////////////////////////////////////////////////////////////////////

    Audio::Length::Length(const uint8_t& NRx1, const uint8_t& NRx4, bool is8bit)
        : mNRx1(NRx1)
        , mNRx4(NRx4)
        , mLoadMask(is8bit ? 0xff : 0x3f)
    {
        reset();
    }

    void Audio::Length::reset()
    {
        mCounter = 0;
        mOutputMask = OUTPUT_MASK_DISABLED;
    }

    void Audio::Length::reload()
    {
        mCounter = mLoadMask + 1 - (mNRx1 & mLoadMask);
        mOutputMask = OUTPUT_MASK_ENABLED;
    }

    void Audio::Length::onWriteNRx1()
    {
        reload();
    }

    void Audio::Length::onWriteNRx4()
    {
        if (mNRx4 & NRx4_RELOAD)
            reload();
    }

    void Audio::Length::step()
    {
        if ((mNRx4 & NRx4_DECREMENT) && mCounter)
        {
            --mCounter;
            if (mCounter == 0)
                mOutputMask = OUTPUT_MASK_DISABLED;
        }
    }

    void Audio::Length::serialize(emu::ISerializer& serializer)
    {
        serializer.serialize(mCounter);
        serializer.serialize(mOutputMask);
    }

    ////////////////////////////////////////////////////////////////////////////

    Audio::Volume::Volume(const uint8_t& NRx2, const uint8_t& NRx4)
        : mNRx2(NRx2)
        , mNRx4(NRx4)
    {
        reset();
    }

    void Audio::Volume::reset()
    {
        mCounter = 0;
        mVolume = 0;
    }

    void Audio::Volume::reload()
    {
        mCounter = (mNRx2 & NRx2_COUNTER_MASK) >> NRx2_COUNTER_SHIFT;
        mVolume = (mNRx2 & NRx2_VOLUME_MASK) >> NRx2_VOLUME_SHIFT;
    }

    void Audio::Volume::onWriteNRx4()
    {
        if (mNRx4 & NRx4_RELOAD)
            reload();
    }

    void Audio::Volume::step()
    {
        if (mCounter)
        {
            --mCounter;
            if (mNRx2 & NRx2_INCREASE)
            {
                if (mVolume < VOLUME_MAX)
                    ++mVolume;
            }
            else
            {
                if (mVolume > VOLUME_MIN)
                    --mVolume;
            }
        }
    }

    void Audio::Volume::serialize(emu::ISerializer& serializer)
    {
        serializer.serialize(mCounter);
        serializer.serialize(mVolume);
    }

    ////////////////////////////////////////////////////////////////////////////

    Audio::Frequency::Frequency(const uint8_t& NRx3, const uint8_t& NRx4, int32_t clockShift, int32_t cycleSize)
        : mNRx3(NRx3)
        , mNRx4(NRx4)
        , mClockShiftRef(clockShift)
        , mCycleSizeRef(cycleSize)
    {
        initClock(1);
        reset();
    }

    bool Audio::Frequency::initClock(int32_t clockDivider)
    {
        EMU_VERIFY((clockDivider == 2) || (clockDivider == 1));
        mClockShift = mClockShiftRef + (clockDivider == 2 ? 1 : 0);
        return true;
    }

    void Audio::Frequency::reset()
    {
        mClockLastTick = 0;
        mClockStep = 0;
        mCycle = 0;
        setPeriod(0);
    }

    void Audio::Frequency::reload()
    {
        mClockStep = 0;
        mCycle = 0;
        auto period = getPeriod();
        setPeriod(period);
    }

    void Audio::Frequency::advanceClock(int32_t tick)
    {
        mClockLastTick -= tick;
    }

    void Audio::Frequency::onWriteNRx4(int32_t tick)
    {
        if (mNRx4 & NRx4_RELOAD)
            reload();
    }

    void Audio::Frequency::setPeriod(int32_t period)
    {
        period = FREQUENCY_PERIOD_LIMIT - period;
        mClockPeriod = period;
        mClockPeriodFast = period * FREQUENCY_PERIOD_FAST;
        if (mClockStep >= mClockPeriod)
            mClockStep = 0;
    }

    int32_t Audio::Frequency::getPeriod() const
    {
        auto period = mNRx3 | (((mNRx4 & NRx4_PERIOD_HI_MASK) >> NRx4_PERIOD_HI_SHIFT) << 8);
        return period;
    }

    void Audio::Frequency::update(int32_t tick)
    {
        int32_t clockStart = mClockLastTick >> mClockShift;
        int32_t clockEnd = tick >> mClockShift;
        int32_t deltaClock = clockEnd - clockStart;
        int32_t remainingSteps = mClockPeriod - mClockStep;
        mClockLastTick = tick;
        if (deltaClock < remainingSteps)
        {
            mClockStep += deltaClock;
        }
        else
        {
            int32_t deltaCycles = 0;
            deltaClock -= remainingSteps;
            ++deltaCycles;
            if (deltaClock >= mClockPeriodFast)
            {
                deltaCycles += deltaClock / mClockPeriod;
                mClockStep = deltaClock % mClockPeriod;
            }
            else
            {
                while (deltaClock >= mClockPeriod)
                {
                    deltaClock -= mClockPeriod;
                    ++deltaCycles;
                }
                mClockStep = deltaClock;
            }
            mCycle = (mCycle + deltaCycles) & (mCycleSizeRef - 1);
        }
    }

    void Audio::Frequency::serialize(emu::ISerializer& serializer)
    {
        serializer.serialize(mClockLastTick);
        serializer.serialize(mClockStep);
        serializer.serialize(mClockPeriodFast);
        serializer.serialize(mClockPeriod);
        serializer.serialize(mCycle);
    }

    ////////////////////////////////////////////////////////////////////////////

    Audio::SquarePattern::SquarePattern(const uint8_t& NRx1)
        : mNRx1(NRx1)
    {
    }

    int32_t Audio::SquarePattern::getSample(uint32_t cycle, uint32_t volume) const
    {
        static const uint8_t kLevel[4][SQUARE_CYCLE_SIZE] =
        {
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff },
            { 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff },
            { 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff },
            { 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00 },
        };
        auto duty = (mNRx1 & NRx1_DUTY_MASK) >> NRx1_DUTY_SHIFT;
        auto level = kLevel[duty][cycle];
        auto result = kVolumeDAC[volume & level];
        return result;
    }

    ////////////////////////////////////////////////////////////////////////////

    Audio::WavePattern::WavePattern(const uint8_t& NRx0, const uint8_t& NRx2, const uint8_t* wave)
        : mNRx0(NRx0)
        , mNRx2(NRx2)
        , mWave(wave)
    {
    }
    
    int32_t Audio::WavePattern::getSample(uint32_t cycle) const
    {
        static const int8_t kLevelShift[4] =
        {
            4, 0, 1, 2
        };
        auto index = cycle >> 1;
        auto nibbleShift = ((cycle ^ 1) & 1) << 2;
        auto nibble = (mWave[index] >> nibbleShift) & 0x0f;
        auto levelShift = kLevelShift[(mNRx2 & NR32_OUTPUT_LEVEL_MASK) >> NR32_OUTPUT_LEVEL_SHIFT];
        auto level = nibble >> levelShift;
        auto result = kVolumeDAC[level];
        return result;
    }

    ////////////////////////////////////////////////////////////////////////////

    Audio::NoisePattern::NoisePattern(const uint8_t& NRx3, const uint8_t& NRx4)
        : mNRx3(NRx3)
        , mNRx4(NRx4)
    {
    }

    int32_t Audio::NoisePattern::getSample(uint32_t cycle, uint32_t volume) const
    {
        return 0;
    }

    ////////////////////////////////////////////////////////////////////////////

    Audio::Audio()
        : mChannel1Length(mRegNR11, mRegNR14, false)
        , mChannel1Volume(mRegNR12, mRegNR14)
        , mChannel1Frequency(mRegNR13, mRegNR14, SQUARE_CLOCK_SHIFT, SQUARE_CYCLE_SIZE)
        , mChannel1Sweep(mRegNR10, mRegNR14, mChannel1Frequency)
        , mChannel1Pattern(mRegNR11)
        , mChannel2Length(mRegNR21, mRegNR24, false)
        , mChannel2Volume(mRegNR22, mRegNR24)
        , mChannel2Frequency(mRegNR23, mRegNR24, SQUARE_CLOCK_SHIFT, SQUARE_CYCLE_SIZE)
        , mChannel2Pattern(mRegNR21)
        , mChannel3Length(mRegNR31, mRegNR34, true)
        , mChannel3Frequency(mRegNR33, mRegNR34, WAVE_CLOCK_SHIFT, WAVE_CYCLE_SIZE)
        , mChannel3Pattern(mRegNR30, mRegNR32, mRegWAVE)
        , mChannel4Length(mRegNR41, mRegNR44, false)
        , mChannel4Volume(mRegNR42, mRegNR44)
        , mChannel4Frequency(mRegNR43, mRegNR44, NOISE_CLOCK_SHIFT, NOISE_CYCLE_SIZE)
        , mChannel4Pattern(mRegNR43, mRegNR44)
    {
        initialize();
    }

    Audio::~Audio()
    {
        destroy();
    }

    void Audio::initialize()
    {
        mClock = nullptr;
        mSoundBuffer = nullptr;
        mSoundBufferSize = 0;
        mSoundBufferOffset = 0;
        resetClock();
    }

    bool Audio::create(emu::Clock& clock, uint32_t masterClockFrequency, uint32_t masterClockDivider, emu::RegisterBank& registers)
    {
        mClock = &clock;
        EMU_VERIFY(mClockListener.create(clock, *this));

        // The following timings are not exactly true. They are used to match correctly with a frame rate of 60 Hz and to allow SGB to run a bit faster
        mMasterClockFrequency = MASTER_CLOCK_FREQUENCY_GB * masterClockDivider;
        mTicksPerSequencerStep = TICKS_PER_SEQUENCER_STEP * masterClockDivider;
        mTicksPerSample = 0;

        EMU_VERIFY(mChannel1Frequency.initClock(masterClockDivider));
        EMU_VERIFY(mChannel2Frequency.initClock(masterClockDivider));
        EMU_VERIFY(mChannel3Frequency.initClock(masterClockDivider));
        EMU_VERIFY(mChannel4Frequency.initClock(masterClockDivider));

        EMU_VERIFY(mRegisterAccessors.read.NR10.create(registers, 0x10, *this, &Audio::readNR10));
        EMU_VERIFY(mRegisterAccessors.read.NR11.create(registers, 0x11, *this, &Audio::readNR11));
        EMU_VERIFY(mRegisterAccessors.read.NR12.create(registers, 0x12, *this, &Audio::readNR12));
        EMU_VERIFY(mRegisterAccessors.read.NR13.create(registers, 0x13, *this, &Audio::readNR13));
        EMU_VERIFY(mRegisterAccessors.read.NR14.create(registers, 0x14, *this, &Audio::readNR14));
        EMU_VERIFY(mRegisterAccessors.read.NR21.create(registers, 0x16, *this, &Audio::readNR21));
        EMU_VERIFY(mRegisterAccessors.read.NR22.create(registers, 0x17, *this, &Audio::readNR22));
        EMU_VERIFY(mRegisterAccessors.read.NR23.create(registers, 0x18, *this, &Audio::readNR23));
        EMU_VERIFY(mRegisterAccessors.read.NR24.create(registers, 0x19, *this, &Audio::readNR24));
        EMU_VERIFY(mRegisterAccessors.read.NR30.create(registers, 0x1A, *this, &Audio::readNR30));
        EMU_VERIFY(mRegisterAccessors.read.NR31.create(registers, 0x1B, *this, &Audio::readNR31));
        EMU_VERIFY(mRegisterAccessors.read.NR32.create(registers, 0x1C, *this, &Audio::readNR32));
        EMU_VERIFY(mRegisterAccessors.read.NR33.create(registers, 0x1D, *this, &Audio::readNR33));
        EMU_VERIFY(mRegisterAccessors.read.NR34.create(registers, 0x1E, *this, &Audio::readNR34));
        EMU_VERIFY(mRegisterAccessors.read.NR41.create(registers, 0x20, *this, &Audio::readNR41));
        EMU_VERIFY(mRegisterAccessors.read.NR42.create(registers, 0x21, *this, &Audio::readNR42));
        EMU_VERIFY(mRegisterAccessors.read.NR43.create(registers, 0x22, *this, &Audio::readNR43));
        EMU_VERIFY(mRegisterAccessors.read.NR44.create(registers, 0x23, *this, &Audio::readNR44));
        EMU_VERIFY(mRegisterAccessors.read.NR50.create(registers, 0x24, *this, &Audio::readNR50));
        EMU_VERIFY(mRegisterAccessors.read.NR51.create(registers, 0x25, *this, &Audio::readNR51));
        EMU_VERIFY(mRegisterAccessors.read.NR52.create(registers, 0x26, *this, &Audio::readNR52));

        EMU_VERIFY(mRegisterAccessors.write.NR10.create(registers, 0x10, *this, &Audio::writeNR10));
        EMU_VERIFY(mRegisterAccessors.write.NR11.create(registers, 0x11, *this, &Audio::writeNR11));
        EMU_VERIFY(mRegisterAccessors.write.NR12.create(registers, 0x12, *this, &Audio::writeNR12));
        EMU_VERIFY(mRegisterAccessors.write.NR13.create(registers, 0x13, *this, &Audio::writeNR13));
        EMU_VERIFY(mRegisterAccessors.write.NR14.create(registers, 0x14, *this, &Audio::writeNR14));
        EMU_VERIFY(mRegisterAccessors.write.NR21.create(registers, 0x16, *this, &Audio::writeNR21));
        EMU_VERIFY(mRegisterAccessors.write.NR22.create(registers, 0x17, *this, &Audio::writeNR22));
        EMU_VERIFY(mRegisterAccessors.write.NR23.create(registers, 0x18, *this, &Audio::writeNR23));
        EMU_VERIFY(mRegisterAccessors.write.NR24.create(registers, 0x19, *this, &Audio::writeNR24));
        EMU_VERIFY(mRegisterAccessors.write.NR30.create(registers, 0x1A, *this, &Audio::writeNR30));
        EMU_VERIFY(mRegisterAccessors.write.NR31.create(registers, 0x1B, *this, &Audio::writeNR31));
        EMU_VERIFY(mRegisterAccessors.write.NR32.create(registers, 0x1C, *this, &Audio::writeNR32));
        EMU_VERIFY(mRegisterAccessors.write.NR33.create(registers, 0x1D, *this, &Audio::writeNR33));
        EMU_VERIFY(mRegisterAccessors.write.NR34.create(registers, 0x1E, *this, &Audio::writeNR34));
        EMU_VERIFY(mRegisterAccessors.write.NR41.create(registers, 0x20, *this, &Audio::writeNR41));
        EMU_VERIFY(mRegisterAccessors.write.NR42.create(registers, 0x21, *this, &Audio::writeNR42));
        EMU_VERIFY(mRegisterAccessors.write.NR43.create(registers, 0x22, *this, &Audio::writeNR43));
        EMU_VERIFY(mRegisterAccessors.write.NR44.create(registers, 0x23, *this, &Audio::writeNR44));
        EMU_VERIFY(mRegisterAccessors.write.NR50.create(registers, 0x24, *this, &Audio::writeNR50));
        EMU_VERIFY(mRegisterAccessors.write.NR51.create(registers, 0x25, *this, &Audio::writeNR51));
        EMU_VERIFY(mRegisterAccessors.write.NR52.create(registers, 0x26, *this, &Audio::writeNR52));

        for (uint32_t index = 0; index < EMU_ARRAY_SIZE(mRegWAVE); ++index)
        {
            EMU_VERIFY(mRegisterAccessors.read.WAVE[index].create(registers, 0x30 + index, *this, &Audio::readWAVE));
            EMU_VERIFY(mRegisterAccessors.write.WAVE[index].create(registers, 0x30 + index, *this, &Audio::writeWAVE));
        }

        return true;
    }

    void Audio::destroy()
    {
        mClockListener.destroy();
        mRegisterAccessors = RegisterAccessors();
        initialize();
    }

    void Audio::execute()
    {
        update(mDesiredTick);
    }

    void Audio::resetClock()
    {
        mDesiredTick = 0;
        mUpdateTick = 0;
        mSampleTick = 0;
        mSequencerTick = 0;
    }

    void Audio::advanceClock(int32_t tick)
    {
        if (mSoundBuffer)
        {
            EMU_ASSERT(mSoundBufferOffset <= mSoundBufferSize);
            while (mSoundBufferOffset < mSoundBufferSize)
            {
                mSoundBuffer[mSoundBufferOffset] = mSoundBuffer[mSoundBufferOffset - 1];
                ++mSoundBufferOffset;
            }
            mSoundBufferOffset -= mSoundBufferSize;
        }

        mDesiredTick -= tick;
        mUpdateTick -= tick;
        mSampleTick = 0;    // TODO: Do a -= tick once we stop producing a fixed number of samples per frame
        mSequencerTick -= tick;

        mChannel1Frequency.advanceClock(tick);
        mChannel2Frequency.advanceClock(tick);
        mChannel3Frequency.advanceClock(tick);
        mChannel4Frequency.advanceClock(tick);
    }

    void Audio::setDesiredTicks(int32_t tick)
    {
        mDesiredTick = tick;
    }

    void Audio::sampleStep()
    {
        if (mSoundBufferOffset < mSoundBufferSize)
        {
            int8_t channel[4];
            channel[0] = mChannel1Length.getOutputMask() & mChannel1Pattern.getSample(mChannel1Frequency.getCycle(), mChannel1Volume.getVolume());
            channel[1] = mChannel2Length.getOutputMask() & mChannel2Pattern.getSample(mChannel2Frequency.getCycle(), mChannel2Volume.getVolume());
            channel[2] = mChannel3Length.getOutputMask() & mChannel3Pattern.getSample(mChannel3Frequency.getCycle());
            channel[3] = 0;// mChannel4Length.getOutputMask() & mChannel4Volume.getVolume();
            int8_t value = channel[0] + channel[1] + channel[2] + channel[3];
            emu::word16_t sample;
            //sample.w8[0].u = value;
            //sample.w8[1].u = 0;
            sample.i = value << 6;
            mSoundBuffer[mSoundBufferOffset] = sample.u;
        }
        ++mSoundBufferOffset;
    }

    void Audio::sequencerStep()
    {
        mSequencerStep = (mSequencerStep + 1) & 7;
        if ((mSequencerStep & 1) == 0)
        {
            mChannel1Length.step();
            mChannel2Length.step();
            mChannel3Length.step();
            mChannel4Length.step();
        }
        if ((mSequencerStep & 3) == 2)
        {
            mChannel1Sweep.step();
        }
        if ((mSequencerStep & 7) == 7)
        {
            mChannel1Volume.step();
            mChannel2Volume.step();
            mChannel4Volume.step();
        }
    }

    void Audio::updateSample(int32_t tick)
    {
        if (mSoundBuffer)
        {
            while (tick - mSampleTick >= static_cast<int32_t>(mTicksPerSample))
            {
                mSampleTick += mTicksPerSample;
                mChannel1Frequency.update(mSampleTick);
                mChannel2Frequency.update(mSampleTick);
                mChannel3Frequency.update(mSampleTick);
                mChannel4Frequency.update(mSampleTick);
                sampleStep();
            }
        }
    }

    void Audio::updateSequencer(int32_t tick)
    {
        while (tick - mSequencerTick >= static_cast<int32_t>(mTicksPerSequencerStep))
        {
            mSequencerTick += mTicksPerSequencerStep;
            updateSample(mSequencerTick);
            sequencerStep();
        }
        updateSample(tick);
    }

    void Audio::update(int32_t tick)
    {
        if (mUpdateTick >= tick)
            return;

        updateSequencer(tick);
        mChannel1Frequency.update(tick);
        mChannel2Frequency.update(tick);
        mChannel3Frequency.update(tick);
        mChannel4Frequency.update(tick);
        mUpdateTick = tick;
    }

    uint8_t Audio::readNR10(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegNR10;
    }

    void Audio::writeNR10(int32_t tick, uint16_t addr, uint8_t value)
    {
        update(tick);
        mRegNR10 = value;
    }

    uint8_t Audio::readNR11(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegNR11;
    }

    void Audio::writeNR11(int32_t tick, uint16_t addr, uint8_t value)
    {
        update(tick);
        mRegNR11 = value;
        mChannel1Length.onWriteNRx1();
    }

    uint8_t Audio::readNR12(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegNR12;
    }

    void Audio::writeNR12(int32_t tick, uint16_t addr, uint8_t value)
    {
        update(tick);
        mRegNR12 = value;
    }

    uint8_t Audio::readNR13(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegNR13;
    }

    void Audio::writeNR13(int32_t tick, uint16_t addr, uint8_t value)
    {
        update(tick);
        mRegNR13 = value;
    }

    uint8_t Audio::readNR14(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegNR14;
    }

    void Audio::writeNR14(int32_t tick, uint16_t addr, uint8_t value)
    {
        update(tick);
        mRegNR14 = value;
        mChannel1Length.onWriteNRx4();
        mChannel1Volume.onWriteNRx4();
        mChannel1Frequency.onWriteNRx4(tick);
        mChannel1Sweep.onWriteNRx4();
    }

    uint8_t Audio::readNR21(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegNR21;
    }

    void Audio::writeNR21(int32_t tick, uint16_t addr, uint8_t value)
    {
        update(tick);
        mRegNR21 = value;
        mChannel2Length.onWriteNRx1();
    }

    uint8_t Audio::readNR22(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegNR22;
    }

    void Audio::writeNR22(int32_t tick, uint16_t addr, uint8_t value)
    {
        update(tick);
        mRegNR22 = value;
    }

    uint8_t Audio::readNR23(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegNR23;
    }

    void Audio::writeNR23(int32_t tick, uint16_t addr, uint8_t value)
    {
        update(tick);
        mRegNR23 = value;
    }

    uint8_t Audio::readNR24(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegNR24;
    }

    void Audio::writeNR24(int32_t tick, uint16_t addr, uint8_t value)
    {
        update(tick);
        mRegNR24 = value;
        mChannel2Length.onWriteNRx4();
        mChannel2Volume.onWriteNRx4();
        mChannel2Frequency.onWriteNRx4(tick);
    }

    uint8_t Audio::readNR30(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegNR30;
    }

    void Audio::writeNR30(int32_t tick, uint16_t addr, uint8_t value)
    {
        update(tick);
        mRegNR30 = value;
    }

    uint8_t Audio::readNR31(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegNR31;
    }

    void Audio::writeNR31(int32_t tick, uint16_t addr, uint8_t value)
    {
        update(tick);
        mRegNR31 = value;
        mChannel3Length.onWriteNRx1();
    }

    uint8_t Audio::readNR32(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegNR32;
    }

    void Audio::writeNR32(int32_t tick, uint16_t addr, uint8_t value)
    {
        update(tick);
        mRegNR32 = value;
    }

    uint8_t Audio::readNR33(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegNR33;
    }

    void Audio::writeNR33(int32_t tick, uint16_t addr, uint8_t value)
    {
        update(tick);
        mRegNR33 = value;
    }

    uint8_t Audio::readNR34(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegNR34;
    }

    void Audio::writeNR34(int32_t tick, uint16_t addr, uint8_t value)
    {
        update(tick);
        mRegNR34 = value;
        mChannel3Length.onWriteNRx4();
        mChannel3Frequency.onWriteNRx4(tick);
    }

    uint8_t Audio::readNR41(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegNR41;
    }

    void Audio::writeNR41(int32_t tick, uint16_t addr, uint8_t value)
    {
        update(tick);
        mRegNR41 = value;
        mChannel4Length.onWriteNRx1();
    }

    uint8_t Audio::readNR42(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegNR42;
    }

    void Audio::writeNR42(int32_t tick, uint16_t addr, uint8_t value)
    {
        update(tick);
        mRegNR42 = value;
    }

    uint8_t Audio::readNR43(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegNR43;
    }

    void Audio::writeNR43(int32_t tick, uint16_t addr, uint8_t value)
    {
        update(tick);
        mRegNR43 = value;
    }

    uint8_t Audio::readNR44(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegNR44;
    }

    void Audio::writeNR44(int32_t tick, uint16_t addr, uint8_t value)
    {
        update(tick);
        mRegNR44 = value;
        mChannel4Length.onWriteNRx4();
        mChannel4Volume.onWriteNRx4();
        mChannel4Frequency.onWriteNRx4(tick);
    }

    uint8_t Audio::readNR50(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegNR50;
    }

    void Audio::writeNR50(int32_t tick, uint16_t addr, uint8_t value)
    {
        update(tick);
        mRegNR50 = value;
    }

    uint8_t Audio::readNR51(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegNR51;
    }

    void Audio::writeNR51(int32_t tick, uint16_t addr, uint8_t value)
    {
        update(tick);
        mRegNR51 = value;
    }

    uint8_t Audio::readNR52(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegNR52;
    }

    void Audio::writeNR52(int32_t tick, uint16_t addr, uint8_t value)
    {
        update(tick);
        mRegNR52 = value;
    }

    uint8_t Audio::readWAVE(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegWAVE[addr - 0x30];
    }

    void Audio::writeWAVE(int32_t tick, uint16_t addr, uint8_t value)
    {
        update(tick);
        mRegWAVE[addr - 0x30] = value;
    }

    void Audio::reset()
    {
        mSoundBufferOffset = 0;
        mRegNR10 = 0x80;
        mRegNR11 = 0xBF;
        mRegNR12 = 0xF3;
        mRegNR13 = 0x00;
        mRegNR14 = 0xBF;
        mRegNR21 = 0x3F;
        mRegNR22 = 0x00;
        mRegNR23 = 0x00;
        mRegNR24 = 0xBF;
        mRegNR30 = 0x7F;
        mRegNR31 = 0xFF;
        mRegNR32 = 0x9F;
        mRegNR33 = 0xBF;
        mRegNR34 = 0x00;
        mRegNR41 = 0xFF;
        mRegNR42 = 0x00;
        mRegNR43 = 0x00;
        mRegNR44 = 0xBF;
        mRegNR50 = 0x77;
        mRegNR51 = 0xF3;
        mRegNR52 = 0xF1;
        memset(mRegWAVE, EMU_ARRAY_SIZE(mRegWAVE), 0x00);
        mChannel1Sweep.reset();
        mChannel1Length.reset();
        mChannel1Volume.reset();
        mChannel1Frequency.reset();
        mChannel2Length.reset();
        mChannel2Volume.reset();
        mChannel2Frequency.reset();
        mChannel3Length.reset();
        mChannel3Frequency.reset();
        mChannel4Length.reset();
        mChannel4Volume.reset();
        mChannel4Frequency.reset();
    }

    void Audio::setSoundBuffer(int16_t* buffer, size_t size)
    {
        mSoundBuffer = buffer;
        mSoundBufferSize = static_cast<uint32_t>(size);
        mSoundBufferOffset = 0;
        mTicksPerSample = mMasterClockFrequency / (60 * mSoundBufferSize) + 1;
    }

    void Audio::serialize(emu::ISerializer& serializer)
    {
        serializer.serialize(mDesiredTick);
        serializer.serialize(mUpdateTick);
        serializer.serialize(mSequencerTick);
        serializer.serialize(mSequencerStep);
        serializer.serialize(mRegNR10);
        serializer.serialize(mRegNR11);
        serializer.serialize(mRegNR12);
        serializer.serialize(mRegNR13);
        serializer.serialize(mRegNR14);
        serializer.serialize(mRegNR21);
        serializer.serialize(mRegNR22);
        serializer.serialize(mRegNR23);
        serializer.serialize(mRegNR24);
        serializer.serialize(mRegNR30);
        serializer.serialize(mRegNR31);
        serializer.serialize(mRegNR32);
        serializer.serialize(mRegNR33);
        serializer.serialize(mRegNR34);
        serializer.serialize(mRegNR41);
        serializer.serialize(mRegNR42);
        serializer.serialize(mRegNR43);
        serializer.serialize(mRegNR44);
        serializer.serialize(mRegNR50);
        serializer.serialize(mRegNR51);
        serializer.serialize(mRegNR52);
        serializer.serialize(mRegWAVE, EMU_ARRAY_SIZE(mRegWAVE));
        mChannel1Sweep.serialize(serializer);
        mChannel1Length.serialize(serializer);
        mChannel1Volume.serialize(serializer);
        mChannel1Frequency.serialize(serializer);
        mChannel2Length.serialize(serializer);
        mChannel2Volume.serialize(serializer);
        mChannel2Frequency.serialize(serializer);
        mChannel3Length.serialize(serializer);
        mChannel3Frequency.serialize(serializer);
        mChannel4Length.serialize(serializer);
        mChannel4Volume.serialize(serializer);
        mChannel4Frequency.serialize(serializer);
    }
}
