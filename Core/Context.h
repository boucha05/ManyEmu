#pragma once

#include "Core.h"

namespace emu
{
    class IContext
    {
    public:
        virtual bool getDisplaySize(uint32_t& sizeX, uint32_t& sizeY) = 0;
        virtual bool serializeGameData(ISerializer& serializer) = 0;
        virtual bool serializeGameState(ISerializer& serializer) = 0;
        virtual bool setRenderBuffer(void* buffer, size_t pitch) = 0;
        virtual bool setSoundBuffer(void* buffer, size_t size) = 0;
        virtual bool setController(uint32_t index, uint32_t value) = 0;
        virtual bool reset() = 0;
        virtual bool execute() = 0;
    };
}
