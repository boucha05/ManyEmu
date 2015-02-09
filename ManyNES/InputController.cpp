#include "InputController.h"
#include <vector>

namespace
{
    class KeyboardControllerImpl : public NES::KeyboardController
    {
    public:
        ~KeyboardControllerImpl()
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

        virtual uint8_t readInput()
        {
            const uint8_t* state = SDL_GetKeyboardState(nullptr);
            uint8_t buttonMask = 0;
            for (auto mapping : mMappings)
            {
                if (state[mapping.scancode])
                    buttonMask |= mapping.buttonMask;
            }
            return buttonMask;
        }

        virtual void addKey(SDL_Scancode scancode, uint32_t buttonMask)
        {
            KeyboardMapping mapping =
            {
                scancode,
                buttonMask
            };
            mMappings.push_back(mapping);
        }

    private:
        struct KeyboardMapping
        {
            SDL_Scancode    scancode;
            uint8_t         buttonMask;
        };

        typedef std::vector<KeyboardMapping> MappingTable;

        MappingTable    mMappings;
    };
}

namespace NES
{
    KeyboardController* KeyboardController::create()
    {
        return new KeyboardControllerImpl();
    }
}