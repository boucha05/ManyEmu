#include "Backend.h"
#include <SDL.h>
#include <string>
#include <vector>
#include <deque>
#include <Core/InputController.h>
#include <Core/InputManager.h>
#include <Core/Job.h>
#include <Core/Path.h>
#include <Core/Serialization.h>
#include <Core/Stream.h>
#include <Gameboy/GB.h>

namespace
{
    class GBBackend : public IBackend
    {
    public:
        virtual const char* getExtension() override
        {
            return "gb";
        }

        virtual emu::IEmulator& getEmulator() override
        {
            return gb::getEmulatorGB();
        }

        virtual void configureController(StandardController& controller, uint32_t player)
        {
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
        virtual const char* getExtension() override
        {
            return "gbc";
        }

        virtual emu::IEmulator& getEmulator() override
        {
            return gb::getEmulatorGBC();
        }
    };

    BackendRegistry::AutoRegister<GBBackend> gb;
    BackendRegistry::AutoRegister<GBCBackend> gbc;
}
