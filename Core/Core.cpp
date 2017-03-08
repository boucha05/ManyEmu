#include "Core.h"
#include "Log.h"

namespace emu
{
    void Assert(bool valid, const char* msg)
    {
        if (!valid)
            emu::Log::printf(emu::Log::Type::Error, "Assertion failed: %s\n", msg);
    }

    void notImplemented(const char* function)
    {
        emu::Log::printf(emu::Log::Type::Warning, "Feature not implemented in %s\n", function);
    }
}