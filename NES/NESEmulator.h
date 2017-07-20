#ifndef __NES_EMULATOR_H__
#define __NES_EMULATOR_H__

#include <stdint.h>
#include <string>
#include <Core/Emulator.h>

namespace nes
{
    class Emulator : public emu::IEmulator
    {
    public:
        static Emulator& getInstance();
    };
}

#endif
