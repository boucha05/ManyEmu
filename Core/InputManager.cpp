#include "InputManager.h"
#include <vector>

InputManager::InputManager()
{
    initialize();
}

InputManager::~InputManager()
{
    destroy();
}

void InputManager::initialize()
{
    mInputs.clear();
}

bool InputManager::create(uint32_t inputCount)
{
    mInputs.resize(inputCount);
    return true;
}

void InputManager::destroy()
{
    initialize();
}

void InputManager::clearInputs()
{
    for (auto& input : mInputs)
    {
        input = 0.0f;
    }
}

void InputManager::updateInput(uint32_t id, float value)
{
    if (id >= mInputs.size())
        return;
    mInputs[id] += value;
}

float InputManager::getInput(uint32_t id)
{
    if (id >= mInputs.size())
        return 0.0f;
    return mInputs[id];
}

bool InputManager::isPressed(uint32_t id)
{
    return abs(getInput(id)) > 0.1f;
}

////////////////////////////////////////////////////////////////////////////////

namespace
{
    class KeyboardDeviceImpl : public KeyboardDevice
    {
    public:
        ~KeyboardDeviceImpl()
        {
            destroy();
        }

        virtual void dispose()
        {
            delete this;
        }

        bool create()
        {
            return true;
        }

        void destroy()
        {
        }

        virtual void update(InputManager& inputManager)
        {
            const uint8_t* state = SDL_GetKeyboardState(nullptr);
            for (auto mapping : mMappings)
            {
                float value = state[mapping.scancode] ? mapping.maxValue : mapping.minValue;
                inputManager.updateInput(mapping.inputId, value);
            }
        }

        virtual void addKey(SDL_Scancode scancode, uint32_t inputId, float maxValue, float minValue)
        {
            KeyboardMapping mapping =
            {
                scancode,
                inputId,
                minValue,
                maxValue,
            };
            mMappings.push_back(mapping);
        }

    private:
        struct KeyboardMapping
        {
            SDL_Scancode    scancode;
            uint32_t        inputId;
            float           minValue;
            float           maxValue;
        };

        typedef std::vector<KeyboardMapping> MappingTable;

        MappingTable    mMappings;
    };

    ///////////////////////////////////////////////////////////////////////////

    class GamepadDeviceImpl : public GamepadDevice
    {
    public:
        GamepadDeviceImpl()
            : mGameController(nullptr)
        {
        }

        ~GamepadDeviceImpl()
        {
            destroy();
        }

        virtual void dispose()
        {
            delete this;
        }

        bool create(uint32_t index)
        {
            mGameController = SDL_GameControllerOpen(index);
            if (!mGameController)
                return false;

            return true;
        }

        void destroy()
        {
            if (mGameController)
            {
                SDL_GameControllerClose(mGameController);
                mGameController = nullptr;
            }
        }

        virtual void update(InputManager& inputManager)
        {
            for (auto mapping : mButtons)
            {
                bool state = SDL_GameControllerGetButton(mGameController, mapping.button) != 0;
                float value = state ? mapping.maxValue : mapping.minValue;
                inputManager.updateInput(mapping.inputId, value);
            }
            for (auto mapping : mAxis)
            {
                static const int16_t threshold = 32768 * 20 / 100;
                int16_t stateRaw = SDL_GameControllerGetAxis(mGameController, mapping.axis);
                if ((stateRaw >= -threshold) && (stateRaw <= threshold))
                    stateRaw = 0;
                float state = (stateRaw + 32767.5f) / 65535.0f;
                float value = state * mapping.maxValue + (1.0f - state) * mapping.minValue;
                inputManager.updateInput(mapping.inputId, value);
            }
        }

        virtual void addButton(SDL_GameControllerButton button, uint32_t inputId, float maxValue, float minValue)
        {
            ButtonMapping mapping =
            {
                button,
                inputId,
                minValue,
                maxValue,
            };
            mButtons.push_back(mapping);
        }

        virtual void addAxis(SDL_GameControllerAxis axis, uint32_t inputId, float maxValue, float minValue)
        {
            AxisMapping mapping =
            {
                axis,
                inputId,
                minValue,
                maxValue,
            };
            mAxis.push_back(mapping);
        }

    private:
        struct ButtonMapping
        {
            SDL_GameControllerButton    button;
            uint32_t                    inputId;
            float                       minValue;
            float                       maxValue;
        };

        struct AxisMapping
        {
            SDL_GameControllerAxis      axis;
            uint32_t                    inputId;
            float                       minValue;
            float                       maxValue;
        };

        typedef std::vector<ButtonMapping> ButtonMappingTable;
        typedef std::vector<AxisMapping> AxisMappingTable;

        ButtonMappingTable      mButtons;
        AxisMappingTable        mAxis;
        SDL_GameController*     mGameController;
    };
}

KeyboardDevice* KeyboardDevice::create()
{
    KeyboardDeviceImpl* instance = new KeyboardDeviceImpl();
    if (!instance->create())
    {
        instance->dispose();
        instance = nullptr;
    }
    return instance;
}

GamepadDevice* GamepadDevice::create(uint32_t index)
{
    GamepadDeviceImpl* instance = new GamepadDeviceImpl();
    if (!instance->create(index))
    {
        instance->dispose();
        instance = nullptr;
    }
    return instance;
}
