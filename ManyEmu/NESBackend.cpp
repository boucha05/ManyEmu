#include "Backend.h"
#include "InputManager.h"
#include "Job.h"
#include <SDL.h>
#include <string>
#include <vector>
#include <deque>
#include <Core/InputController.h>
#include <Core/Path.h>
#include <NES/NESEmulator.h>
#include <NES/Tests.h>
#include <NES/nes.h>

namespace
{
    class NESBackend : public IBackend
    {
    public:
        virtual const char* getExtension() override
        {
            return "nes";
        }

        virtual void configureController(StandardController& controller, uint32_t player)
        {
            controller.addInput(Input_Player1 + Input_HorizontalAxis, nes::Context::ButtonRight, nes::Context::ButtonLeft);
            controller.addInput(Input_Player1 + Input_VerticalAxis, nes::Context::ButtonDown, nes::Context::ButtonUp);
            controller.addInput(Input_Player1 + Input_B, nes::Context::ButtonB);
            controller.addInput(Input_Player1 + Input_A, nes::Context::ButtonA);
            controller.addInput(Input_Player1 + Input_Select, nes::Context::ButtonSelect);
            controller.addInput(Input_Player1 + Input_Start, nes::Context::ButtonStart);
        }
    };

    BackendRegistry::AutoRegister<NESBackend> nes;
}
