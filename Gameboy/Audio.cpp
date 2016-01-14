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

    static const uint8_t NRx4_RELOAD = 0x80;
    static const uint8_t NRx4_DECREMENT = 0x40;
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

    void Audio::Sweep::serialize(emu::ISerializer& serializer)
    {
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

    void Audio::Volume::serialize(emu::ISerializer& serializer)
    {
    }

    ////////////////////////////////////////////////////////////////////////////

    void Audio::SquarePattern::serialize(emu::ISerializer& serializer)
    {
    }

    ////////////////////////////////////////////////////////////////////////////

    void Audio::WavePattern::serialize(emu::ISerializer& serializer)
    {
    }

    ////////////////////////////////////////////////////////////////////////////

    void Audio::NoisePattern::serialize(emu::ISerializer& serializer)
    {
    }

    ////////////////////////////////////////////////////////////////////////////

    Audio::Audio()
        : mChannel1Length(mRegNR11, mRegNR14, false)
        , mChannel2Length(mRegNR21, mRegNR24, false)
        , mChannel3Length(mRegNR31, mRegNR34, true)
        , mChannel4Length(mRegNR41, mRegNR44, false)
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
            channel[0] = mChannel1Length.getOutputMask() & 15;
            channel[1] = mChannel2Length.getOutputMask() & 15;
            channel[2] = mChannel3Length.getOutputMask() & 15;
            channel[3] = mChannel4Length.getOutputMask() & 15;
            int8_t value = channel[0] + channel[1] + channel[2] + channel[3];
            emu::word16_t sample;
            sample.w8[0].u = value;
            sample.w8[1].u = 0;
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
            //mChannel1Sweep.step();
        }
        if ((mSequencerStep & 7) == 7)
        {
            //mChannel1Volume.step();
            //mChannel2Volume.step();
            //mChannel4Volume.step();
        }
    }

    void Audio::updateSample(int32_t tick)
    {
        if (mSoundBuffer)
        {
            while (tick - mSampleTick >= static_cast<int32_t>(mTicksPerSample))
            {
                mSampleTick += mTicksPerSample;
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
        mUpdateTick = tick;
    }

    uint8_t Audio::readNR10(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegNR10;
    }

    void Audio::writeNR10(int32_t tick, uint16_t addr, uint8_t value)
    {
        EMU_NOT_IMPLEMENTED();
        mRegNR10 = value;
    }

    uint8_t Audio::readNR11(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegNR11;
    }

    void Audio::writeNR11(int32_t tick, uint16_t addr, uint8_t value)
    {
        EMU_NOT_IMPLEMENTED();
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
        EMU_NOT_IMPLEMENTED();
        mRegNR12 = value;
    }

    uint8_t Audio::readNR13(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegNR13;
    }

    void Audio::writeNR13(int32_t tick, uint16_t addr, uint8_t value)
    {
        EMU_NOT_IMPLEMENTED();
        mRegNR13 = value;
    }

    uint8_t Audio::readNR14(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegNR14;
    }

    void Audio::writeNR14(int32_t tick, uint16_t addr, uint8_t value)
    {
        EMU_NOT_IMPLEMENTED();
        mRegNR14 = value;
        mChannel1Length.onWriteNRx4();
    }

    uint8_t Audio::readNR21(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegNR21;
    }

    void Audio::writeNR21(int32_t tick, uint16_t addr, uint8_t value)
    {
        EMU_NOT_IMPLEMENTED();
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
        EMU_NOT_IMPLEMENTED();
        mRegNR22 = value;
    }

    uint8_t Audio::readNR23(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegNR23;
    }

    void Audio::writeNR23(int32_t tick, uint16_t addr, uint8_t value)
    {
        EMU_NOT_IMPLEMENTED();
        mRegNR23 = value;
    }

    uint8_t Audio::readNR24(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegNR24;
    }

    void Audio::writeNR24(int32_t tick, uint16_t addr, uint8_t value)
    {
        EMU_NOT_IMPLEMENTED();
        mRegNR24 = value;
        mChannel2Length.onWriteNRx4();
    }

    uint8_t Audio::readNR30(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegNR30;
    }

    void Audio::writeNR30(int32_t tick, uint16_t addr, uint8_t value)
    {
        EMU_NOT_IMPLEMENTED();
        mRegNR30 = value;
    }

    uint8_t Audio::readNR31(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegNR31;
    }

    void Audio::writeNR31(int32_t tick, uint16_t addr, uint8_t value)
    {
        EMU_NOT_IMPLEMENTED();
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
        EMU_NOT_IMPLEMENTED();
        mRegNR32 = value;
    }

    uint8_t Audio::readNR33(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegNR33;
    }

    void Audio::writeNR33(int32_t tick, uint16_t addr, uint8_t value)
    {
        EMU_NOT_IMPLEMENTED();
        mRegNR33 = value;
    }

    uint8_t Audio::readNR34(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegNR34;
    }

    void Audio::writeNR34(int32_t tick, uint16_t addr, uint8_t value)
    {
        EMU_NOT_IMPLEMENTED();
        mRegNR34 = value;
        mChannel3Length.onWriteNRx4();
    }

    uint8_t Audio::readNR41(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegNR41;
    }

    void Audio::writeNR41(int32_t tick, uint16_t addr, uint8_t value)
    {
        EMU_NOT_IMPLEMENTED();
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
        EMU_NOT_IMPLEMENTED();
        mRegNR42 = value;
    }

    uint8_t Audio::readNR43(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegNR43;
    }

    void Audio::writeNR43(int32_t tick, uint16_t addr, uint8_t value)
    {
        EMU_NOT_IMPLEMENTED();
        mRegNR43 = value;
    }

    uint8_t Audio::readNR44(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegNR44;
    }

    void Audio::writeNR44(int32_t tick, uint16_t addr, uint8_t value)
    {
        EMU_NOT_IMPLEMENTED();
        mRegNR44 = value;
        mChannel4Length.onWriteNRx4();
    }

    uint8_t Audio::readNR50(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegNR50;
    }

    void Audio::writeNR50(int32_t tick, uint16_t addr, uint8_t value)
    {
        EMU_NOT_IMPLEMENTED();
        mRegNR50 = value;
    }

    uint8_t Audio::readNR51(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegNR51;
    }

    void Audio::writeNR51(int32_t tick, uint16_t addr, uint8_t value)
    {
        EMU_NOT_IMPLEMENTED();
        mRegNR51 = value;
    }

    uint8_t Audio::readNR52(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegNR52;
    }

    void Audio::writeNR52(int32_t tick, uint16_t addr, uint8_t value)
    {
        EMU_NOT_IMPLEMENTED();
        mRegNR52 = value;
    }

    uint8_t Audio::readWAVE(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegWAVE[addr - 0x30];
    }

    void Audio::writeWAVE(int32_t tick, uint16_t addr, uint8_t value)
    {
        EMU_NOT_IMPLEMENTED();
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
        mChannel1Pattern.serialize(serializer);
        mChannel2Length.serialize(serializer);
        mChannel2Volume.serialize(serializer);
        mChannel2Pattern.serialize(serializer);
        mChannel3Length.serialize(serializer);
        mChannel3Pattern.serialize(serializer);
        mChannel4Length.serialize(serializer);
        mChannel4Volume.serialize(serializer);
        mChannel4Pattern.serialize(serializer);
    }
}
