#ifndef __EMULATOR_H__
#define __EMULATOR_H__

#ifdef EXPORT_EMULATOR
#define EMULATOR_API    __declspec(dllexport)
#else
#define EMULATOR_API    __declspec(dllimport)
#endif

namespace emu
{
    class IEmulatorVisitor
    {
    public:
    };

    class IEmulatorAPI
    {
    public:
        virtual ~IEmulatorAPI() {}
        virtual void visitEmulators(IEmulatorVisitor& visitor) = 0;
    };
}

EMULATOR_API emu::IEmulatorAPI* emuCreateAPI();
EMULATOR_API void emuDestroyAPI(emu::IEmulatorAPI& api);

#endif
