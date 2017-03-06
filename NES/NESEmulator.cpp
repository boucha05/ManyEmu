#include <Core/Path.h>
#include <Core/Serialization.h>
#include <Core/Stream.h>
#include "nes.h"
#include "NESEmulator.h"

namespace nes
{
    emu::Rom* Emulator::loadRom(const char* path)
    {
        return Rom::load(path);
    }

    void Emulator::unloadRom(emu::Rom& rom)
    {
        static_cast<Rom&>(rom).dispose();
    }

    emu::Context* Emulator::createContext(const emu::Rom& rom)
    {
        return Context::create(static_cast<const Rom&>(rom));
    }

    void Emulator::destroyContext(emu::Context& context)
    {
        static_cast<Context&>(context).dispose();
    }

    bool Emulator::getDisplaySize(emu::Context& context, uint32_t& sizeX, uint32_t& sizeY)
    {
        EMU_UNUSED(context);
        sizeX = nes::Context::DisplaySizeX;
        sizeY = nes::Context::DisplaySizeY;
        return true;
    }

    bool Emulator::serializeGameData(emu::Context& context, emu::ISerializer& serializer)
    {
        static_cast<Context&>(context).serializeGameData(serializer);
        return true;
    }

    bool Emulator::serializeGameState(emu::Context& context, emu::ISerializer& serializer)
    {
        static_cast<Context&>(context).serializeGameState(serializer);
        return true;
    }

    bool Emulator::setRenderBuffer(emu::Context& context, void* buffer, size_t pitch)
    {
        static_cast<Context&>(context).setRenderSurface(buffer, pitch);
        return true;
    }

    bool Emulator::setSoundBuffer(emu::Context& context, void* buffer, size_t size)
    {
        static_cast<Context&>(context).setSoundBuffer(static_cast<int16_t*>(buffer), size);
        return true;
    }

    bool Emulator::setController(emu::Context& context, uint32_t index, uint32_t value)
    {
        static_cast<Context&>(context).setController(index, value);
        return true;
    }

    bool Emulator::reset(emu::Context& context)
    {
        static_cast<Context&>(context).reset();
        return true;
    }

    bool Emulator::execute(emu::Context& context)
    {
        static_cast<Context&>(context).update();
        return true;
    }

    Emulator& Emulator::getInstance()
    {
        static Emulator instance;
        return instance;
    }
}