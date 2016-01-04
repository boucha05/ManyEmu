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
        bool create(emu::Clock& clock, uint32_t master_clock_divider, emu::RegisterBank& registers);
        void destroy();
        void reset();
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

        emu::Clock*             mClock;
        ClockListener           mClockListener;
        RegisterAccessors       mRegisterAccessors;
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
    };
}
