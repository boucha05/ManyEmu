#include "EmulatorImpl.h"
#include <Core/Core.h>
#include <Gameboy/GB.h>
#include <NES/NESEmulator.h>
#include <map>

namespace
{
    using namespace emu;
    using namespace emu::impl;

    class EmulatorAPI : public IEmulatorAPI
    {
    public:
        EmulatorAPI()
        {
            initialize();
        }

        ~EmulatorAPI()
        {
            destroy();
        }

        void initialize()
        {
            mComponentManager = nullptr;
        }

        bool create()
        {
            mComponentManager = createComponentManager();
            EMU_VERIFY(mComponentManager);
            registerInputDevice(*this);
            createEmulators();
            return true;
        }

        void destroy()
        {
            destroyEmulators();
            if (mComponentManager)
            {
                unregisterInputDevice(*this);
                destroyComponentManager(*mComponentManager);
            }
            initialize();
        }

        virtual void traverseEmulators(IEmulatorVisitor& visitor) override
        {
            (void)visitor;
        }

        virtual IEmulator* getEmulator(const char* name) override
        {
            auto item = mEmulators.find(name);
            if (item != mEmulators.end())
            {
                return item->second;
            }
            else
            {
                return nullptr;
            }
        }

        virtual IComponentManager& getComponentManager() override
        {
            return *mComponentManager;
        }

    private:
        typedef std::map<std::string, IEmulator*> EmulatorMap;

        void createEmulators()
        {
            mEmulators =
            {
                { "gb", gb::createEmulatorGB(*this) },
                { "gbc", gb::createEmulatorGBC(*this) },
                { "nes", nes::Emulator::createInstance(*this) },
            };
        }

        void destroyEmulators()
        {
            gb::destroyEmulatorGB(*mEmulators["gb"]);
            gb::destroyEmulatorGBC(*mEmulators["gbc"]);
            nes::Emulator::destroyInstance(*mEmulators["nes"]);
        }

        IComponentManager*  mComponentManager;
        EmulatorMap         mEmulators;
    };
}

emu::IEmulatorAPI* emuCreateAPI()
{
    auto instance = new EmulatorAPI();
    if (!instance->create())
    {
        delete instance;
        instance = nullptr;
    }
    return instance;
}

void emuDestroyAPI(emu::IEmulatorAPI& api)
{
    delete static_cast<EmulatorAPI*>(&api);
}
