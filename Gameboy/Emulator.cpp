#include <Core/Serializer.h>
#include <Core/Stream.h>
#include "gb.h"

namespace
{
    class EmulatorGB : public emu::IEmulator
    {
    public:
        virtual emu::Rom* loadRom(const char* path) override
        {
            return gb::Rom::load(path);
        }

        virtual void unloadRom(emu::Rom& rom) override
        {
            static_cast<gb::Rom&>(rom).dispose();
        }

        virtual emu::Context* createContext(const emu::Rom& rom) override
        {
            return gb::Context::create(static_cast<const gb::Rom&>(rom), gb::Model::GB);
        }

        virtual void destroyContext(emu::Context& context) override
        {
            static_cast<gb::Context&>(context).dispose();
        }

        virtual bool getDisplaySize(emu::Context& context, uint32_t& sizeX, uint32_t& sizeY) override
        {
            EMU_UNUSED(context);
            sizeX = gb::Context::DisplaySizeX;
            sizeY = gb::Context::DisplaySizeY;
            return true;
        }

        virtual bool serializeGameData(emu::Context& context, emu::ISerializer& serializer) override
        {
            static_cast<gb::Context&>(context).serializeGameData(serializer);
            return true;
        }

        virtual bool serializeGameState(emu::Context& context, emu::ISerializer& serializer) override
        {
            static_cast<gb::Context&>(context).serializeGameState(serializer);
            return true;
        }

        virtual bool setRenderBuffer(emu::Context& context, void* buffer, size_t pitch) override
        {
            static_cast<gb::Context&>(context).setRenderSurface(buffer, pitch);
            return true;
        }

        virtual bool setSoundBuffer(emu::Context& context, void* buffer, size_t size) override
        {
            static_cast<gb::Context&>(context).setSoundBuffer(static_cast<int16_t*>(buffer), size);
            return true;
        }

        virtual bool setController(emu::Context& context, uint32_t index, uint32_t value) override
        {
            static_cast<gb::Context&>(context).setController(index, value);
            return true;
        }

        virtual bool reset(emu::Context& context) override
        {
            static_cast<gb::Context&>(context).reset();
            return true;
        }

        virtual bool execute(emu::Context& context) override
        {
            static_cast<gb::Context&>(context).update();
            return true;
        }
    };

    class EmulatorGBC : public EmulatorGB
    {
    public:
        virtual emu::Context* createContext(const emu::Rom& rom) override
        {
            return gb::Context::create(static_cast<const gb::Rom&>(rom), gb::Model::GBC);
        }
    };
}

namespace gb
{
    emu::IEmulator& getEmulatorGB()
    {
        static EmulatorGB instance;
        return instance;
    }

    emu::IEmulator& getEmulatorGBC()
    {
        static EmulatorGBC instance;
        return instance;
    }
}