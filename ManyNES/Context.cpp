#include "nes.h"
#include "APU.h"
#include "Clock.h"
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

    static const uint32_t MASTER_CLOCK_PPU_DIVIDER_NTSC = 4;
    static const uint32_t MASTER_CLOCK_PPU_DIVIDER_PAL = 5;
    static const uint32_t MASTER_CLOCK_PPU_DIVIDER_DENDY = 5;

    static const uint32_t MASTER_CLOCK_PER_FRAME_NTSC = static_cast<uint32_t>((341 * 261 + 340.5) * MASTER_CLOCK_PPU_DIVIDER_NTSC);
    static const uint32_t MASTER_CLOCK_PER_FRAME_PAL = (341 * 312) * MASTER_CLOCK_PPU_DIVIDER_PAL;
    static const uint32_t MASTER_CLOCK_PER_FRAME_DENDY = (341 * 312) * MASTER_CLOCK_PPU_DIVIDER_PAL;

    static const uint32_t VISIBLE_LINES_NTSC = 240; //Real value is 224
    static const uint32_t VISIBLE_LINES_PAL = 240;
    static const uint32_t VISIBLE_LINES_DENDY = 240;

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

            // Clock
            if (!clock.create())
                return false;

            // CPU
            if (!cpuMemory.create(MEM_SIZE_LOG2, MEM_PAGE_SIZE_LOG2))
                return false;
            if (!cpu.create(clock, cpuMemory.getState(), MASTER_CLOCK_CPU_DIVIDER_NTSC))
                return false;

            const auto& romDesc = rom->getDescription();
            const auto& romContent = rom->getContent();

            // PPU
            uint32_t ppuCreateFlags = 0;
            if (romDesc.mirroring == NES::Rom::Mirroring_Horizontal)
                ppuCreateFlags |= NES::PPU::CREATE_VRAM_HORIZONTAL_MIRROR;
            else if (romDesc.mirroring == NES::Rom::Mirroring_Vertical)
                ppuCreateFlags |= NES::PPU::CREATE_VRAM_VERTICAL_MIRROR;
            else if (romDesc.mirroring == NES::Rom::Mirroring_FourScreen)
                ppuCreateFlags |= NES::PPU::CREATE_VRAM_FOUR_SCREEN;
            else
                ppuCreateFlags |= NES::PPU::CREATE_VRAM_ONE_SCREEN;
            if (!ppu.create(clock, MASTER_CLOCK_PPU_DIVIDER_NTSC, ppuCreateFlags, VISIBLE_LINES_NTSC))
                return false;

            // APU
            if (!apu.create(cpuMemory))
                return false;

            // ROM
            const uint8_t* romPage1 = romContent.prgRom;
            const uint8_t* romPage2 = romDesc.prgRomPages > 1 ? romPage1 + 0x4000 : romPage1;
            accessPrgRom1.setReadMemory(romPage1);
            accessPrgRom2.setReadMemory(romPage2);
            cpuMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_READ, 0x8000, 0xbfff, accessPrgRom1);
            cpuMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_READ, 0xc000, 0xffff, accessPrgRom2);

            // CHR-ROM
            const uint8_t* chrRomPage1 = romContent.chrRom;
            const uint8_t* chrRomPage2 = chrRomPage1 + 0x1000;
            ppu.getPatternTableRead(0)->setReadMemory(chrRomPage1);
            ppu.getPatternTableRead(1)->setReadMemory(chrRomPage2);

            // PPU registers
            accessPpuRegsRead.setReadMethod(ppuRegsRead, this, 0x2000);
            accessPpuRegsWrite.setWriteMethod(ppuRegsWrite, this, 0x2000);
            cpuMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_READ, 0x2000, 0x3fff, accessPpuRegsRead);
            cpuMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_WRITE, 0x2000, 0x3fff, accessPpuRegsWrite);

            if (!ppuListener.create(*this))
                return false;
            ppu.addListener(ppuListener);

            // APU registers
            static const uint16_t APU_START_ADDR = 0x4000;
            static const uint16_t APU_END_ADDR = APU_START_ADDR + NES::APU::APU_REGISTER_COUNT - 1;
            accessApuRegsRead.setReadMethod(apuRegsRead, this, APU_START_ADDR);
            accessApuRegsWrite.setWriteMethod(apuRegsWrite, this, APU_START_ADDR);
            cpuMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_READ, APU_START_ADDR, APU_END_ADDR, accessApuRegsRead);
            cpuMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_WRITE, APU_START_ADDR, APU_END_ADDR, accessApuRegsWrite);

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

            // Save RAM
            static const uint16_t SAVE_RAM_SIZE = 0x2000;
            static const uint16_t SAVE_RAM_START_ADDR = 0x6000;
            static const uint16_t SAVE_RAM_END_ADDR = SAVE_RAM_START_ADDR + SAVE_RAM_SIZE - 1;
            saveRam.resize(SAVE_RAM_SIZE, 0);
            accessSaveRamRead.setReadMemory(&saveRam[0]);
            accessSaveRamWrite.setWriteMemory(&saveRam[0]);
            cpuMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_READ, SAVE_RAM_START_ADDR, SAVE_RAM_END_ADDR, accessSaveRamRead);
            cpuMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_WRITE, SAVE_RAM_START_ADDR, SAVE_RAM_END_ADDR, accessSaveRamWrite);

            mapper = NES::MapperRegistry::getInstance().create(romDesc.mapper);
            if (!mapper)
                return false;
            NES::Mapper::Components components;
            components.rom = rom;
            components.memory = &cpuMemory;
            components.cpu = &cpu;
            components.ppu = &ppu;
            components.apu = &apu;
            if (!mapper->initialize(components))
                return false;

            reset();

#if 0
            // TODO: REMOVE THIS ONCE THE CPU AND MEMORY ARE MORE RELIABLE
            clock.setTargetExecution(100 * MASTER_CLOCK_CPU_DIVIDER_NTSC);
            clock.addEvent(startVBlank, this, 80 * MASTER_CLOCK_CPU_DIVIDER_NTSC);
            while (clock.canExecute())
            {
                clock.beginExecute();
                cpu.execute();
                ppu.execute();
                clock.endExecute();
            }
            reset();
#endif

            return true;
        }

        void destroy()
        {
            if (mapper)
            {
                mapper->dispose();
                mapper = nullptr;
            }

            apu.destroy();
            ppuListener.destroy();
            ppu.destroy();
            cpu.destroy();
            cpuMemory.destroy();
            clock.destroy();

            rom = nullptr;
        }

        virtual void dispose()
        {
            delete this;
        }

        virtual void reset()
        {
            clock.reset();
            cpu.reset();
            ppu.reset();
            apu.reset();
            mapper->reset();
        }

        virtual void setRenderSurface(void* surface, uint32_t pitch)
        {
            ppu.setRenderSurface(surface, pitch);
        }

        virtual void update()
        {
            clock.setTargetExecution(MASTER_CLOCK_PER_FRAME_NTSC);
            while (clock.canExecute())
            {
                clock.beginExecute();
                cpu.execute();
                ppu.execute();
                apu.execute();
                clock.endExecute();
            }
            clock.advance();
        }

        virtual uint8_t read8(uint16_t addr)
        {
            uint8_t value = memory_bus_read8(cpuMemory.getState(), clock.getDesiredTicks(), addr);
            return value;
        }

        virtual void write8(uint16_t addr, uint8_t value)
        {
            memory_bus_write8(cpuMemory.getState(), clock.getDesiredTicks(), addr, value);
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

        static uint8_t apuRegsRead(void* context, int32_t ticks, uint32_t addr)
        {
            return static_cast<ContextImpl*>(context)->apu.regRead(ticks, addr);
        }

        static void apuRegsWrite(void* context, int32_t ticks, uint32_t addr, uint8_t value)
        {
            static_cast<ContextImpl*>(context)->apu.regWrite(ticks, addr, value);
        }

        void onVBlankStart()
        {
            cpu.nmi();
        }

        class PPUListener : public NES::PPU::IListener
        {
        public:
            PPUListener()
                : mContext(nullptr)
            {
            }

            ~PPUListener()
            {
                destroy();
            }

            bool create(ContextImpl& context)
            {
                mContext = &context;
                return true;
            }

            void destroy()
            {
            }

            virtual void onVBlankStart()
            {
                mContext->onVBlankStart();
            }

        private:
            ContextImpl*    mContext;
        };

        const NES::Rom*         rom;
        NES::Clock              clock;
        NES::MemoryBus          cpuMemory;
        MEM_ACCESS              accessPrgRom1;
        MEM_ACCESS              accessPrgRom2;
        MEM_ACCESS              accessPpuRegsRead;
        MEM_ACCESS              accessPpuRegsWrite;
        MEM_ACCESS              accessApuRegsRead;
        MEM_ACCESS              accessApuRegsWrite;
        MEM_ACCESS              accessCpuRamRead;
        MEM_ACCESS              accessCpuRamWrite;
        MEM_ACCESS              accessSaveRamRead;
        MEM_ACCESS              accessSaveRamWrite;
        NES::Cpu6502            cpu;
        NES::PPU                ppu;
        PPUListener             ppuListener;
        NES::APU                apu;
        std::vector<uint8_t>    cpuRam;
        std::vector<uint8_t>    saveRam;
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