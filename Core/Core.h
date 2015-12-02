#ifndef __CORE_H__
#define __CORE_H__

#include <stdint.h>
#if defined(DEBUG) || defined(_DEBUG)
#include <assert.h>
#define EMU_ASSERT(e)   emu::Assert(!!(e), #e)
#define EMU_VERIFY(e)   emu::Assert(!!(e), #e)
#else
#define EMU_ASSERT(e)
#define EMU_VERIFY(e)   (e)
#endif

#define EMU_ARRAY_SIZE(a)  (sizeof(a) / sizeof((a)[0]))

namespace emu
{
    void Assert(bool valid, const char* msg);

    class ISerializer;

    class IDisposable
    {
    public:
        virtual void dispose() = 0;
    };

    struct Rom
    {
    };

    struct Context
    {
    };

    class IEmulator
    {
    public:
        virtual Rom* loadRom(const char* path) = 0;
        virtual void unloadRom(Rom& rom) = 0;
        virtual Context* createContext(const Rom& rom) = 0;
        virtual void destroyContext(Context& context) = 0;
        virtual bool serializeGameData(Context& context, emu::ISerializer& serializer) = 0;
        virtual bool serializeGameState(Context& context, emu::ISerializer& serializer) = 0;
        virtual bool setRenderBuffer(Context& context, void* buffer, size_t pitch) = 0;
        virtual bool setSoundBuffer(Context& context, void* buffer, size_t size) = 0;
        virtual bool setController(Context& context, uint32_t index, uint32_t value) = 0;
        virtual bool reset(Context& context) = 0;
        virtual bool execute(Context& context) = 0;
    };
}

#endif
