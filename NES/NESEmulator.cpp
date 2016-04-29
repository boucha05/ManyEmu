#include <Core/Path.h>
#include "nes.h"
#include "NESEmulator.h"

namespace nes
{
    Emulator::Emulator(emu::IEmulatorAPI& api)
        : mApi(api)
    {
    }

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
        return Context::create(getApi(), static_cast<const Rom&>(rom));
    }

    void Emulator::destroyContext(emu::Context& context)
    {
        static_cast<Context&>(context).dispose();
    }

    bool Emulator::getDisplaySize(emu::Context& context, uint32_t& sizeX, uint32_t& sizeY)
    {
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

    uint32_t Emulator::getInputDeviceCount(emu::Context& context)
    {
        return static_cast<Context&>(context).getInputDeviceCount();
    }

    emu::IInputDevice* Emulator::getInputDevice(emu::Context& context, uint32_t index)
    {
        return static_cast<Context&>(context).getInputDevice(index);
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

    emu::IEmulator* Emulator::createInstance(emu::IEmulatorAPI& api)
    {
        return new Emulator(api);
    }

    void Emulator::destroyInstance(emu::IEmulator& instance)
    {
        delete static_cast<Emulator*>(&instance);
    }
}