#pragma once

#include "Core.h"

namespace emu
{
    class IContext;
    class IRom;

    class IEmulator
    {
    public:
        struct SystemInfo
        {
            const char* name = "";
            const char* extensions = "";
        };

        virtual bool getSystemInfo(SystemInfo& info) = 0;
        virtual IRom* loadRom(const char* path) = 0;
        virtual void unloadRom(IRom& rom) = 0;
        virtual IContext* createContext(const IRom& rom) = 0;
        virtual void destroyContext(IContext& context) = 0;
    };
}
