#include <Core/RegisterBank.h>
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
        mApi            = nullptr;
        mComponent      = nullptr;
        mInputDevice    = nullptr;
        mClock          = nullptr;
        mInterrupts     = nullptr;
        mButtons        = 0;
        mRegJOYP        = JOYP_ALL_RELEASED;
    }

    bool Joypad::create(emu::IEmulatorAPI& api, emu::Clock& clock, Interrupts& interrupts, emu::RegisterBank& registers)
    {
        mApi = &api;

        auto componentType = mApi->getComponentManager().getComponentType(emu::InputDevice::ComponentID);
        EMU_VERIFY(componentType);
        mComponent = componentType->createComponent();
        EMU_VERIFY(mComponent);
        mInputDevice = static_cast<emu::IInputDevice*>(componentType->getInterface(*mComponent, emu::IInputDevice::getID()));
        EMU_VERIFY(mInputDevice);

        mClock = &clock;
        mInterrupts = &interrupts;

        EMU_VERIFY(mClockListener.create(clock, *this));

        EMU_VERIFY(mRegisterAccessors.read.JOYP.create(registers, 0x00, *this, &Joypad::readJOYP));
        EMU_VERIFY(mRegisterAccessors.write.JOYP.create(registers, 0x00, *this, &Joypad::writeJOYP));

        auto mapping = static_cast<emu::IInputDeviceMapping*>(componentType->getInterface(*mComponent, emu::IInputDeviceMapping::getID()));
        EMU_VERIFY(mapping);

        EMU_VERIFY(mapping->addButton("A") == static_cast<uint32_t>(gb::Context::Button::A));
        EMU_VERIFY(mapping->addButton("B") == static_cast<uint32_t>(gb::Context::Button::B));
        EMU_VERIFY(mapping->addButton("Select") == static_cast<uint32_t>(gb::Context::Button::Select));
        EMU_VERIFY(mapping->addButton("Start") == static_cast<uint32_t>(gb::Context::Button::Start));
        EMU_VERIFY(mapping->addButton("Up") == static_cast<uint32_t>(gb::Context::Button::Up));
        EMU_VERIFY(mapping->addButton("Down") == static_cast<uint32_t>(gb::Context::Button::Down));
        EMU_VERIFY(mapping->addButton("Left") == static_cast<uint32_t>(gb::Context::Button::Left));
        EMU_VERIFY(mapping->addButton("Right") == static_cast<uint32_t>(gb::Context::Button::Right));
        mInputDevice->addListener(*this);

        return true;
    }

    void Joypad::destroy()
    {
        if (mInputDevice) mInputDevice->removeListener(*this);

        mClockListener.destroy();
        mRegisterAccessors = RegisterAccessors();

        if (mApi)
        {
            auto componentType = mApi->getComponentManager().getComponentType(emu::InputDevice::ComponentID);
            if (componentType)
            {
                if (mComponent) componentType->destroyComponent(*mComponent);
            }
        }

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
        return mRegJOYP;
    }

    void Joypad::writeJOYP(int32_t tick, uint16_t addr, uint8_t value)
    {
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

    emu::IInputDevice& Joypad::getInputDevice()
    {
        return *mInputDevice;
    }

    void Joypad::onButtonSet(uint32_t index, bool value)
    {
        uint32_t mask = 1 << index;
        if (value)
            mButtons |= mask;
        else
            mButtons &= ~mask;
    }
}
