#include <Core/Serialization.h>
#include "Mappers.h"

namespace
{
    static const uint32_t ROM_BANK_SIZE = 0x4000;

    static const uint32_t BANK_SIZE_EXTERNAL_RAM = 0x2000;

    static const uint8_t MBC1_RAM_ENABLE_MASK = 0x0f;
    static const uint8_t MBC1_RAM_ENABLE_VALUE = 0x0a;

    static const uint8_t MBC1_ROM_BANK_LOW_MASK = 0x1f;
    static const uint8_t MBC1_ROM_BANK_HIGH_MASK = 0x60;
    static const uint8_t MBC1_ROM_BANK_HIGH_SHIFT = 5;
    static const uint8_t MBC1_ROM_BANK_INVALID_MASK = 0x60;
    static const uint8_t MBC1_ROM_BANK_INVALID_ADJ = 0x01;

    static const uint8_t MBC1_RAM_BANK_MODE = 0x01;

    uint8_t read8null(void* context, int32_t tick, uint32_t addr)
    {
        EMU_INVOKE_ONCE(printf("External RAM read disabled!\n"));
        return 0;
    }

    void write8null(void* context, int32_t tick, uint32_t addr, uint8_t value)
    {
        EMU_INVOKE_ONCE(printf("External RAM write disabled!\n"));
    }
}

namespace gb
{
    MapperBase::MapperBase()
    {
        initialize();
    }

    MapperBase::~MapperBase()
    {
        destroy();
    }

    void MapperBase::initialize()
    {
        mRom = nullptr;
        mMemory = nullptr;
        mEnableExternalRAM = false;
    }

    bool MapperBase::create(const Rom& rom, emu::MemoryBus& memory)
    {
        mRom = &rom;
        mMemory = &memory;

        const auto& desc = rom.getDescription();
        if (desc.hasRam)
        {
            mExternalRAM.resize(desc.ramSize, 0);
        }

        reset();

        EMU_VERIFY(mMemory->addMemoryRange(MEMORY_BUS::PAGE_TABLE_READ, 0x0000, 0x3fff, mMemoryROM[0]));
        EMU_VERIFY(mMemory->addMemoryRange(MEMORY_BUS::PAGE_TABLE_READ, 0x4000, 0x7fff, mMemoryROM[1]));
        if (!mExternalRAM.empty())
        {
            uint16_t size = static_cast<uint16_t>(mExternalRAM.size());
            if (size > BANK_SIZE_EXTERNAL_RAM)
                size = BANK_SIZE_EXTERNAL_RAM;
            EMU_VERIFY(mMemory->addMemoryRange(0xa000, 0xa000 + size - 1, mMemoryExternalRAM));
        }
        return true;
    }

    void MapperBase::destroy()
    {
        mExternalRAM.clear();
        initialize();
    }

    void MapperBase::reset()
    {
        mBankROM[0] = 0;
        mBankROM[1] = 1;
        mBankExternalRAM = 0xff;
        mEnableExternalRAM = false;
        updateMemoryMap();
    }

    void MapperBase::enableRam(bool enable)
    {
        mEnableExternalRAM = enable;
    }

    bool MapperBase::updateMemoryMap()
    {
        const auto& romContent = mRom->getContent();
        mMemoryROM[0].setReadMemory(romContent.rom + mBankROM[0] * ROM_BANK_SIZE);
        mMemoryROM[1].setReadMemory(romContent.rom + mBankROM[1] * ROM_BANK_SIZE);
        if (!mExternalRAM.empty())
        {
            if (mEnableExternalRAM)
                mMemoryExternalRAM.setReadWriteMemory(mExternalRAM.data());
            else
            {
                mMemoryExternalRAM.read.setReadMethod(&read8null, this);
                mMemoryExternalRAM.write.setWriteMethod(&write8null, this);
            }
        }
        return true;
    }

    void MapperBase::serializeGameData(emu::ISerializer& serializer)
    {
        if (mRom->getDescription().hasBattery)
            serializer.serialize(mExternalRAM);
    }

    void MapperBase::serializeGameState(emu::ISerializer& serializer)
    {
        serializer.serialize(mExternalRAM);
        serializer.serialize(mBankROM, EMU_ARRAY_SIZE(mBankROM));
        serializer.serialize(mBankExternalRAM);
        serializer.serialize(mEnableExternalRAM);
    }

    ////////////////////////////////////////////////////////////////////////////

    MapperROM::MapperROM()
    {
    }

    ////////////////////////////////////////////////////////////////////////////

    MapperMBC1::MapperMBC1()
    {
    }

    bool MapperMBC1::create(const Rom& rom, emu::MemoryBus& memory)
    {
        EMU_VERIFY(MapperBase::create(rom, memory));
        EMU_VERIFY(mMemory->addMemoryRange(MEMORY_BUS::PAGE_TABLE_WRITE, 0x0000, 0x7fff, mMemoryControlRegs.setWriteMethod(&MapperMBC1::write8, this)));
        return true;
    }

    void MapperMBC1::reset()
    {
        MapperBase::reset();
        mRamBankMode = false;
    }

    void MapperMBC1::serializeGameState(emu::ISerializer& serializer)
    {
        MapperBase::serializeGameState(serializer);
        serializer.serialize(mRamBankMode);
    }

    void MapperMBC1::write8(int32_t tick, uint32_t addr, uint8_t value)
    {
        switch (addr >> 13)
        {
        case 0:
        {
            bool enable = (value & MBC1_RAM_ENABLE_MASK) == MBC1_RAM_ENABLE_VALUE;
            enableRam(enable);
            break;
        }
        case 1:
        {
            uint8_t bank = getRomBank();
            bank = (bank & ~MBC1_ROM_BANK_LOW_MASK) | (value & MBC1_ROM_BANK_LOW_MASK);
            if ((bank & MBC1_ROM_BANK_INVALID_MASK) == bank)
                bank |= MBC1_ROM_BANK_INVALID_ADJ;
            setRomBank(bank);
            break;
        }
        case 2:
        {
            uint8_t bank = getRomBank();
            bank = (bank & ~MBC1_ROM_BANK_HIGH_MASK) | ((value << MBC1_ROM_BANK_HIGH_SHIFT) & MBC1_ROM_BANK_HIGH_MASK);
            if ((bank & MBC1_ROM_BANK_INVALID_MASK) == bank)
                bank |= MBC1_ROM_BANK_INVALID_ADJ;
            setRomBank(bank);
            break;
        }
        case 3:
        {
            mRamBankMode = (value & MBC1_RAM_BANK_MODE) != 0;
            break;
        }
        default:
            EMU_ASSERT(false);
        }

        updateMemoryMap();
    }

    ////////////////////////////////////////////////////////////////////////////

    IMapper* createMapper(const Rom& rom, emu::MemoryBus& memory)
    {
        const auto& desc = rom.getDescription();

        switch (desc.mapper)
        {
        case Rom::Mapper::ROM:
        {
            auto mapper = new MapperROM();
            if (mapper->create(rom, memory))
                return mapper;
            delete mapper;
            break;
        }

        case Rom::Mapper::MBC1:
        {
            auto mapper = new MapperMBC1();
            if (mapper->create(rom, memory))
                return mapper;
            delete mapper;
            break;
        }

        default:
            EMU_NOT_IMPLEMENTED();
        }
        return nullptr;
    }
}
