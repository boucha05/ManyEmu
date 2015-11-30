#ifndef __CORE_H__
#define __CORE_H__

#include <stdint.h>
#if defined(DEBUG) || defined(_DEBUG)
#include <assert.h>
#define EMU_ASSERT(e)   NES::Assert(!!(e), #e)
#define EMU_VERIFY(e)   NES::Assert(!!(e), #e)
#else
#define EMU_ASSERT(e)
#define EMU_VERIFY(e)   (e)
#endif

#define EMU_ARRAY_SIZE(a)  (sizeof(a) / sizeof((a)[0]))

namespace NES
{
    void Assert(bool valid, const char* msg);

    class ISerializer;

    class IDisposable
    {
    public:
        virtual void dispose() = 0;
    };
}

#endif
