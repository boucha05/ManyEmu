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

    class GameController : public InputController
    {
    public:
        virtual void addButton(SDL_GameControllerButton button, uint32_t buttonMask) = 0;
        virtual void addAxis(SDL_GameControllerAxis axis, uint32_t buttonMask1, uint32_t buttonMask2) = 0;

        static GameController* create(uint32_t index);
    };

    class GroupController : public InputController
    {
    public:
        virtual void addController(InputController& controller) = 0;

        static GroupController* create();
    };

    class InputRecorder : public InputController
    {
    public:
        virtual InputController& getSource() = 0;
        virtual bool save(const char* path) = 0;

        static InputRecorder* create(InputController& source);
    };

    class InputPlayback : public InputController
    {
    public:
        virtual InputController& getSource() = 0;
        virtual bool load(const char* path) = 0;

        static InputPlayback* create(InputController& source);
    };
}

#endif
