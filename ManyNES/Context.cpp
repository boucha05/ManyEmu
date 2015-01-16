#include "nes.h"

namespace
{
    class ContextImpl : public NES::Context
    {
    public:
        ContextImpl()
            : rom(nullptr)
        {
        }

        ~ContextImpl()
        {
            destroy();
        }

        bool create(const NES::Rom& _rom)
        {
            rom = &_rom;

            return true;
        }

        void destroy()
        {
            rom = nullptr;
        }

        virtual void dispose()
        {
            delete this;
        }

        virtual void update(void* surface, uint32_t pitch)
        {
        }

    private:
        const NES::Rom*     rom;
    };
}

namespace NES
{
    Context* Context::create(const Rom& rom)
    {
        ContextImpl* context = new ContextImpl;
        if (!context->create(rom))
        {
            context->dispose();
            context = nullptr;
        }
        return context;
    }
}