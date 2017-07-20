#include <Core/Serializer.h>
#include <Core/Stream.h>
#include "nes.h"
#include "NESEmulator.h"

namespace nes
{
    class EmulatorImpl : public Emulator
    {
    public:
        bool getSystemInfo(SystemInfo& info) override
        {
            info.name = "NES";
            info.extensions = "nes";
            return true;
        }

        emu::IRom* loadRom(const char* path) override
        {
            return Rom::load(path);
        }

        void unloadRom(emu::IRom& rom) override
        {
            static_cast<Rom&>(rom).dispose();
        }

        emu::IContext* createContext(const emu::IRom& rom) override
        {
            return Context::create(static_cast<const Rom&>(rom));
        }

        void destroyContext(emu::IContext& context) override
        {
            static_cast<Context&>(context).dispose();
        }
    };

    Emulator& Emulator::getInstance()
    {
        static EmulatorImpl instance;
        return instance;
    }
}