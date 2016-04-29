#include <Core/Path.h>
#include "gb.h"

namespace
{
    class EmulatorGB : public emu::IEmulator
    {
    public:
        EmulatorGB(emu::IEmulatorAPI& api)
            : mApi(api)
        {
        }

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
            return gb::Context::create(getApi(), static_cast<const gb::Rom&>(rom), gb::Model::GB);
        }

        virtual void destroyContext(emu::Context& context) override
        {
            static_cast<gb::Context&>(context).dispose();
        }

        virtual bool getDisplaySize(emu::Context& context, uint32_t& sizeX, uint32_t& sizeY) override
        {
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

        virtual uint32_t getInputDeviceCount(emu::Context& context) override
        {
            return static_cast<gb::Context&>(context).getInputDeviceCount();
        }

        virtual emu::IInputDevice* getInputDevice(emu::Context& context, uint32_t index) override
        {
            return static_cast<gb::Context&>(context).getInputDevice(index);
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

        emu::IEmulatorAPI& getApi()
        {
            return mApi;
        }

    private:
        emu::IEmulatorAPI&  mApi;
    };

    class EmulatorGBC : public EmulatorGB
    {
    public:
        EmulatorGBC(emu::IEmulatorAPI& api)
            : EmulatorGB(api)
        {
        }

        virtual emu::Context* createContext(const emu::Rom& rom) override
        {
            return gb::Context::create(getApi(), static_cast<const gb::Rom&>(rom), gb::Model::GBC);
        }
    };
}

namespace gb
{
    emu::IEmulator* createEmulatorGB(emu::IEmulatorAPI& api)
    {
        return new EmulatorGB(api);
    }

    void destroyEmulatorGB(emu::IEmulator& emulator)
    {
        delete static_cast<EmulatorGB*>(&emulator);
    }

    emu::IEmulator* createEmulatorGBC(emu::IEmulatorAPI& api)
    {
        return new EmulatorGBC(api);
    }

    void destroyEmulatorGBC(emu::IEmulator& emulator)
    {
        delete static_cast<EmulatorGBC*>(&emulator);
    }
}