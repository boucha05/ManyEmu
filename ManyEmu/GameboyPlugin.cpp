#include "Backend.h"
#include "GameboyPlugin.h"
#include "InputManager.h"
#include <SDL.h>
#include <string>
#include <vector>
#include <deque>
#include <Core/InputController.h>
#include <Core/Serializer.h>
#include <Core/Stream.h>
#include <Gameboy/GB.h>

namespace
{
    class GBBackend : public IBackend
    {
    public:
        virtual emu::IEmulator& getEmulator() override
        {
            return gb::Emulator::getEmulatorGB();
        }

        virtual void configureController(StandardController& controller, uint32_t player)
        {
            EMU_UNUSED(player);
            controller.addInput(Input_Player1 + Input_HorizontalAxis, gb::Context::ButtonRight, gb::Context::ButtonLeft);
            controller.addInput(Input_Player1 + Input_VerticalAxis, gb::Context::ButtonDown, gb::Context::ButtonUp);
            controller.addInput(Input_Player1 + Input_B, gb::Context::ButtonB);
            controller.addInput(Input_Player1 + Input_A, gb::Context::ButtonA);
            controller.addInput(Input_Player1 + Input_Select, gb::Context::ButtonSelect);
            controller.addInput(Input_Player1 + Input_Start, gb::Context::ButtonStart);
        }
    };

    class GBCBackend : public GBBackend
    {
    public:
        virtual emu::IEmulator& getEmulator() override
        {
            return gb::Emulator::getEmulatorGBC();
        }
    };

    class SGBBackend : public GBBackend
    {
    public:
        virtual emu::IEmulator& getEmulator() override
        {
            return gb::Emulator::getEmulatorSGB();
        }
    };

    class GameboyPluginImpl : public GameboyPlugin
    {
    public:
        virtual bool create(Application& application) override
        {
            EMU_VERIFY(application.getBackendRegistry().add(mGBBackend));
            EMU_VERIFY(application.getBackendRegistry().add(mGBCBackend));
            EMU_VERIFY(application.getBackendRegistry().add(mSGBBackend));
            return true;
        }

        virtual void destroy(Application& application) override
        {
            application.getBackendRegistry().remove(mSGBBackend);
            application.getBackendRegistry().remove(mGBCBackend);
            application.getBackendRegistry().remove(mGBBackend);
        }

        virtual const char* getName() const override
        {
            return "Gameboy";
        }

    private:
        GBBackend   mGBBackend;
        GBCBackend  mGBCBackend;
        SGBBackend  mSGBBackend;
    };
}

GameboyPlugin* GameboyPlugin::create()
{
    return new GameboyPluginImpl();
}

void GameboyPlugin::destroy(GameboyPlugin& instance)
{
    delete &static_cast<GameboyPluginImpl&>(instance);
}
