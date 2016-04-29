#pragma once

#include <stdint.h>

namespace emu
{
    class IInputDevice
    {
    public:
        static const char* getID()
        {
            return "IInputDevice";
        }

        class IListener
        {
        public:
            virtual void onButtonSet(uint32_t index, bool value) {};
        };

        virtual void addListener(IListener& listener) = 0;
        virtual void removeListener(IListener& listener) = 0;
        virtual uint32_t getButtonCount() = 0;
        virtual const char* getButtonName(uint32_t index) = 0;
        virtual uint32_t getButtonIndex(const char* name) = 0;
        virtual void setButton(uint32_t index, bool value) = 0;
        virtual bool getButton(uint32_t index) = 0;
    };

    class IInputDeviceMapping
    {
    public:
        static const char* getID()
        {
            return "IInputDeviceMapping";
        }

        virtual uint32_t addButton(const char* name) = 0;
    };

    namespace InputDevice
    {
        static const char* ComponentID = "IInputDevice";
    }
}
