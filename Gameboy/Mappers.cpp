#include <Core/Serialization.h>
#include "Mappers.h"

namespace
{
    static const uint32_t ROM_BANK_SIZE = 0x4000;

    static const uint32_t BANK_SIZE_EXTERNAL_RAM = 0x2000;
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
    }

    bool MapperBase::create(const Rom& rom, emu::MemoryBus& memory, uint32_t ramSize, bool battery)
    {
        mRom = &rom;
        mMemory = &memory;

        mBankROM[0] = 0;
        mBankROM[1] = 1;
        mBankExternalRAM = 0xff;

        EMU_VERIFY(updateMemoryMap());
        EMU_VERIFY(mMemory->addMemoryRange(MEMORY_BUS::PAGE_TABLE_READ, 0x0000, 0x3fff, mMemoryROM[0]));
        EMU_VERIFY(mMemory->addMemoryRange(MEMORY_BUS::PAGE_TABLE_READ, 0x4000, 0x7fff, mMemoryROM[1]));
        if (!mExternalRAM.empty())
        {
            EMU_VERIFY(mMemory->addMemoryRange(0xa000, 0xbfff, mMemoryExternalRAM));
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
    }

    bool MapperBase::updateMemoryMap()
    {
        const auto& romContent = mRom->getContent();
        mMemoryROM[0].setReadMemory(romContent.rom + mBankROM[0] * ROM_BANK_SIZE);
        mMemoryROM[1].setReadMemory(romContent.rom + mBankROM[1] * ROM_BANK_SIZE);
        if (!mExternalRAM.empty())
            mMemoryExternalRAM.setReadWriteMemory(mExternalRAM.data());
        return true;
    }

    void MapperBase::serializeGameData(emu::ISerializer& serializer)
    {
    }

    void MapperBase::serializeGameState(emu::ISerializer& serializer)
    {
        serializer.serialize(mExternalRAM);
        serializer.serialize(mBankROM, EMU_ARRAY_SIZE(mBankROM));
        serializer.serialize(mBankExternalRAM);
    }

    ////////////////////////////////////////////////////////////////////////////

    MapperROM::MapperROM()
    {
    }

    MapperROM* MapperROM::allocate(const Rom& rom, emu::MemoryBus& memory, uint32_t ramSize, bool battery)
    {
        auto mapper = new MapperROM();
        if (!mapper->create(rom, memory, ramSize, battery))
        {
            delete mapper;
            mapper = nullptr;
        }
        return mapper;
    }
}
