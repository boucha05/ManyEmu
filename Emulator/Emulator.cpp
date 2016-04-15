#include "Emulator.h"
#include <Core/Core.h>
#include <Gameboy/GB.h>
#include <NES/NESEmulator.h>
#include <map>

namespace
{
    using namespace emu;

    typedef std::map<std::string, IEmulator&> EmulatorMap;

    EmulatorMap& getEmulators()
    {
        static EmulatorMap emulators =
        {
            { "gb", gb::getEmulatorGB() },
            { "gbc", gb::getEmulatorGBC() },
            { "nes", nes::Emulator::getInstance() },
        };
        return emulators;
    }

    class EmulatorAPI : public IEmulatorAPI
    {
    public:
        virtual void traverseEmulators(IEmulatorVisitor& visitor) override
        {
            (void)visitor;
        }

        virtual IEmulator* getEmulator(const char* name) override
        {
            const auto& emulators = getEmulators();
            auto item = emulators.find(name);
            if (item != emulators.end())
            {
                return &item->second;
            }
            else
            {
                return nullptr;
            }
        }
    };
}

emu::IEmulatorAPI* emuCreateAPI()
{
    return new EmulatorAPI();
}

void emuDestroyAPI(emu::IEmulatorAPI& api)
{
    delete &api;
}
