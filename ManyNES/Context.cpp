#include "nes.h"
#include "Cpu6502.h"
#include "Mappers.h"
#include "MemoryBus.h"
#include <vector>

namespace
{
}

namespace
{
    class ContextImpl : public NES::Context
    {
    public:
        ContextImpl()
            : rom(nullptr)
            , mapper(nullptr)
        {
        }

        ~ContextImpl()
        {
            destroy();
        }

        bool create(const NES::Rom& _rom)
        {
            rom = &_rom;

            if (cpu.create(cpuMemory.getState()))
                return false;

            mapper = NES::MapperRegistry::getInstance().create(rom->getDescription().mapper);
            if (!mapper)
                return false;
            if (!mapper->initialize(*rom, cpuMemory))
                return false;

            return true;
        }

        void destroy()
        {
            if (mapper)
            {
                mapper->dispose();
                mapper = nullptr;
            }

            cpu.destroy();

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
        MemoryBus           cpuMemory;
        Cpu6502             cpu;
        NES::Mapper*        mapper;
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