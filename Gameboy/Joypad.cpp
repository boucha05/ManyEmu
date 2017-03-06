#include <Core/RegisterBank.h>
#include <Core/Serialization.h>
#include "Joypad.h"
#include "Interrupts.h"

namespace
{
    static const uint8_t JOYP_BUTTON_KEYS = 0x20;
    static const uint8_t JOYP_DIRECTION_KEYS = 0x10;
    static const uint8_t JOYP_DOWN = 0x08;
    static const uint8_t JOYP_UP = 0x04;
    static const uint8_t JOYP_LEFT = 0x02;
    static const uint8_t JOYP_RIGHT = 0x01;
    static const uint8_t JOYP_START = 0x08;
    static const uint8_t JOYP_SELECT = 0x04;
    static const uint8_t JOYP_A = 0x02;
    static const uint8_t JOYP_B = 0x01;
    static const uint8_t JOYP_ALL_RELEASED = 0x0f;
}

namespace gb
{
    Joypad::ClockListener::ClockListener()
    {
        initialize();
    }

    Joypad::ClockListener::~ClockListener()
    {
        destroy();
    }

    void Joypad::ClockListener::initialize()
    {
        mClock = nullptr;
        mJoypad = nullptr;
    }

    bool Joypad::ClockListener::create(emu::Clock& clock, Joypad& joypad)
    {
        mClock = &clock;
        mJoypad = &joypad;
        mClock->addListener(*this);
        return true;
    }

    void Joypad::ClockListener::destroy()
    {
        if (mClock)
            mClock->removeListener(*this);
        initialize();
    }

    void Joypad::ClockListener::advanceClock(int32_t tick)
    {
        mJoypad->advanceClock(tick);
    }

    ////////////////////////////////////////////////////////////////////////////

    Joypad::Joypad()
    {
        initialize();
    }

    Joypad::~Joypad()
    {
        destroy();
    }

    void Joypad::initialize()
    {
        mClock                  = nullptr;
        mInterrupts             = nullptr;
        mButtons                = 0;
        mRegJOYP                = JOYP_ALL_RELEASED;
    }

    bool Joypad::create(emu::Clock& clock, Interrupts& interrupts, emu::RegisterBank& registers)
    {
        mClock = &clock;
        mInterrupts = &interrupts;

        EMU_VERIFY(mClockListener.create(clock, *this));

        EMU_VERIFY(mRegisterAccessors.read.JOYP.create(registers, 0x00, *this, &Joypad::readJOYP));
        EMU_VERIFY(mRegisterAccessors.write.JOYP.create(registers, 0x00, *this, &Joypad::writeJOYP));

        return true;
    }

    void Joypad::destroy()
    {
        mClockListener.destroy();
        mRegisterAccessors = RegisterAccessors();
        initialize();
    }

    void Joypad::reset()
    {
        mRegJOYP = JOYP_ALL_RELEASED;
    }

    void Joypad::advanceClock(int32_t tick)
    {
        if (mButtons & gb::Context::ButtonAll)
            mInterrupts->setInterrupt(tick, gb::Interrupts::Signal::Joypad);
    }

    uint8_t Joypad::readJOYP(int32_t tick, uint16_t addr)
    {
        EMU_UNUSED(tick);
        EMU_UNUSED(addr);
        return mRegJOYP;
    }

    void Joypad::writeJOYP(int32_t tick, uint16_t addr, uint8_t value)
    {
        EMU_UNUSED(tick);
        EMU_UNUSED(addr);
        uint8_t nextJOYP = JOYP_ALL_RELEASED;
        if ((value & JOYP_BUTTON_KEYS) == 0)
        {
            if (mButtons & gb::Context::ButtonStart)
                nextJOYP &= ~JOYP_START;
            if (mButtons & gb::Context::ButtonSelect)
                nextJOYP &= ~JOYP_SELECT;
            if (mButtons & gb::Context::ButtonB)
                nextJOYP &= ~JOYP_B;
            if (mButtons & gb::Context::ButtonA)
                nextJOYP &= ~JOYP_A;
        }
        if ((value & JOYP_DIRECTION_KEYS) == 0)
        {
            if (mButtons & gb::Context::ButtonDown)
                nextJOYP &= ~JOYP_DOWN;
            if (mButtons & gb::Context::ButtonUp)
                nextJOYP &= ~JOYP_UP;
            if (mButtons & gb::Context::ButtonLeft)
                nextJOYP &= ~JOYP_LEFT;
            if (mButtons & gb::Context::ButtonRight)
                nextJOYP &= ~JOYP_RIGHT;
        }
        mRegJOYP = (value & ~JOYP_ALL_RELEASED) | nextJOYP;
    }

    void Joypad::serialize(emu::ISerializer& serializer)
    {
        serializer.serialize(mButtons);
        serializer.serialize(mRegJOYP);
    }

    void Joypad::setController(uint32_t index, uint32_t buttons)
    {
        EMU_UNUSED(index);
        mButtons = buttons & gb::Context::ButtonAll;
    }
}
