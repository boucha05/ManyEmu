#include "Core.h"
#include <stdio.h>

namespace emu
{
    void Assert(bool valid, const char* msg)
    {
        if (!valid)
            printf("Assertion failed: %s\n", msg);
    }
}