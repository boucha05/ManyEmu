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

        emu::Rom* loadRom(const char* path) override
        {
            return Rom::load(path);
        }

        void unloadRom(emu::Rom& rom) override
        {
            static_cast<Rom&>(rom).dispose();
        }

        emu::Context* createContext(const emu::Rom& rom) override
        {
            return Context::create(static_cast<const Rom&>(rom));
        }

        void destroyContext(emu::Context& context) override
        {
            static_cast<Context&>(context).dispose();
        }

        bool getDisplaySize(emu::Context& context, uint32_t& sizeX, uint32_t& sizeY) override
        {
            EMU_UNUSED(context);
            sizeX = nes::Context::DisplaySizeX;
            sizeY = nes::Context::DisplaySizeY;
            return true;
        }

        bool serializeGameData(emu::Context& context, emu::ISerializer& serializer) override
        {
            static_cast<Context&>(context).serializeGameData(serializer);
            return true;
        }

        bool serializeGameState(emu::Context& context, emu::ISerializer& serializer) override
        {
            static_cast<Context&>(context).serializeGameState(serializer);
            return true;
        }

        bool setRenderBuffer(emu::Context& context, void* buffer, size_t pitch) override
        {
            static_cast<Context&>(context).setRenderSurface(buffer, pitch);
            return true;
        }

        bool setSoundBuffer(emu::Context& context, void* buffer, size_t size) override
        {
            static_cast<Context&>(context).setSoundBuffer(static_cast<int16_t*>(buffer), size);
            return true;
        }

        bool setController(emu::Context& context, uint32_t index, uint32_t value) override
        {
            static_cast<Context&>(context).setController(index, value);
            return true;
        }

        bool reset(emu::Context& context) override
        {
            static_cast<Context&>(context).reset();
            return true;
        }

        bool execute(emu::Context& context) override
        {
            static_cast<Context&>(context).update();
            return true;
        }
    };

    Emulator& Emulator::getInstance()
    {
        static EmulatorImpl instance;
        return instance;
    }
}