#include <Core/MemoryBus.h>
#include <Core/Serializer.h>
#include "Mappers.h"
#include "nes.h"
#include "PPU.h"
#include <vector>

namespace nes
{
    class Mapper2 : public nes::IMapper
    {
    public:
        virtual void dispose()
        {
            delete this;
        }

        virtual bool initialize(const Components& components)
        {
            if (!components.memory)
                return false;
            auto& cpuMemory = *components.memory;

            // PRG ROM
            if (!components.rom)
                return false;
            mRom = components.rom;
            const auto& romDescription = mRom->getDescription();
            const auto& romContent = mRom->getContent();
            EMU_UNUSED(romDescription);
            EMU_UNUSED(romContent);
            mMemPrgRomWrite.setWriteMethod(regWrite, this);

            // CHR RAM
            if (!components.ppu)
                return false;
            mPpu = components.ppu;
            auto& ppuMemory = mPpu->getMemory();
            mChrRam.resize(8 * 1024);
            mMemChrRamRead.setReadMemory(&mChrRam[0]);
            mMemChrRamWrite.setWriteMemory(&mChrRam[0]);

            // Load banks
            reset();

            // Enable memory ranges
            cpuMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_READ, 0x8000, 0xbfff, mMemPrgRomRead[0]);
            cpuMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_READ, 0xc000, 0xffff, mMemPrgRomRead[1]);
            cpuMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_WRITE, 0x8000, 0xffff, mMemPrgRomWrite);
            ppuMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_READ, 0x0000, 0x1fff, mMemChrRamRead);
            ppuMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_WRITE, 0x0000, 0x1fff, mMemChrRamWrite);

            return true;
        }

        virtual void reset()
        {
            mRegister = 0x00;
            updateMemoryMap();
        }

        virtual void serializeGameState(emu::ISerializer& serializer)
        {
            uint32_t version = 1;
            serializer
                .value("Version", version)
                .value("ChrRam", mChrRam)
                .value("Register", mRegister);
            updateMemoryMap();
        }

    private:
        void initialize()
        {
            mRom = nullptr;
            mPpu = nullptr;
            mRegister = 0;
        }

        void updateMemoryMap()
        {
            const auto& romDescription = mRom->getDescription();
            const auto& romContent = mRom->getContent();
            
            uint32_t prgBank0 = mRegister & 0xff;
            uint32_t prgBank1 = romDescription.prgRomPages - 1;

            EMU_ASSERT(prgBank0 < romDescription.prgRomPages);
            mMemPrgRomRead[0].setReadMemory(&romContent.prgRom[16 * 1024 * prgBank0]);
            mMemPrgRomRead[1].setReadMemory(&romContent.prgRom[16 * 1024 * prgBank1]);
        }

        void regWrite(uint32_t addr, uint8_t value)
        {
            EMU_UNUSED(addr);
            mRegister = value;
            updateMemoryMap();
        }

        static void regWrite(void* context, int32_t ticks, uint32_t addr, uint8_t value)
        {
            EMU_UNUSED(ticks);
            static_cast<Mapper2*>(context)->regWrite(addr, value);
        }

        const nes::Rom*         mRom;
        nes::PPU*               mPpu;
        MEM_ACCESS              mMemPrgRomRead[2];
        MEM_ACCESS              mMemPrgRomWrite;
        MEM_ACCESS              mMemChrRamRead;
        MEM_ACCESS              mMemChrRamWrite;
        emu::Buffer             mChrRam;
        uint8_t                 mRegister;
    };
}
