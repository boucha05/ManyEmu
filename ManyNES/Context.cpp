#include "nes.h"
#include "Cpu6502.h"
#include "Mappers.h"
#include "MemoryBus.h"
#include <vector>

namespace
{
    static const uint32_t MEM_SIZE_LOG2 = 16;
    static const uint32_t MEM_SIZE = 1 << MEM_SIZE_LOG2;
    static const uint32_t MEM_PAGE_SIZE_LOG2 = 10;
    static const uint32_t MEM_PAGE_SIZE = 1 << MEM_PAGE_SIZE_LOG2;
    static const uint32_t MEM_PAGE_COUNT = 16 - MEM_PAGE_SIZE_LOG2;

    static const uint32_t CPU_CLOCK_FREQUENCY_NTSC = 1789773;
    static const uint32_t CPU_CLOCK_FREQUENCY_PAL = 1662607;
    static const uint32_t CPU_CLOCK_DENDY = 1773448;
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

            if (!cpuMemory.create(MEM_SIZE_LOG2, MEM_PAGE_SIZE_LOG2))
                return false;
            if (!cpu.create(cpuMemory.getState()))
                return false;

            const auto& romDesc = rom->getDescription();
            const auto& romContent = rom->getContent();

            const uint8_t* romPage1 = romContent.prgRom;
            const uint8_t* romPage2 = romDesc.prgRomPages > 1 ? romPage1 + 0x4000 : romPage1;
            accessPrgRom1.setReadMemory(romPage1);
            accessPrgRom2.setReadMemory(romPage2);
            cpuMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_READ, 0x8000, 0xbfff, accessPrgRom1);
            cpuMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_READ, 0xc000, 0xffff, accessPrgRom2);

            mapper = NES::MapperRegistry::getInstance().create(romDesc.mapper);
            if (!mapper)
                return false;
            if (!mapper->initialize(*rom, cpuMemory))
                return false;

            cpu.reset();

            // TODO: REMOVE THIS ONCE THE CPU AND MEMORY ARE MORE RELIABLE
            //cpu.execute(100);

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
            cpuMemory.destroy();

            rom = nullptr;
        }

        virtual void dispose()
        {
            delete this;
        }

        virtual void update(void* surface, uint32_t pitch)
        {
            //cpu.execute(CPU_CLOCK_FREQUENCY_NTSC);
        }

    private:
        const NES::Rom*     rom;
        MemoryBus           cpuMemory;
        MEM_ACCESS          accessPrgRom1;
        MEM_ACCESS          accessPrgRom2;
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