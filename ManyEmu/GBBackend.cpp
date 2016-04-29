#include "Backend.h"
#include "InputManager.h"
#include "Serialization.h"
#include "Stream.h"
#include "Job.h"
#include <SDL.h>
#include <string>
#include <vector>
#include <deque>
#include <Core/InputController.h>
#include <Core/Path.h>

namespace
{
    class GBBackend : public IBackend
    {
    public:
        virtual const char* getExtension() override
        {
            return "gb";
        }

        virtual void configureController(StandardController& controller, uint32_t player, emu::IInputDevice& inputDevice)
        {
            controller.addInput(inputDevice, Input_Player1 + Input_HorizontalAxis, "Right", "Left");
            controller.addInput(inputDevice, Input_Player1 + Input_VerticalAxis, "Down", "Up");
            controller.addInput(inputDevice, Input_Player1 + Input_B, "B");
            controller.addInput(inputDevice, Input_Player1 + Input_A, "A");
            controller.addInput(inputDevice, Input_Player1 + Input_X, "B");
            controller.addInput(inputDevice, Input_Player1 + Input_Y, "A");
            controller.addInput(inputDevice, Input_Player1 + Input_Select, "Select");
            controller.addInput(inputDevice, Input_Player1 + Input_Start, "Start");
        }
    };

    class GBCBackend : public GBBackend
    {
    public:
        virtual const char* getExtension() override
        {
            return "gbc";
        }
    };

    BackendRegistry::AutoRegister<GBBackend> gb;
    BackendRegistry::AutoRegister<GBCBackend> gbc;
}
