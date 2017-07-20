#pragma once

#include "Core.h"

namespace emu
{
    struct Context;
    struct Rom;

    class IEmulator
    {
    public:
        struct SystemInfo
        {
            const char* name = "";
            const char* extensions = "";
        };

        virtual bool getSystemInfo(SystemInfo& info) = 0;
        virtual Rom* loadRom(const char* path) = 0;
        virtual void unloadRom(Rom& rom) = 0;
        virtual Context* createContext(const Rom& rom) = 0;
        virtual void destroyContext(Context& context) = 0;
        virtual bool getDisplaySize(emu::Context& context, uint32_t& sizeX, uint32_t& sizeY) = 0;
        virtual bool serializeGameData(Context& context, emu::ISerializer& serializer) = 0;
        virtual bool serializeGameState(Context& context, emu::ISerializer& serializer) = 0;
        virtual bool setRenderBuffer(Context& context, void* buffer, size_t pitch) = 0;
        virtual bool setSoundBuffer(Context& context, void* buffer, size_t size) = 0;
        virtual bool setController(Context& context, uint32_t index, uint32_t value) = 0;
        virtual bool reset(Context& context) = 0;
        virtual bool execute(Context& context) = 0;
    };
}
