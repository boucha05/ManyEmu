#pragma once
#include "Component.h"
#include "Emulator.h"

namespace emu
{
    class IComponentManager;
    class IInputDevice;

    namespace impl
    {
        IComponentManager* createComponentManager();
        void destroyComponentManager(IComponentManager& instance);

        bool registerInputDevice(emu::IEmulatorAPI& api);
        void unregisterInputDevice(emu::IEmulatorAPI& api);
    }
}