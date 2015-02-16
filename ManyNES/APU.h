#ifndef __APU_H__
#define __APU_H__

#include "Clock.h"
#include <stdint.h>

namespace NES
{
    class Clock;
    class MemoryBus;

    class APU : public Clock::IListener
    {
    public:
        APU();
        ~APU();
        bool create(Clock& clock, MemoryBus& memoryBus, uint32_t masterClockDivider, uint32_t masterClockFrequency);
        void destroy();
        void reset();
        void execute();
        virtual void advanceClock(int32_t ticks);
        virtual void setDesiredTicks(int32_t ticks);
        uint8_t regRead(int32_t ticks, uint32_t addr);
        void regWrite(int32_t ticks, uint32_t addr, uint8_t value);
        void setController(uint32_t index, uint8_t buttons);
        void setSoundBuffer(int16_t* buffer, size_t size);
        void advanceBuffer(int32_t tick);

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
            uint32_t    duty;
            uint32_t    volume;
            uint32_t    sweepPeriod;
            uint32_t    sweepShift;
            uint32_t    timer;
            uint32_t    length;
            bool        loop;
            bool        constant;
            bool        sweepEnable;
            bool        sweepNegate;

            void reset();
            void disable();
        };

        void initialize();

        Clock*                  mClock;
        MemoryBus*              mMemory;
        uint32_t                mMasterClockDivider;
        uint32_t                mFrameCountTicks;
        uint8_t                 mRegister[APU_REGISTER_COUNT];
        uint8_t                 mController[4];
        uint8_t                 mShifter[4];
        int16_t*                mSoundBuffer;
        uint32_t                mSoundBufferSize;
        std::vector<uint8_t*>   mSampleBuffer;
        int32_t                 mBufferTick;
        Pulse                   mPulse[2];
        bool                    mMode5Step;
        bool                    mIRQ;
    };
}

#endif
