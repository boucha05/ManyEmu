#pragma once

#include <Core/Clock.h>
#include <Core/Core.h>
#include <Core/RegisterBank.h>
#include "GB.h"

namespace gb
{
    class Interrupts;

    class Timer
    {
    public:
        Timer();
        ~Timer();
        bool create(emu::Clock& clock, uint32_t clockFrequency, uint32_t fixedClockDivider, uint32_t variableClockDivider, Interrupts& interrupts, emu::RegisterBank& registers);
        void destroy();
        void reset();
        void setVariableClockDivider(uint32_t variableClockDivider);
        void beginFrame();
        void serialize(emu::ISerializer& serializer);

    private:
        class ClockListener : public emu::Clock::IListener
        {
        public:
            ClockListener();
            ~ClockListener();
            void initialize();
            bool create(emu::Clock& clock, Timer& timer);
            void destroy();
            virtual void execute() override;
            virtual void resetClock() override;
            virtual void advanceClock(int32_t tick) override;
            virtual void setDesiredTicks(int32_t tick) override;

        private:
            emu::Clock*     mClock;
            Timer*          mTimer;
        };

        struct RegisterAccessors
        {
            struct
            {
                emu::RegisterRead   DIV;
                emu::RegisterRead   TIMA;
                emu::RegisterRead   TMA;
                emu::RegisterRead   TAC;
            }                       read;

            struct
            {
                emu::RegisterWrite  DIV;
                emu::RegisterWrite  TIMA;
                emu::RegisterWrite  TMA;
                emu::RegisterWrite  TAC;
            }                       write;
        };

        void initialize();
        void execute();
        void resetClock();
        void advanceClock(int32_t tick);
        void setDesiredTicks(int32_t tick);
        void scheduleNextEvent(int32_t tick);
        void advanceDIV(int32_t tick);
        void resetTimer();
        void updateTimerValue(int32_t tick);
        void updateTimerSync();
        void updateTimerPrediction();
        void updateTimerInterrupt(int32_t tick);

        uint8_t readDIV(int32_t tick, uint16_t addr);
        void writeDIV(int32_t tick, uint16_t addr, uint8_t value);
        uint8_t readTIMA(int32_t tick, uint16_t addr);
        void writeTIMA(int32_t tick, uint16_t addr, uint8_t value);
        uint8_t readTMA(int32_t tick, uint16_t addr);
        void writeTMA(int32_t tick, uint16_t addr, uint8_t value);
        uint8_t readTAC(int32_t tick, uint16_t addr);
        void writeTAC(int32_t tick, uint16_t addr, uint8_t value);

        emu::Clock*             mClock;
        Interrupts*             mInterrupts;
        ClockListener           mClockListener;
        RegisterAccessors       mRegisterAccessors;
        uint32_t                mClockFrequency;
        int32_t                 mSimulatedTick;
        int32_t                 mDesiredTick;
        int32_t                 mDivSpeed;
        int32_t                 mDivTick;
        uint8_t                 mRegDIV;
        uint8_t                 mRegTIMA;
        uint8_t                 mRegTMA;
        uint8_t                 mRegTAC;
        int32_t                 mTimerMasterClock;
        int32_t                 mTimerTicksPerClock;
        int32_t                 mTimerLastClockTick;
        int32_t                 mTimerTick;
        int32_t                 mTimerClock;
        int32_t                 mTimerIntTick;
        int32_t                 mTimerLastIntTick;
    };
}
