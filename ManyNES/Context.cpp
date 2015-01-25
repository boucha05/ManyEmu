#include "nes.h"
#include "Cpu6502.h"
#include "Mappers.h"
#include "MemoryBus.h"
#include "PPU.h"
#include <assert.h>
#include <vector>

namespace
{
    static const uint32_t MEM_SIZE_LOG2 = 16;
    static const uint32_t MEM_SIZE = 1 << MEM_SIZE_LOG2;
    static const uint32_t MEM_PAGE_SIZE_LOG2 = 10;
    static const uint32_t MEM_PAGE_SIZE = 1 << MEM_PAGE_SIZE_LOG2;
    static const uint32_t MEM_PAGE_COUNT = 16 - MEM_PAGE_SIZE_LOG2;

    static const uint32_t MASTER_CLOCK_FREQUENCY_NTSC = 21477272;
    static const uint32_t MASTER_CLOCK_FREQUENCY_PAL = 26601712;
    static const uint32_t MASTER_CLOCK_DENDY = 26601712;

    static const uint32_t MASTER_CLOCK_CPU_DIVIDER_NTSC = 12;
    static const uint32_t MASTER_CLOCK_CPU_DIVIDER_PAL = 16;
    static const uint32_t MASTER_CLOCK_CPU_DIVIDER_DENDY = 15;

    void NOT_IMPLEMENTED()
    {
        printf("Feature not implemented\n");
        assert(false);
    }
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
            if (!cpu.create(cpuMemory.getState(), MASTER_CLOCK_CPU_DIVIDER_NTSC))
                return false;

            const auto& romDesc = rom->getDescription();
            const auto& romContent = rom->getContent();

            // ROM
            const uint8_t* romPage1 = romContent.prgRom;
            const uint8_t* romPage2 = romDesc.prgRomPages > 1 ? romPage1 + 0x4000 : romPage1;
            accessPrgRom1.setReadMemory(romPage1);
            accessPrgRom2.setReadMemory(romPage2);
            cpuMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_READ, 0x8000, 0xbfff, accessPrgRom1);
            cpuMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_READ, 0xc000, 0xffff, accessPrgRom2);

            // PPU registers
            accessPpuRegsRead.setReadMethod(ppuRegsRead, this, 0x2000);
            accessPpuRegsWrite.setWriteMethod(ppuRegsWrite, this, 0x2000);
            cpuMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_READ, 0x2000, 0x3fff, accessPpuRegsRead);
            cpuMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_WRITE, 0x2000, 0x3fff, accessPpuRegsWrite);

            // CPU RAM
            cpuRam.resize(0x800, 0);
            accessCpuRamRead.setReadMemory(&cpuRam[0]);
            accessCpuRamWrite.setWriteMemory(&cpuRam[0]);
            for (uint16_t mirror = 0; mirror < 4; ++mirror)
            {
                uint16_t addr_start = mirror * 0x0800;
                uint16_t addr_end = addr_start + 0x07ff;
                cpuMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_READ, addr_start, addr_end, accessCpuRamRead);
                cpuMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_WRITE, addr_start, addr_end, accessCpuRamWrite);
            }

            mapper = NES::MapperRegistry::getInstance().create(romDesc.mapper);
            if (!mapper)
                return false;
            if (!mapper->initialize(*rom, cpuMemory))
                return false;

            reset();

            // TODO: REMOVE THIS ONCE THE CPU AND MEMORY ARE MORE RELIABLE
            cpu.addTimedEvent(startVBlank, this, 80 * MASTER_CLOCK_CPU_DIVIDER_NTSC);
            cpu.execute(100 * MASTER_CLOCK_CPU_DIVIDER_NTSC);

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

        virtual void reset()
        {
            cpu.reset();
            ppu.reset();
            mapper->reset();
        }

        virtual void update(void* surface, uint32_t pitch)
        {
            //cpu.execute(MASTER_CLOCK_FREQUENCY_NTSC / 60);
            ppu.update(surface, pitch);
        }

    private:
        static uint8_t ppuRegsRead(void* context, int32_t ticks, uint32_t addr)
        {
            return static_cast<ContextImpl*>(context)->ppu.regRead(ticks, addr);
        }

        static void ppuRegsWrite(void* context, int32_t ticks, uint32_t addr, uint8_t value)
        {
            static_cast<ContextImpl*>(context)->ppu.regWrite(ticks, addr, value);
        }

        void startVBlank()
        {
            ppu.startVBlank();
        }

        static void startVBlank(void* context, int32_t ticks)
        {
            static_cast<ContextImpl*>(context)->startVBlank();
        }

        const NES::Rom*         rom;
        MemoryBus               cpuMemory;
        MEM_ACCESS              accessPrgRom1;
        MEM_ACCESS              accessPrgRom2;
        MEM_ACCESS              accessPpuRegsRead;
        MEM_ACCESS              accessPpuRegsWrite;
        MEM_ACCESS              accessCpuRamRead;
        MEM_ACCESS              accessCpuRamWrite;
        Cpu6502                 cpu;
        NES::PPU                ppu;
        std::vector<uint8_t>    cpuRam;
        NES::Mapper*            mapper;
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