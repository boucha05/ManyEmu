#ifndef __INPUT_CONTROLLER_H__
#define __INPUT_CONTROLLER_H__

#include <stdint.h>
#include <SDL.h>
#include "nes.h"

namespace NES
{
    class InputController : public IDisposable
    {
    public:
        virtual void reset() {}
        virtual uint8_t readInput() = 0;
    };

    class KeyboardController : public InputController
    {
    public:
        virtual void addKey(SDL_Scancode scancode, uint32_t buttonMask) = 0;

        static KeyboardController* create();
    };
}

#endif
