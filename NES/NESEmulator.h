#ifndef __NES_EMULATOR_H__
#define __NES_EMULATOR_H__

#include <stdint.h>
#include <string>
#include <Core/Core.h>
#include <Emulator/Emulator.h>

namespace nes
{
    class Emulator : public emu::IEmulator
    {
    public:
        Emulator(emu::IEmulatorAPI& api);
        virtual emu::Rom* loadRom(const char* path) override;
        virtual void unloadRom(emu::Rom& rom) override;
        virtual emu::Context* createContext(const emu::Rom& rom) override;
        virtual void destroyContext(emu::Context& context) override;
        virtual bool getDisplaySize(emu::Context& context, uint32_t& sizeX, uint32_t& sizeY) override;
        virtual bool serializeGameData(emu::Context& context, emu::ISerializer& serializer) override;
        virtual bool serializeGameState(emu::Context& context, emu::ISerializer& serializer) override;
        virtual bool setRenderBuffer(emu::Context& context, void* buffer, size_t pitch) override;
        virtual bool setSoundBuffer(emu::Context& context, void* buffer, size_t size) override;
        virtual uint32_t getInputDeviceCount(emu::Context& context) override;
        virtual emu::IInputDevice* getInputDevice(emu::Context& context, uint32_t index) override;
        virtual bool reset(emu::Context& context) override;
        virtual bool execute(emu::Context& context) override;

        emu::IEmulatorAPI& getApi()
        {
            return mApi;
        }

        static emu::IEmulator* createInstance(emu::IEmulatorAPI& api);
        static void destroyInstance(emu::IEmulator& instance);

    private:
        emu::IEmulatorAPI&  mApi;
    };
}

#endif
