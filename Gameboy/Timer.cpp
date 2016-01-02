#include <Core/RegisterBank.h>
#include <Core/Serialization.h>
#include "Timer.h"
#include "Interrupts.h"

namespace
{
    static const int32_t DIV_FREQUENCY_GB = 16384;

    static const uint8_t TAC_START = 0x04;
    static const uint8_t TAC_SELECT_MASK = 0x03;
    static const uint8_t TAC_SELECT_SHIFT = 0;
    static const uint32_t TAC_CLOCK_BASE = 262144;
    static const uint32_t TAC_CLOCK_SHIFT[4] = { 6, 0, 2, 4 };

    static const int32_t TIMER_OVERFLOW = 0x100;
}

namespace gb
{
    Timer::ClockListener::ClockListener()
    {
        initialize();
    }

    Timer::ClockListener::~ClockListener()
    {
        destroy();
    }

    void Timer::ClockListener::initialize()
    {
        mClock = nullptr;
        mTimer = nullptr;
    }

    bool Timer::ClockListener::create(emu::Clock& clock, Timer& timer)
    {
        mClock = &clock;
        mTimer = &timer;
        mClock->addListener(*this);
        return true;
    }

    void Timer::ClockListener::destroy()
    {
        if (mClock)
            mClock->removeListener(*this);
        initialize();
    }

    void Timer::ClockListener::execute()
    {
        mTimer->execute();
    }

    void Timer::ClockListener::resetClock()
    {
        mTimer->resetClock();
    }

    void Timer::ClockListener::advanceClock(int32_t tick)
    {
        mTimer->advanceClock(tick);
    }

    void Timer::ClockListener::setDesiredTicks(int32_t tick)
    {
        mTimer->setDesiredTicks(tick);
    }

    ////////////////////////////////////////////////////////////////////////////

    Timer::Timer()
    {
        initialize();
    }

    Timer::~Timer()
    {
        destroy();
    }

    void Timer::initialize()
    {
        mClock          = nullptr;
        mInterrupts     = nullptr;
        mClockFrequency = 0;
        mSimulatedTick  = 0;
        mDesiredTick    = 0;
        mDivSpeed       = 0;
        mDivTick        = 0;
        mRegDIV         = 0x00;
        mRegTIMA        = 0x00;
        mRegTMA         = 0x00;
        mRegTAC         = 0x00;
        mTimerMasterClock = 0;
        mTimerTicksPerClock = 0;
        resetTimer();
    }

    bool Timer::create(emu::Clock& clock, uint32_t clockFrequency, Interrupts& interrupts, emu::RegisterBank& registers)
    {
        mClock = &clock;
        mInterrupts = &interrupts;
        mClockFrequency = clockFrequency;
        mDivSpeed = mClockFrequency / DIV_FREQUENCY_GB;
        mTimerMasterClock = clockFrequency;
        mTimerTicksPerClock = clockFrequency / TAC_CLOCK_BASE;

        EMU_VERIFY(mClockListener.create(clock, *this));

        EMU_VERIFY(mRegisterAccessors.read.DIV.create(registers, 0x04, *this, &Timer::readDIV));
        EMU_VERIFY(mRegisterAccessors.read.TIMA.create(registers, 0x05, *this, &Timer::readTIMA));
        EMU_VERIFY(mRegisterAccessors.read.TMA.create(registers, 0x06, *this, &Timer::readTMA));
        EMU_VERIFY(mRegisterAccessors.read.TAC.create(registers, 0x07, *this, &Timer::readTAC));

        EMU_VERIFY(mRegisterAccessors.write.DIV.create(registers, 0x04, *this, &Timer::writeDIV));
        EMU_VERIFY(mRegisterAccessors.write.TIMA.create(registers, 0x05, *this, &Timer::writeTIMA));
        EMU_VERIFY(mRegisterAccessors.write.TMA.create(registers, 0x06, *this, &Timer::writeTMA));
        EMU_VERIFY(mRegisterAccessors.write.TAC.create(registers, 0x07, *this, &Timer::writeTAC));

        return true;
    }

    void Timer::destroy()
    {
        mClockListener.destroy();
        mRegisterAccessors = RegisterAccessors();
        initialize();
    }

    void Timer::reset()
    {
        mRegDIV         = 0x00;
        mRegTIMA        = 0x00;
        mRegTMA         = 0x00;
        mRegTAC         = 0x00;
        resetTimer();
    }

    void Timer::beginFrame()
    {
        updateTimerValue(mSimulatedTick);
        updateTimerPrediction();
    }

    void Timer::execute()
    {
        if (mSimulatedTick < mDesiredTick)
        {
            mSimulatedTick = mDesiredTick;
            updateTimerInterrupt(mSimulatedTick);
        }
    }

    void Timer::resetClock()
    {
        mSimulatedTick = 0;
        mDesiredTick = 0;
        mDivTick = 0;
    }

    void Timer::advanceClock(int32_t tick)
    {
        mSimulatedTick -= tick;
        mDesiredTick -= tick;
        mDivTick -= tick;
        mTimerLastClockTick -= tick;
        mTimerTick -= tick;
        mTimerIntTick -= tick;
        mTimerLastIntTick -= tick;
    }

    void Timer::setDesiredTicks(int32_t tick)
    {
        mDesiredTick = tick;
    }

    void Timer::advanceDIV(int32_t tick)
    {
        // TODO: Fast forward if gap is too big
        while (mDivTick + mDivSpeed <= tick)
        {
            mDivTick += mDivSpeed;
            ++mRegDIV;
        }
    }

    void Timer::resetTimer()
    {
        mTimerLastClockTick = 0;
        mTimerTick = 0;
        mTimerClock = 0;
        mTimerIntTick = 0;
        mTimerLastIntTick = 0;
    }

    void Timer::updateTimerValue(int32_t tick)
    {
        if (tick < mTimerTick)
            return;
        EMU_ASSERT(mTimerTick <= tick);
        mTimerTick = tick;

        bool timerEnabled = (mRegTAC & TAC_START) != 0;

        // Advance clock
        int32_t deltaClockTick = (tick - mTimerLastClockTick);
        int32_t deltaClock = 0;
        if (deltaClockTick >= mTimerTicksPerClock)
        {
            deltaClock = deltaClockTick / mTimerTicksPerClock;
            int32_t modClockTicks = deltaClockTick % mTimerTicksPerClock;
            mTimerLastClockTick = tick - modClockTicks;
        }

        // Advance timer
        if (timerEnabled)
        {
            mTimerClock += deltaClock;

            auto shift = TAC_CLOCK_SHIFT[(mRegTAC & TAC_SELECT_MASK) >> TAC_SELECT_SHIFT];
            auto divClock = mTimerClock >> shift;
            auto modClock = mTimerClock & ((1 << shift) - 1);
            mTimerClock = modClock;

            if (divClock)
            {
                uint32_t timer = divClock + mRegTIMA;
                while (timer >= TIMER_OVERFLOW)
                    timer -= TIMER_OVERFLOW - mRegTMA;
                mRegTIMA = timer;
            }
        }
    }

    void Timer::updateTimerSync()
    {
        if ((mRegTAC & TAC_START) == 0)
            return;

        mClock->addSync(mTimerIntTick);
    }

    void Timer::updateTimerPrediction()
    {
        if ((mRegTAC & TAC_START) == 0)
            return;

        auto shift = TAC_CLOCK_SHIFT[(mRegTAC & TAC_SELECT_MASK) >> TAC_SELECT_SHIFT];
        auto missingTimer = TIMER_OVERFLOW - mRegTIMA;
        auto missingClock = (missingTimer << shift) - mTimerClock;
        auto missingTicks = missingClock * mTimerTicksPerClock;
        mTimerIntTick = mTimerLastClockTick + missingTicks;

        updateTimerSync();
    }

    void Timer::updateTimerInterrupt(int32_t tick)
    {
        if ((mRegTAC & TAC_START) == 0)
            return;

        bool triggered = false;
        while (mTimerIntTick <= tick)
        {
            triggered = true;
            updateTimerValue(mTimerIntTick);
            updateTimerPrediction();
        }

        if (triggered)
        {
            mInterrupts->setInterrupt(tick, gb::Interrupts::Signal::Timer);
            auto delta = tick - mTimerLastIntTick;
            mTimerLastIntTick = tick;
        }
    }

    uint8_t Timer::readDIV(int32_t tick, uint16_t addr)
    {
        advanceDIV(tick);
        return mRegDIV;
    }

    void Timer::writeDIV(int32_t tick, uint16_t addr, uint8_t value)
    {
        advanceDIV(tick);
        mRegDIV = 0;
    }

    uint8_t Timer::readTIMA(int32_t tick, uint16_t addr)
    {
        updateTimerValue(tick);
        return mRegTIMA;
    }

    void Timer::writeTIMA(int32_t tick, uint16_t addr, uint8_t value)
    {
        updateTimerValue(tick);
        mRegTIMA = value;
        updateTimerPrediction();
    }

    uint8_t Timer::readTMA(int32_t tick, uint16_t addr)
    {
        return mRegTMA;
    }

    void Timer::writeTMA(int32_t tick, uint16_t addr, uint8_t value)
    {
        if (mRegTMA != value)
        {
            updateTimerValue(tick);
            mRegTMA = value;
            updateTimerPrediction();
        }
    }

    uint8_t Timer::readTAC(int32_t tick, uint16_t addr)
    {
        return mRegTAC;
    }

    void Timer::writeTAC(int32_t tick, uint16_t addr, uint8_t value)
    {
        if (mRegTAC != value)
        {
            updateTimerValue(tick);
            mRegTAC = value;
            updateTimerPrediction();
        }
    }

    void Timer::serialize(emu::ISerializer& serializer)
    {
        serializer.serialize(mSimulatedTick);
        serializer.serialize(mDesiredTick);
        serializer.serialize(mDivSpeed);
        serializer.serialize(mDivTick);
        serializer.serialize(mRegDIV);
        serializer.serialize(mRegTIMA);
        serializer.serialize(mRegTMA);
        serializer.serialize(mRegTAC);
        serializer.serialize(mTimerLastClockTick);
        serializer.serialize(mTimerTick);
        serializer.serialize(mTimerClock);
        serializer.serialize(mTimerIntTick);
        serializer.serialize(mTimerLastIntTick);
    }
}
