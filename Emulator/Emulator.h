#ifndef __EMULATOR_H__
#define __EMULATOR_H__

#ifdef EXPORT_EMULATOR
#define EMULATOR_API    __declspec(dllexport)
#else
#define EMULATOR_API    __declspec(dllimport)
#endif

namespace emu
{
    class IEmulator;
    class IComponentManager;

    class IEmulatorVisitor
    {
    public:
        virtual ~IEmulatorVisitor() {}
        virtual void visitEmulator(const char* name, IEmulator& emulator) = 0;
    };

    class IEmulatorAPI
    {
    public:
        virtual void traverseEmulators(IEmulatorVisitor& visitor) = 0;
        virtual IEmulator* getEmulator(const char* name) = 0;
        virtual IComponentManager& getComponentManager() = 0;
    };
}

EMULATOR_API emu::IEmulatorAPI* emuCreateAPI();
EMULATOR_API void emuDestroyAPI(emu::IEmulatorAPI& api);

#endif
