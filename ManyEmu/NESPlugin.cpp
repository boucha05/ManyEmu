#include "Backend.h"
#include "InputManager.h"
#include "NESPlugin.h"
#include <SDL.h>
#include <string>
#include <vector>
#include <deque>
#include <Core/InputController.h>
#include <Core/Serializer.h>
#include <Core/Stream.h>
#include <NES/NESEmulator.h>
#include <NES/Tests.h>
#include <NES/nes.h>

namespace
{
    class NESBackend : public IBackend
    {
    public:
        virtual emu::IEmulator& getEmulator() override
        {
            return nes::Emulator::getInstance();
        }

        virtual void configureController(StandardController& controller, uint32_t player)
        {
            EMU_UNUSED(player);
            controller.addInput(Input_Player1 + Input_HorizontalAxis, nes::Context::ButtonRight, nes::Context::ButtonLeft);
            controller.addInput(Input_Player1 + Input_VerticalAxis, nes::Context::ButtonDown, nes::Context::ButtonUp);
            controller.addInput(Input_Player1 + Input_B, nes::Context::ButtonB);
            controller.addInput(Input_Player1 + Input_A, nes::Context::ButtonA);
            controller.addInput(Input_Player1 + Input_Select, nes::Context::ButtonSelect);
            controller.addInput(Input_Player1 + Input_Start, nes::Context::ButtonStart);
        }
    };

    class NESPluginImpl : public NESPlugin
    {
    public:
        virtual bool create(Application& application) override
        {
            application.getBackendRegistry().add(mNESBackend);
            return true;
        }

        virtual void destroy(Application& application) override
        {
            application.getBackendRegistry().remove(mNESBackend);
        }

        virtual const char* getName() const override
        {
            return "NES";
        }

    private:
        NESBackend      mNESBackend;
    };
}

NESPlugin* NESPlugin::create()
{
    return new NESPluginImpl();
}

void NESPlugin::destroy(NESPlugin& instance)
{
    delete &static_cast<NESPluginImpl&>(instance);
}
