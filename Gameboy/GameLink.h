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

        static uint8_t readSB(void* context, int32_t tick, uint16_t addr)
        {
            return static_cast<GameLink*>(context)->readSB(tick, addr);
        }

        static void writeSB(void* context, int32_t tick, uint16_t addr, uint8_t value)
        {
            static_cast<GameLink*>(context)->writeSB(tick, addr, value);
        }

        static uint8_t readSC(void* context, int32_t tick, uint16_t addr)
        {
            return static_cast<GameLink*>(context)->readSC(tick, addr);
        }

        static void writeSC(void* context, int32_t tick, uint16_t addr, uint8_t value)
        {
            static_cast<GameLink*>(context)->writeSC(tick, addr, value);
        }

        uint8_t                 mRegSB;
        uint8_t                 mRegSC;
    };
}
