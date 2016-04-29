#pragma once

#include <Core/Clock.h>
#include <Core/Core.h>
#include <Core/RegisterBank.h>
#include <Emulator/Component.h>
#include <Emulator/InputDevice.h>
#include "GB.h"

namespace gb
{
    class Interrupts;

    class Joypad : public emu::IInputDevice::IListener
    {
    public:
        Joypad();
        ~Joypad();
        bool create(emu::IEmulatorAPI& api, emu::Clock& clock, Interrupts& interrupts, emu::RegisterBank& registers);
        void destroy();
        void reset();
        void serialize(emu::ISerializer& serializer);
        emu::IInputDevice& getInputDevice();
        virtual void onButtonSet(uint32_t index, bool value) override;

    private:
        class ClockListener : public emu::Clock::IListener
        {
        public:
            ClockListener();
            ~ClockListener();
            void initialize();
            bool create(emu::Clock& clock, Joypad& joypad);
            void destroy();
            virtual void advanceClock(int32_t tick) override;

        private:
            emu::Clock*     mClock;
            Joypad*         mJoypad;
        };

        struct RegisterAccessors
        {
            struct
            {
                emu::RegisterRead   JOYP;
            }                       read;

            struct
            {
                emu::RegisterWrite  JOYP;
            }                       write;
        };

        void initialize();
        void advanceClock(int32_t tick);

        uint8_t readJOYP(int32_t tick, uint16_t addr);
        void writeJOYP(int32_t tick, uint16_t addr, uint8_t value);

        emu::IEmulatorAPI*      mApi;
        emu::IComponent*        mComponent;
        emu::IInputDevice*      mInputDevice;
        emu::Clock*             mClock;
        Interrupts*             mInterrupts;
        ClockListener           mClockListener;
        RegisterAccessors       mRegisterAccessors;
        uint8_t                 mButtons;
        uint8_t                 mRegJOYP;
    };
}
