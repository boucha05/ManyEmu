#include <Core/Clock.h>
#include <Core/MemoryBus.h>
#include <Core/Serialization.h>
#include "nes.h"
#include "APU.h"
#include "Cpu6502.h"
#include "Mappers.h"
#include "nes.h"
#include "PPU.h"
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

    static const uint32_t MASTER_CLOCK_APU_DIVIDER_NTSC = 12;
    static const uint32_t MASTER_CLOCK_APU_DIVIDER_PAL = 16;
    static const uint32_t MASTER_CLOCK_APU_DIVIDER_DENDY = 15;

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
        EMU_ASSERT(false);
    }
}

namespace
{
    class ContextImpl : public nes::Context
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

        bool create(const nes::Rom& _rom)
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
            if (romDesc.mirroring == nes::Rom::Mirroring_Horizontal)
                ppuCreateFlags |= nes::PPU::CREATE_VRAM_HORIZONTAL_MIRROR;
            else if (romDesc.mirroring == nes::Rom::Mirroring_Vertical)
                ppuCreateFlags |= nes::PPU::CREATE_VRAM_VERTICAL_MIRROR;
            else if (romDesc.mirroring == nes::Rom::Mirroring_FourScreen)
                ppuCreateFlags |= nes::PPU::CREATE_VRAM_FOUR_SCREEN;
            else
                ppuCreateFlags |= nes::PPU::CREATE_VRAM_ONE_SCREEN;
            if (!ppu.create(clock, MASTER_CLOCK_PPU_DIVIDER_NTSC, ppuCreateFlags, VISIBLE_LINES_NTSC))
                return false;

            // IRQ
            irqApu = false;
            irqMapper = false;

            // APU
            if (!apu.create(clock, cpuMemory, MASTER_CLOCK_APU_DIVIDER_NTSC, MASTER_CLOCK_FREQUENCY_NTSC))
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
            static const uint16_t APU_END_ADDR = APU_START_ADDR + nes::APU::APU_REGISTER_COUNT - 1;
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

            // Mapper
            if (!mapperListener.create(*this))
                return false;
            mapper = nes::MapperRegistry::getInstance().create(romDesc.mapper);
            if (!mapper)
                return false;
            nes::IMapper::Components components;
            components.rom = rom;
            components.clock = &clock;
            components.memory = &cpuMemory;
            components.cpu = &cpu;
            components.ppu = &ppu;
            components.apu = &apu;
            components.listener = &mapperListener;
            if (!mapper->initialize(components))
                return false;

            reset();
            return true;
        }

        void destroy()
        {
            if (mapper)
            {
                mapper->dispose();
                mapper = nullptr;
            }
            mapperListener.destroy();

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
            irqApu = false;
            irqMapper = false;
            updateIrqStatus();
        }

        virtual void setController(uint32_t index, uint32_t buttons)
        {
            apu.setController(index, static_cast<uint8_t>(buttons));
        }

        virtual void setSoundBuffer(int16_t* buffer, size_t size)
        {
            apu.setSoundBuffer(buffer, size);
        }

        virtual void setRenderSurface(void* surface, size_t pitch)
        {
            ppu.setRenderSurface(surface, pitch);
        }

        virtual void update()
        {
            ppu.beginFrame();
            apu.beginFrame();
            mapper->beginFrame();
            clock.execute(MASTER_CLOCK_PER_FRAME_NTSC);
            clock.advance();
            clock.clearEvents();
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

        virtual void serializeGameData(emu::ISerializer& serializer)
        {
            mapper->serializeGameData(serializer);
        }

        virtual void serializeGameState(emu::ISerializer& serializer)
        {
            uint32_t version = 2;
            clock.serialize(serializer);
            cpu.serialize(serializer);
            ppu.serialize(serializer);
            apu.serialize(serializer);
            serializer.serialize(cpuRam);
            serializer.serialize(saveRam);
            if (version >= 2)
            {
                serializer.serialize(irqApu);
                serializer.serialize(irqMapper);
            }
            mapper->serializeGameState(serializer);
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

        void updateApuIrq(bool active)
        {
            irqApu = active;
            updateIrqStatus();
        }

        void updateMapperIrq(bool active)
        {
            irqMapper = active;
            updateIrqStatus();
        }

        void updateIrqStatus()
        {
            bool active = irqApu | irqMapper;
            cpu.irq(active);
        }

        class PPUListener : public nes::PPU::IListener
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

        class MapperListener : public nes::IMapper::IListener
        {
        public:
            MapperListener()
                : mContext(nullptr)
            {
            }

            ~MapperListener()
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

            virtual void onIrqUpdate(bool active)
            {
                mContext->updateMapperIrq(active);
            }

        private:
            ContextImpl*    mContext;
        };

        const nes::Rom*         rom;
        emu::Clock              clock;
        emu::MemoryBus          cpuMemory;
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
        bool                    irqApu;
        bool                    irqMapper;
        nes::Cpu6502            cpu;
        nes::PPU                ppu;
        PPUListener             ppuListener;
        nes::APU                apu;
        std::vector<uint8_t>    cpuRam;
        std::vector<uint8_t>    saveRam;
        MapperListener          mapperListener;
        nes::IMapper*           mapper;
    };
}

namespace nes
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