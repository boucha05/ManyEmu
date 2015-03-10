#ifndef __INPUT_MANAGER_H__
#define __INPUT_MANAGER_H__

#include "nes.h"
#include <SDL.h>
#include <stdint.h>
#include <vector>

class InputDevice;
class InputManager;

class InputManager
{
public:
    InputManager();
    ~InputManager();
    bool create(uint32_t numInputs);
    void destroy();
    void clearInputs();
    void updateInput(uint32_t id, float value);
    float getInput(uint32_t id);
    bool isPressed(uint32_t id);

private:
    typedef std::vector<float> InputTable;

    void initialize();

    InputTable      mInputs;
};

class InputDevice : public NES::IDisposable
{
public:
    virtual void update(InputManager& inputManager) = 0;
};

class KeyboardDevice : public InputDevice
{
public:
    virtual void addKey(SDL_Scancode scancode, uint32_t inputId, float maxValue = 1.0f, float minValue = 0.0f) = 0;

    static KeyboardDevice* create();
};

class GamepadDevice : public InputDevice
{
public:
    virtual void addButton(SDL_GameControllerButton button, uint32_t inputId, float maxValue = 1.0f, float minValue = 0.0f) = 0;
    virtual void addAxis(SDL_GameControllerAxis axis, uint32_t inputId, float maxValue = +1.0f, float minValue = -1.0f) = 0;

    static GamepadDevice* create(uint32_t index);
};

#endif
