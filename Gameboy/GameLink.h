#pragma once

#include <Core/Core.h>
#include <Core/RegisterBank.h>
#include "GB.h"

namespace gb
{
    class GameLink
    {
    public:
        GameLink();
        ~GameLink();
        bool create(emu::RegisterBank& registersIO);
        void destroy();
        void reset();
        void serialize(emu::ISerializer& serializer);

    private:
        uint8_t readSB(int32_t tick, uint16_t addr);
        void writeSB(int32_t tick, uint16_t addr, uint8_t value);
        uint8_t readSC(int32_t tick, uint16_t addr);
        void writeSC(int32_t tick, uint16_t addr, uint8_t value);

        struct RegisterAccessors
        {
            struct
            {
                emu::RegisterRead   SB;
                emu::RegisterRead   SC;
            }                       read;

            struct
            {
                emu::RegisterWrite  SB;
                emu::RegisterWrite  SC;
            }                       write;
        };

        RegisterAccessors   mRegisterAccessors;
        uint8_t             mRegSB;
        uint8_t             mRegSC;
    };
}
