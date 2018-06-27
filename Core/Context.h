#pragma once

#include "Core.h"

namespace emu
{
    class ICPU;

    class IContext
    {
    public:
        struct SystemInfo
        {
            uint32_t    cpuCount = 0;
        };

        struct DisplayInfo
        {
            uint32_t    sizeX = 0;
            uint32_t    sizeY = 0;
            float       fps = 60.0f;
        };

        virtual bool getSystemInfo(SystemInfo& info) = 0;
        virtual bool getDisplayInfo(DisplayInfo& info) = 0;
        virtual bool serializeGameData(ISerializer& serializer) = 0;
        virtual bool serializeGameState(ISerializer& serializer) = 0;
        virtual bool setRenderBuffer(void* buffer, size_t pitch) = 0;
        virtual bool setSoundBuffer(void* buffer, size_t size) = 0;
        virtual bool setController(uint32_t index, uint32_t value) = 0;
        virtual bool reset() = 0;
        virtual bool execute() = 0;

        virtual ICPU* getCpu(uint32_t index)
        {
            EMU_UNUSED(index);
            return nullptr;
        }
    };
}
