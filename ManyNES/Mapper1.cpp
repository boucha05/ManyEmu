#include "Mappers.h"
#include "MemoryBus.h"
#include "PPU.h"
#include <assert.h>
#include <vector>

namespace
{
    static const size_t PRG_ROM_PAGE_SIZE = 16 * 1024;
    static const size_t CHR_ROM_PAGE_SIZE = 4 * 1024;
    static const size_t PRG_RAM_SIZE = 8 * 1024;
}

namespace NES
{
    class MMC1
    {
    public:
        MMC1()
        {
            initialize();
        }

        ~MMC1()
        {
            destroy();
        }

        bool create(const Mapper::Components& components)
        {
            if (!components.memory)
                return false;
            auto& cpu = *components.memory;

            // PRG ROM
            if (!components.rom)
                return false;
            mRom = components.rom;
            const auto& romDescription = mRom->getDescription();
            const auto& romContent = mRom->getContent();
            mMemPrgRomRead[0].setReadMethod(unsupportedRead, nullptr);
            mMemPrgRomRead[1].setReadMethod(unsupportedRead, nullptr);
            mMemPrgRomWrite.setWriteMethod(regWrite, this);
            cpu.addMemoryRange(MEMORY_BUS::PAGE_TABLE_READ, 0x8000, 0xbfff, mMemPrgRomRead[0]);
            cpu.addMemoryRange(MEMORY_BUS::PAGE_TABLE_READ, 0xc000, 0xffff, mMemPrgRomRead[1]);
            cpu.addMemoryRange(MEMORY_BUS::PAGE_TABLE_WRITE, 0x8000, 0xffff, mMemPrgRomWrite);

            // PRG RAM
            mPrgRam.resize(PRG_RAM_SIZE);
            mMemPrgRamRead.setReadMethod(unsupportedRead, nullptr);
            mMemPrgRamRead.setWriteMethod(unsupportedWrite, nullptr);
            cpu.addMemoryRange(MEMORY_BUS::PAGE_TABLE_READ, 0x6000, 0x7fff, mMemPrgRamRead);
            cpu.addMemoryRange(MEMORY_BUS::PAGE_TABLE_WRITE, 0x6000, 0x7fff, mMemPrgRamWrite);

            // PPU CHR ROM
            if (!components.ppu)
                return false;
            auto& ppu = components.ppu->getMemory();
            mMemChrRomRead[0].setReadMethod(unsupportedRead, nullptr);
            mMemChrRomRead[1].setReadMethod(unsupportedRead, nullptr);
            mMemChrRomWrite[0].setWriteMethod(unsupportedWrite, nullptr);
            mMemChrRomWrite[1].setWriteMethod(unsupportedWrite, nullptr);
            ppu.addMemoryRange(MEMORY_BUS::PAGE_TABLE_READ, 0x0000, 0x0fff, mMemChrRomRead[0]);
            ppu.addMemoryRange(MEMORY_BUS::PAGE_TABLE_READ, 0x1000, 0x1fff, mMemChrRomRead[1]);
            ppu.addMemoryRange(MEMORY_BUS::PAGE_TABLE_WRITE, 0x0000, 0x0fff, mMemChrRomWrite[0]);
            ppu.addMemoryRange(MEMORY_BUS::PAGE_TABLE_WRITE, 0x1000, 0x1fff, mMemChrRomWrite[1]);

            // CHR RAM
            if (romDescription.prgRamPages == 0)
                mChrRam.resize(8 * 1024);

            reset();

            return true;
        }

        void destroy()
        {
            mPrgRam.clear();
            mChrRam.clear();
            initialize();
        }

        void reset()
        {
            mShift = 0;

            const auto& romDescription = mRom->getDescription();
            const auto& romContent = mRom->getContent();

            mMemPrgRomRead[0].setReadMemory(&romContent.prgRom[0]);
            mMemPrgRomRead[1].setReadMemory(&romContent.prgRom[PRG_ROM_PAGE_SIZE * (romDescription.prgRomPages - 1)]);

            mMemChrRomRead[0].setReadMemory(&romContent.chrRom[0]);
            mMemChrRomRead[1].setReadMemory(&romContent.chrRom[1 * CHR_ROM_PAGE_SIZE]);

            mRegister[0] = 0x0c;
            mRegister[1] = 0x00;
            mRegister[2] = 0x00;
            mRegister[3] = 0x00;

            updateMemoryMap();
        }

    private:
        void initialize()
        {
            mRom = nullptr;
            mShift = 0;
            mCycle = 0;
            mPrgRomPage[0] = mPrgRomPage[1] = 0;
            mChrRomPage[0] = mChrRomPage[1] = 0;
            mRegister[0] = mRegister[1] = mRegister[2] = mRegister[3];
        }

        void updateMemoryMap()
        {
            uint32_t mirroring = mRegister[0] & 0x03;
            uint32_t prgRomMode = (mRegister[0] >> 2) & 0x03;
            uint32_t chrBank0 = mRegister[1] & 0x1f;
            uint32_t chrBank1 = mRegister[2] & 0x1f;
            uint32_t prgBank = mRegister[3] & 0x0f;

            bool chrRom8KbMode = (mRegister[0] & 0x10) == 0;
            bool prgRamEnable = (mRegister[3] & 0x10) == 0;

            if (chrRom8KbMode)
            {
                chrBank0 &= ~1;
                chrBank1 = chrBank0 | 1;
            }

            const auto& romDescription = mRom->getDescription();
            const auto& romContent = mRom->getContent();

            if (romDescription.chrRomPages == 0)
            {
                // SNROM variant : most significant bit of chr ram page toggle PRG RAM
                prgRamEnable = ((chrBank0 | chrBank1) & 0x10) ? prgRamEnable : false;
                chrBank0 &= ~0x10;
                chrBank1 &= ~0x10;
            }

            switch (prgRomMode)
            {
            case 0:
            case 1:
                mPrgRomPage[0] = prgBank;
                mPrgRomPage[1] = mPrgRomPage[0] + 1;
                break;

            case 2:
                mPrgRomPage[0] = 0;
                mPrgRomPage[1] = prgBank;
                break;

            case 3:
                mPrgRomPage[0] = prgBank;
                mPrgRomPage[1] = romDescription.prgRomPages - 1;
                break;
            }

            if (romDescription.chrRomPages == 0)
            {
                assert(chrBank0 < 2);
                assert(chrBank1 < 2);
                mMemChrRomRead[0].setReadMemory(&mChrRam[4 * 1024 * chrBank0]);
                mMemChrRomRead[1].setReadMemory(&mChrRam[4 * 1024 * chrBank1]);
                mMemChrRomWrite[0].setWriteMemory(&mChrRam[4 * 1024 * chrBank0]);
                mMemChrRomWrite[1].setWriteMemory(&mChrRam[4 * 1024 * chrBank1]);
            }
            else
            {
                assert(chrBank0 < romDescription.chrRomPages * 2);
                assert(chrBank1 < romDescription.chrRomPages * 2);
                mMemChrRomRead[0].setReadMemory(&romContent.chrRom[4 * 1024 * chrBank0]);
                mMemChrRomRead[1].setReadMemory(&romContent.chrRom[4 * 1024 * chrBank1]);
                mMemChrRomWrite[0].setWriteMethod(unsupportedWrite, nullptr);
                mMemChrRomWrite[1].setWriteMethod(unsupportedWrite, nullptr);
            }

            assert(mPrgRomPage[0] < romDescription.prgRomPages);
            assert(mPrgRomPage[1] < romDescription.prgRomPages);
            mMemPrgRomRead[0].setReadMemory(&romContent.prgRom[16 * 1024 * mPrgRomPage[0]]);
            mMemPrgRomRead[1].setReadMemory(&romContent.prgRom[16 * 1024 * mPrgRomPage[1]]);
        }

        void regWrite(uint32_t addr, uint8_t value)
        {
            if (value & 0x80)
            {
                mShift = 0;
                mCycle = 0;
            }
            else
            {
                mShift >>= 1;
                mShift |= (value & 1) ? 0x10 : 0x00;
                if (++mCycle >= 5)
                {
                    uint32_t index = (addr >> 13) & 3;
                    mRegister[index] = mShift;
                    onRegisterWrite(index, mShift);
                    mShift = 0;
                    mCycle = 0;
                }
            }
        }

        static void regWrite(void* context, int32_t ticks, uint32_t addr, uint8_t value)
        {
            static_cast<MMC1*>(context)->regWrite(addr, value);
        }

        static uint8_t unsupportedRead(void* context, int32_t ticks, uint32_t addr)
        {
            assert(0);
            return 0;
        }

        static void unsupportedWrite(void* context, int32_t ticks, uint32_t addr, uint8_t value)
        {
            assert(0);
        }

        void onRegisterWrite(uint32_t index, uint32_t value)
        {
            switch (index)
            {
            case 0:
            case 1:
            case 2:
            case 3:
                mRegister[index] = value;
                break;

            default:
                assert(false);
            }
            updateMemoryMap();
        }

        const NES::Rom*         mRom;
        MEM_ACCESS              mMemPrgRomRead[2];
        MEM_ACCESS              mMemPrgRomWrite;
        MEM_ACCESS              mMemPrgRamRead;
        MEM_ACCESS              mMemPrgRamWrite;
        MEM_ACCESS              mMemChrRomRead[2];
        MEM_ACCESS              mMemChrRomWrite[2];
        std::vector<uint8_t>    mChrRam;
        std::vector<uint8_t>    mPrgRam;
        uint32_t                mShift;
        uint32_t                mCycle;
        uint32_t                mPrgRomPage[2];
        uint32_t                mChrRomPage[2];
        uint8_t                 mRegister[4];
    };
}

namespace
{
    class Mapper : public NES::Mapper
    {
    public:
        virtual void dispose()
        {
            delete this;
        }

        virtual bool initialize(const Components& components)
        {
            auto& romDesc = components.rom->getDescription();
            bool prgRomLarge = romDesc.prgRomPages * PRG_ROM_PAGE_SIZE > 256 * 1024;
            bool chrRomLarge = romDesc.chrRomPages * CHR_ROM_PAGE_SIZE > 8 * 1024;

            if (!mMMC1.create(components))
                return false;

            return true;
        }

        virtual void reset()
        {
            mMMC1.reset();
        }

    private:
        NES::MMC1   mMMC1;
    };

    NES::AutoRegisterMapper<Mapper> mapper(1, "SxROM");
}
