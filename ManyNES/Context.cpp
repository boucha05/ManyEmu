#include "nes.h"
#include "Cpu6502.h"
#include "Mappers.h"
#include "MemoryBus.h"
#include <assert.h>
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

    static const uint32_t PPU_REG_PPUCTRL = 0x2000;
    static const uint32_t PPU_REG_PPUMASK = 0x2001;
    static const uint32_t PPU_REG_PPUSTATUS = 0x2002;
    static const uint32_t PPU_REG_OAMADDR = 0x2003;
    static const uint32_t PPU_REG_OAMDATA = 0x2004;
    static const uint32_t PPU_REG_PPUSCROLL = 0x2005;
    static const uint32_t PPU_REG_PPUADDR = 0x2006;
    static const uint32_t PPU_REG_PPUDATA = 0x2007;

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
            if (!cpu.create(cpuMemory.getState()))
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

            cpu.reset();

            // TODO: REMOVE THIS ONCE THE CPU AND MEMORY ARE MORE RELIABLE
            cpu.addTimedEvent(startVBlank, this, 80);
            cpu.execute(100);

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
        inline uint8_t ppuRegsRead(uint32_t addr)
        {
            addr = 0x2000 + (addr & 7);
            switch (addr)
            {
            //case PPU_REG_PPUCTRL: break;
            //case PPU_REG_PPUMASK: break;
            case PPU_REG_PPUSTATUS:
                return 0; // TODO: Implement status register and reset latches
            //case PPU_REG_OAMADDR: break;
            //case PPU_REG_OAMDATA: break;
            //case PPU_REG_PPUSCROLL: break;
            //case PPU_REG_PPUADDR: break;
            //case PPU_REG_PPUDATA: break;
            default:
                NOT_IMPLEMENTED();
            }
            return 0;
        }

        inline void ppuRegsWrite(uint32_t addr, uint8_t value)
        {
            addr = 0x2000 + (addr & 7);
            switch (addr)
            {
            case PPU_REG_PPUCTRL:
                // TODO: Toggle rendering if sensitive state change are detected
                ppu.ppu_ctrl = value;
                break;
            //case PPU_REG_PPUMASK: break;
            //case PPU_REG_PPUSTATUS: break;
            //case PPU_REG_OAMADDR: break;
            //case PPU_REG_OAMDATA: break;
            //case PPU_REG_PPUSCROLL: break;
            //case PPU_REG_PPUADDR: break;
            //case PPU_REG_PPUDATA: break;
            default:
                NOT_IMPLEMENTED();
            }
        }

        static uint8_t ppuRegsRead(void* context, uint32_t addr)
        {
            return static_cast<ContextImpl*>(context)->ppuRegsRead(addr);
        }

        static void ppuRegsWrite(void* context, uint32_t addr, uint8_t value)
        {
            static_cast<ContextImpl*>(context)->ppuRegsWrite(addr, value);
        }

        void startVBlank()
        {
            printf("[VBLANK]\n");
        }

        static void startVBlank(void* context, int32_t ticks)
        {
            static_cast<ContextImpl*>(context)->startVBlank();
        }

        struct PPU_STATE
        {
            uint8_t             ppu_ctrl;
        };

        const NES::Rom*         rom;
        MemoryBus               cpuMemory;
        MEM_ACCESS              accessPrgRom1;
        MEM_ACCESS              accessPrgRom2;
        MEM_ACCESS              accessPpuRegsRead;
        MEM_ACCESS              accessPpuRegsWrite;
        MEM_ACCESS              accessCpuRamRead;
        MEM_ACCESS              accessCpuRamWrite;
        Cpu6502                 cpu;
        PPU_STATE               ppu;
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