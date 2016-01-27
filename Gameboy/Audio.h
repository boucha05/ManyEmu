#pragma once

#include <Core/Clock.h>
#include <Core/Core.h>
#include <Core/RegisterBank.h>
#include "GB.h"

namespace gb
{
    class Audio
    {
    public:
        Audio();
        ~Audio();
        bool create(emu::Clock& clock, uint32_t masterClockFrequency, uint32_t masterClockDivider, emu::RegisterBank& registers);
        void destroy();
        void reset();
        void setSoundBuffer(int16_t* buffer, size_t size);
        void serialize(emu::ISerializer& serializer);

    private:
        class ClockListener : public emu::Clock::IListener
        {
        public:
            ClockListener();
            ~ClockListener();
            void initialize();
            bool create(emu::Clock& clock, Audio& audio);
            void destroy();
            virtual void execute() override;
            virtual void resetClock() override;
            virtual void advanceClock(int32_t tick) override;
            virtual void setDesiredTicks(int32_t tick) override;

        private:
            emu::Clock*     mClock;
            Audio*          mAudio;
        };

        struct RegisterAccessors
        {
            struct
            {
                emu::RegisterRead   NR10;
                emu::RegisterRead   NR11;
                emu::RegisterRead   NR12;
                emu::RegisterRead   NR13;
                emu::RegisterRead   NR14;
                emu::RegisterRead   NR21;
                emu::RegisterRead   NR22;
                emu::RegisterRead   NR23;
                emu::RegisterRead   NR24;
                emu::RegisterRead   NR30;
                emu::RegisterRead   NR31;
                emu::RegisterRead   NR32;
                emu::RegisterRead   NR33;
                emu::RegisterRead   NR34;
                emu::RegisterRead   NR41;
                emu::RegisterRead   NR42;
                emu::RegisterRead   NR43;
                emu::RegisterRead   NR44;
                emu::RegisterRead   NR50;
                emu::RegisterRead   NR51;
                emu::RegisterRead   NR52;
                emu::RegisterRead   WAVE[16];
            }                       read;

            struct
            {
                emu::RegisterWrite  NR10;
                emu::RegisterWrite  NR11;
                emu::RegisterWrite  NR12;
                emu::RegisterWrite  NR13;
                emu::RegisterWrite  NR14;
                emu::RegisterWrite  NR21;
                emu::RegisterWrite  NR22;
                emu::RegisterWrite  NR23;
                emu::RegisterWrite  NR24;
                emu::RegisterWrite  NR30;
                emu::RegisterWrite  NR31;
                emu::RegisterWrite  NR32;
                emu::RegisterWrite  NR33;
                emu::RegisterWrite  NR34;
                emu::RegisterWrite  NR41;
                emu::RegisterWrite  NR42;
                emu::RegisterWrite  NR43;
                emu::RegisterWrite  NR44;
                emu::RegisterWrite  NR50;
                emu::RegisterWrite  NR51;
                emu::RegisterWrite  NR52;
                emu::RegisterWrite  WAVE[16];
            }                       write;
        };

        void initialize();
        void execute();
        void resetClock();
        void advanceClock(int32_t tick);
        void setDesiredTicks(int32_t tick);
        void sampleStep();
        void sequencerStep();
        void updateSample(int32_t tick);
        void updateSequencer(int32_t tick);
        void update(int32_t tick);

        uint8_t readNR10(int32_t tick, uint16_t addr);
        void writeNR10(int32_t tick, uint16_t addr, uint8_t value);
        uint8_t readNR11(int32_t tick, uint16_t addr);
        void writeNR11(int32_t tick, uint16_t addr, uint8_t value);
        uint8_t readNR12(int32_t tick, uint16_t addr);
        void writeNR12(int32_t tick, uint16_t addr, uint8_t value);
        uint8_t readNR13(int32_t tick, uint16_t addr);
        void writeNR13(int32_t tick, uint16_t addr, uint8_t value);
        uint8_t readNR14(int32_t tick, uint16_t addr);
        void writeNR14(int32_t tick, uint16_t addr, uint8_t value);
        uint8_t readNR21(int32_t tick, uint16_t addr);
        void writeNR21(int32_t tick, uint16_t addr, uint8_t value);
        uint8_t readNR22(int32_t tick, uint16_t addr);
        void writeNR22(int32_t tick, uint16_t addr, uint8_t value);
        uint8_t readNR23(int32_t tick, uint16_t addr);
        void writeNR23(int32_t tick, uint16_t addr, uint8_t value);
        uint8_t readNR24(int32_t tick, uint16_t addr);
        void writeNR24(int32_t tick, uint16_t addr, uint8_t value);
        uint8_t readNR30(int32_t tick, uint16_t addr);
        void writeNR30(int32_t tick, uint16_t addr, uint8_t value);
        uint8_t readNR31(int32_t tick, uint16_t addr);
        void writeNR31(int32_t tick, uint16_t addr, uint8_t value);
        uint8_t readNR32(int32_t tick, uint16_t addr);
        void writeNR32(int32_t tick, uint16_t addr, uint8_t value);
        uint8_t readNR33(int32_t tick, uint16_t addr);
        void writeNR33(int32_t tick, uint16_t addr, uint8_t value);
        uint8_t readNR34(int32_t tick, uint16_t addr);
        void writeNR34(int32_t tick, uint16_t addr, uint8_t value);
        uint8_t readNR41(int32_t tick, uint16_t addr);
        void writeNR41(int32_t tick, uint16_t addr, uint8_t value);
        uint8_t readNR42(int32_t tick, uint16_t addr);
        void writeNR42(int32_t tick, uint16_t addr, uint8_t value);
        uint8_t readNR43(int32_t tick, uint16_t addr);
        void writeNR43(int32_t tick, uint16_t addr, uint8_t value);
        uint8_t readNR44(int32_t tick, uint16_t addr);
        void writeNR44(int32_t tick, uint16_t addr, uint8_t value);
        uint8_t readNR50(int32_t tick, uint16_t addr);
        void writeNR50(int32_t tick, uint16_t addr, uint8_t value);
        uint8_t readNR51(int32_t tick, uint16_t addr);
        void writeNR51(int32_t tick, uint16_t addr, uint8_t value);
        uint8_t readNR52(int32_t tick, uint16_t addr);
        void writeNR52(int32_t tick, uint16_t addr, uint8_t value);
        uint8_t readWAVE(int32_t tick, uint16_t addr);
        void writeWAVE(int32_t tick, uint16_t addr, uint8_t value);

        class SquarePattern;

        class Sweep
        {
        public:
            Sweep(const uint8_t& NRx0, const uint8_t& NRx4, SquarePattern& pattern);
            void reset();
            void onWriteNRx4();
            void step();
            void serialize(emu::ISerializer& serializer);

        private:
            void reload();
            void reloadSweep();

            const uint8_t&  mNRx0;
            const uint8_t&  mNRx4;
            SquarePattern&  mPattern;
            bool            mEnabled;
            uint32_t        mPeriod;
            uint32_t        mCounter;
        };

        class Length
        {
        public:
            Length(const uint8_t& NRx1, const uint8_t& NRx4, bool is8bit);
            void reset();
            void onWriteNRx1();
            void onWriteNRx4();
            void step();
            void serialize(emu::ISerializer& serializer);

            uint32_t getOutputMask() const
            {
                return mOutputMask;
            }

        private:
            void reload();

            const uint8_t&  mNRx1;
            const uint8_t&  mNRx4;
            uint32_t        mLoadMask;
            uint32_t        mCounter;
            uint32_t        mOutputMask;
        };

        class Volume
        {
        public:
            Volume(const uint8_t& NRx2, const uint8_t& NRx4);
            void reset();
            void onWriteNRx4();
            void step();
            void serialize(emu::ISerializer& serializer);

            uint32_t getVolume() const
            {
                return mVolume;
            }

        private:
            void reload();

            const uint8_t&  mNRx2;
            const uint8_t&  mNRx4;
            uint32_t        mCounter;
            uint32_t        mVolume;
        };

        class SquarePattern
        {
        public:
            SquarePattern(const uint8_t& NRx1, const uint8_t& NRx3, const uint8_t& NRx4);
            bool initClock(int32_t clockDivider);
            void reset();
            void advanceClock(int32_t tick);
            void onWriteNRx4(int32_t tick);
            void setPeriod(int32_t period);
            int32_t getPeriod() const;
            void update(int32_t tick);
            void serialize(emu::ISerializer& serializer);
            int32_t getSample(uint32_t volume) const;

        private:
            void reload();

            const uint8_t&  mNRx1;
            const uint8_t&  mNRx3;
            const uint8_t&  mNRx4;
            int32_t         mClockShift;
            int32_t         mClockLastTick;
            int32_t         mClockStep;
            int32_t         mClockPeriodFast;
            int32_t         mClockPeriod;
            int32_t         mCycle;
        };

        class WavePattern
        {
        public:
            void reset();
            void serialize(emu::ISerializer& serializer);
        };

        class NoisePattern
        {
        public:
            void reset();
            void serialize(emu::ISerializer& serializer);
        };

        emu::Clock*             mClock;
        ClockListener           mClockListener;
        RegisterAccessors       mRegisterAccessors;
        int16_t*                mSoundBuffer;
        uint32_t                mSoundBufferSize;
        uint32_t                mSoundBufferOffset;
        uint32_t                mMasterClockFrequency;
        uint32_t                mTicksPerSample;
        uint32_t                mTicksPerSequencerStep;
        int32_t                 mDesiredTick;
        int32_t                 mUpdateTick;
        int32_t                 mSampleTick;
        int32_t                 mSequencerTick;
        uint8_t                 mSequencerStep;
        uint8_t                 mRegNR10;
        uint8_t                 mRegNR11;
        uint8_t                 mRegNR12;
        uint8_t                 mRegNR13;
        uint8_t                 mRegNR14;
        uint8_t                 mRegNR21;
        uint8_t                 mRegNR22;
        uint8_t                 mRegNR23;
        uint8_t                 mRegNR24;
        uint8_t                 mRegNR30;
        uint8_t                 mRegNR31;
        uint8_t                 mRegNR32;
        uint8_t                 mRegNR33;
        uint8_t                 mRegNR34;
        uint8_t                 mRegNR41;
        uint8_t                 mRegNR42;
        uint8_t                 mRegNR43;
        uint8_t                 mRegNR44;
        uint8_t                 mRegNR50;
        uint8_t                 mRegNR51;
        uint8_t                 mRegNR52;
        uint8_t                 mRegWAVE[16];
        Length                  mChannel1Length;
        Volume                  mChannel1Volume;
        SquarePattern           mChannel1Pattern;
        Sweep                   mChannel1Sweep;
        Length                  mChannel2Length;
        Volume                  mChannel2Volume;
        SquarePattern           mChannel2Pattern;
        Length                  mChannel3Length;
        WavePattern             mChannel3Pattern;
        Length                  mChannel4Length;
        Volume                  mChannel4Volume;
        NoisePattern            mChannel4Pattern;
    };
}
