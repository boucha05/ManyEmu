#ifndef __INPUT_CONTROLLER_H__
#define __INPUT_CONTROLLER_H__

#include <stdint.h>
#include "nes.h"

namespace NES
{
    class InputController : public IDisposable
    {
    public:
        virtual void reset() {}
        virtual uint8_t readInput() = 0;
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
