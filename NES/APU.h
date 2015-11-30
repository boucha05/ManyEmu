#ifndef __APU_H__
#define __APU_H__

#include <Core/Clock.h>
#include <stdint.h>

namespace NES
{
    class Clock;
    class MemoryBus;
    class ISerializer;

    class APU : public Clock::IListener
    {
    public:
        APU();
        ~APU();
        bool create(Clock& clock, MemoryBus& memoryBus, uint32_t masterClockDivider, uint32_t masterClockFrequency);
        void destroy();
        void reset();
        void beginFrame();
        void execute();
        virtual void advanceClock(int32_t ticks);
        virtual void setDesiredTicks(int32_t ticks);
        uint8_t regRead(int32_t ticks, uint32_t addr);
        void regWrite(int32_t ticks, uint32_t addr, uint8_t value);
        void setController(uint32_t index, uint8_t buttons);
        void setSoundBuffer(int16_t* buffer, size_t size);
        void serialize(ISerializer& serializer);

        static const uint32_t APU_REGISTER_COUNT = 0x20;

        static const uint32_t APU_REG_SQ1_VOL = 0x00;
        static const uint32_t APU_REG_SQ1_SWEEP = 0x01;
        static const uint32_t APU_REG_SQ1_LO = 0x02;
        static const uint32_t APU_REG_SQ1_HI = 0x03;
        static const uint32_t APU_REG_SQ2_VOL = 0x04;
        static const uint32_t APU_REG_SQ2_SWEEP = 0x05;
        static const uint32_t APU_REG_SQ2_LO = 0x06;
        static const uint32_t APU_REG_SQ2_HI = 0x07;
        static const uint32_t APU_REG_TRI_LINEAR = 0x8;
        static const uint32_t APU_REG_TRI_LO = 0x0a;
        static const uint32_t APU_REG_TRI_HI = 0x0b;
        static const uint32_t APU_REG_NOISE_VOL = 0x0c;
        static const uint32_t APU_REG_NOISE_LO = 0x0e;
        static const uint32_t APU_REG_NOISE_HI = 0x0f;
        static const uint32_t APU_REG_DMC_FREQ = 0x10;
        static const uint32_t APU_REG_DMC_RAW = 0x11;
        static const uint32_t APU_REG_DMC_START = 0x12;
        static const uint32_t APU_REG_DMC_LEN = 0x13;
        static const uint32_t APU_REG_OAM_DMA = 0x14;
        static const uint32_t APU_REG_SND_CHN = 0x15;
        static const uint32_t APU_REG_JOY1 = 0x16;
        static const uint32_t APU_REG_JOY2 = 0x17;

    private:
        struct Pulse
        {
            // Configuration
            uint32_t    masterClockDivider;
            uint32_t    sweepCarry;

            // Register fields
            uint32_t    duty;
            bool        loop;
            bool        constant;
            uint32_t    envelope;
            bool        sweepEnable;
            uint32_t    sweepPeriod;
            bool        sweepNegate;
            uint32_t    sweepShift;
            uint32_t    timer;
            uint32_t    length;
            bool        enabled;

            // State
            uint32_t    period;
            uint32_t    timerCount;
            uint32_t    cycle;
            int8_t      level;

            // Envelope
            uint32_t    envelopeDivider;
            uint32_t    envelopeCounter;
            uint32_t    envelopeVolume;

            // Sweep
            bool        sweepReload;
            uint32_t    sweepDivider;

            void reset(uint32_t _masterClockDivider, uint32_t _sweepCarry);
            void enable(bool _enabled);
            void reload();
            void update(uint32_t ticks);
            void updatePeriod();
            void updateEnvelope();
            void updateLengthCounter();
            void updateSweep();
            void write(uint32_t index, uint32_t value);
            void serialize(ISerializer& serializer);
        };

        struct Triangle
        {
            // Configuration
            uint32_t    masterClockDivider;

            // Register fields
            bool        control;
            uint32_t    linear;
            uint32_t    timer;
            uint32_t    length;
            bool        reload;
            bool        enabled;

            // State
            uint32_t    period;
            uint32_t    timerCount;
            uint32_t    linearCount;
            uint32_t    sequence;
            int8_t      level;

            void reset(uint32_t _masterClockDivider);
            void enable(bool _enabled);
            void update(uint32_t ticks);
            void updateLengthCounter();
            void updateLinearCounter();
            void write(uint32_t index, uint32_t value);
            void serialize(ISerializer& serializer);
        };

        struct Noise
        {
            // Configuration
            uint32_t    masterClockDivider;

            // Register fields
            bool        loop;
            bool        constant;
            uint32_t    envelope;
            uint32_t    shiftMode;
            uint32_t    timer;
            uint32_t    length;
            bool        enabled;

            // State
            uint32_t    period;
            uint32_t    timerCount;
            uint32_t    generator;
            int8_t      level;

            // Envelope
            uint32_t    envelopeDivider;
            uint32_t    envelopeCounter;
            uint32_t    envelopeVolume;

            void reset(uint32_t _masterClockDivider);
            void enable(bool _enabled);
            void reload();
            void update(uint32_t ticks);
            void updatePeriod();
            void updateEnvelope();
            void updateLengthCounter();
            void write(uint32_t index, uint32_t value);
            void serialize(ISerializer& serializer);
        };

        struct DMC
        {
            // Configuration
            Clock*      clock;
            MemoryBus*  memory;
            uint32_t    masterClockDivider;

            // Register fields
            bool        loop;
            uint32_t    period;
            uint32_t    sampleAddress;
            uint32_t    sampleLength;

            // State
            int32_t     updateTick;
            int32_t     timerTick;
            uint32_t    samplePos;
            uint32_t    sampleCount;
            uint32_t    sampleBuffer;
            uint32_t    shift;
            uint32_t    bit;
            int8_t      level;
            bool        available;
            bool        silenced;
            bool        irq;

            void reset(Clock& _clock, MemoryBus& _memory, uint32_t _masterClockDivider);
            void enable(uint32_t tick, bool _enabled);
            void beginFrame();
            void advanceClock(int32_t ticks);
            void update(uint32_t ticks);
            void updateReader(uint32_t tick);
            void prepareNextReaderTick();
            void write(uint32_t index, uint32_t value);
            void serialize(ISerializer& serializer);
        };

        void initialize();
        void updateEnvelopesAndLinearCounter();
        void updateLengthCountersAndSweepUnits();
        void advanceSamples(int32_t tick);
        void advanceBuffer(int32_t tick);
        void onSequenceEvent(int32_t tick);
        static void onSequenceEvent(void* context, int32_t tick);

        Clock*                  mClock;
        MemoryBus*              mMemory;
        uint32_t                mMasterClockDivider;
        uint32_t                mMasterClockFrequency;
        uint32_t                mFrameCountTicks;
        uint8_t                 mRegister[APU_REGISTER_COUNT];
        uint8_t                 mController[4];
        uint8_t                 mShifter[4];
        int16_t*                mSoundBuffer;
        uint32_t                mSoundBufferSize;
        uint32_t                mSoundBufferOffset;
        uint32_t                mSampleSpeed;
        int32_t                 mBufferTick;
        int32_t                 mSampleTick;
        int32_t                 mSequenceTick;
        uint32_t                mSequenceCount;
        Pulse                   mPulse[2];
        Triangle                mTriangle;
        Noise                   mNoise;
        DMC                     mDMC;
        bool                    mMode5Step;
        bool                    mIRQ;
    };
}

#endif
