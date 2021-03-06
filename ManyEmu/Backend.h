#pragma once
#include <Core/Context.h>
#include <Core/Core.h>
#include <Core/Emulator.h>
#include <Core/Rom.h>
#include <Core/InputController.h>
#include "InputManager.h"
#include <string>
#include <unordered_map>
#include <vector>

enum
{
    Input_A,
    Input_B,
    Input_Select,
    Input_Start,
    Input_VerticalAxis,
    Input_HorizontalAxis,
    Input_PlayerCount,
};

enum
{
    Input_Exit,
    Input_TimeDir,
    Input_Player1,
    Input_Player2 = Input_Player1 + Input_PlayerCount,
    Input_Count = Input_Player2 + Input_PlayerCount,
};

class StandardController : public emu::InputController
{
public:
    StandardController(InputManager& inputManager)
        : mInputManager(&inputManager)
    {
    }

    virtual void dispose()
    {
        delete this;
    }

    virtual uint8_t readInput()
    {
        uint8_t buttonMask = 0;
        for (auto mapping : mInputs)
        {
            static const float threshold = 0.4f;
            float value = mInputManager->getInput(mapping.inputId);
            if (value < -threshold)
                buttonMask |= mapping.buttonMaskMin;
            else if (value > threshold)
                buttonMask |= mapping.buttonMaskMax;
        }
        return buttonMask;
    }

    virtual void addInput(uint32_t inputId, uint32_t buttonMaskMax, uint32_t buttonMaskMin = 0)
    {
        InputMapping mapping =
        {
            inputId,
            buttonMaskMax,
            buttonMaskMin,
        };
        mInputs.push_back(mapping);
    }

private:
    struct InputMapping
    {
        uint32_t    inputId;
        uint32_t    buttonMaskMax;
        uint32_t    buttonMaskMin;
    };

    typedef std::vector<InputMapping> InputMappingTable;

    InputMappingTable   mInputs;
    InputManager*       mInputManager;
};

class IBackend
{
public:
    virtual emu::IEmulator& getEmulator() = 0;
    virtual void configureController(StandardController& controller, uint32_t player) = 0;
};

class BackendRegistry
{
public:
    class Visitor
    {
    public:
        virtual void visit(IBackend& backend) { EMU_UNUSED(backend); }
    };

    BackendRegistry();
    ~BackendRegistry();
    IBackend* getBackend(const char* extension);
    bool add(IBackend& backend);
    void remove(IBackend& backend);
    void accept(Visitor& visitor);

private:
    std::unordered_map<std::string, IBackend*>  mBackends;
};
