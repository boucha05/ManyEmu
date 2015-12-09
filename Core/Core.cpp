#include "Core.h"
#include <stdio.h>

namespace emu
{
    void Assert(bool valid, const char* msg)
    {
        if (!valid)
            printf("Assertion failed: %s\n", msg);
    }

    void notImplemented(const char* function)
    {
        printf("Feature not implemented in %s\n", function);
    }
}