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
    static const uint32_t TAC_CLOCK[4] = { 4096, 262144, 65536, 16384 };
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
    }

    bool Timer::create(emu::Clock& clock, uint32_t clockFrequency, Interrupts& interrupts, emu::RegisterBank& registers)
    {
        mClock = &clock;
        mInterrupts = &interrupts;
        mClockFrequency = clockFrequency;
        mDivSpeed = mClockFrequency / DIV_FREQUENCY_GB;

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
        mSimulatedTick  = 0;
        mDesiredTick    = 0;
        mDivTick        = 0;
        mRegDIV         = 0x00;
        mRegTIMA        = 0x00;
        mRegTMA         = 0x00;
        mRegTAC         = 0x00;
    }

    void Timer::execute()
    {
    }

    void Timer::resetClock()
    {
    }

    void Timer::advanceClock(int32_t tick)
    {
        mSimulatedTick -= tick;
        mDesiredTick -= tick;
        mDivTick -= tick;
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
        EMU_NOT_IMPLEMENTED();
        return mRegTIMA;
    }

    void Timer::writeTIMA(int32_t tick, uint16_t addr, uint8_t value)
    {
        EMU_NOT_IMPLEMENTED();
        mRegTIMA = value;
    }

    uint8_t Timer::readTMA(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegTMA;
    }

    void Timer::writeTMA(int32_t tick, uint16_t addr, uint8_t value)
    {
        EMU_NOT_IMPLEMENTED();
        mRegTMA = value;
    }

    uint8_t Timer::readTAC(int32_t tick, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return mRegTAC;
    }

    void Timer::writeTAC(int32_t tick, uint16_t addr, uint8_t value)
    {
        EMU_NOT_IMPLEMENTED();
        mRegTAC = value;
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
    }
}
