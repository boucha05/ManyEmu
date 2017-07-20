#pragma once

#include "Core.h"

namespace emu
{
    class ICPU
    {
    public:
        virtual const char* getName() = 0;
        virtual bool disassemble(char* buffer, size_t size, size_t& addr) = 0;
    };
}
