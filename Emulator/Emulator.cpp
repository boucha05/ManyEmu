#include "Emulator.h"

namespace
{
    using namespace emu;

    class EmulatorAPI : public IEmulatorAPI
    {
    public:
        virtual void visitEmulators(IEmulatorVisitor& visitor) override
        {
            (void)visitor;
        }
    };
}

emu::IEmulatorAPI* emuCreateAPI()
{
    return new EmulatorAPI();
}

void emuDestroyAPI(emu::IEmulatorAPI& api)
{
    delete &api;
}
