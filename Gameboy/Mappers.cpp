#include <Core/Core.h>
#include "Mappers.h"

namespace
{
    static const uint32_t ROM_BANK_SIZE = 0x4000;

    static const uint32_t BANK_SIZE_EXTERNAL_RAM = 0x2000;

    static const uint8_t MBC1_RAM_ENABLE_MASK = 0x0f;
    static const uint8_t MBC1_RAM_ENABLE_VALUE = 0x0a;

    static const uint8_t MBC1_ROM_BANK_LOW_MASK = 0x1f;
    static const uint8_t MBC1_ROM_BANK_HIGH_VALID = 0x60;
    static const uint8_t MBC1_ROM_BANK_HIGH_MASK = 0x60;
    static const uint8_t MBC1_ROM_BANK_HIGH_SHIFT = 5;
    static const uint8_t MBC1_ROM_BANK_INVALID_MASK = 0x1f;
    static const uint8_t MBC1_ROM_BANK_INVALID_ADJ = 0x01;

    static const uint8_t MBC1_RAM_BANK_MODE = 0x01;

    static const uint8_t MBC5_RAM_ENABLE_MASK = 0x0f;
    static const uint8_t MBC5_RAM_ENABLE_VALUE = 0x0a;

    static const uint16_t MBC5_ROM_BANK_LOW_MASK = 0x00ff;
    static const uint16_t MBC5_ROM_BANK_HIGH_MASK = 0x0100;
    static const uint16_t MBC5_ROM_BANK_HIGH_SHIFT = 8;

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
        uint8_t* pRAM = nullptr;
        if (desc.hasRam)
        {
            mExternalRAM.resize(desc.ramSize, 0);
            pRAM = mExternalRAM.data();
        }

        const uint8_t* pROM = mRom->getContent().rom;
        for (uint32_t bank = 0, offset = 0; bank < EMU_ARRAY_SIZE(mBankMapROM); ++bank, offset += ROM_BANK_SIZE)
        {
            if (offset >= desc.romSize)
                offset = 0;
            mBankMapROM[bank] = pROM + offset;
        }
        for (uint32_t bank = 0, offset = 0; bank < EMU_ARRAY_SIZE(mBankMapRAM); ++bank, offset += BANK_SIZE_EXTERNAL_RAM)
        {
            if (offset >= desc.ramSize)
                offset = 0;
            mBankMapRAM[bank] = pRAM + offset;
        }

        reset();

        EMU_VERIFY(mMemory->addMemoryRange(MEMORY_BUS::PAGE_TABLE_READ, 0x0000, 0x3fff, mMemoryROM[0]));
        EMU_VERIFY(mMemory->addMemoryRange(MEMORY_BUS::PAGE_TABLE_READ, 0x4000, 0x7fff, mMemoryROM[1]));
        uint16_t size = 0;
        if (!mExternalRAM.empty())
        {
            size = static_cast<uint16_t>(mExternalRAM.size());
            if (size > BANK_SIZE_EXTERNAL_RAM)
                size = BANK_SIZE_EXTERNAL_RAM;
            EMU_VERIFY(mMemory->addMemoryRange(0xa000, 0xa000 + size - 1, mMemoryExternalRAM));
        }
        if (size < BANK_SIZE_EXTERNAL_RAM)
        {
            EMU_VERIFY(mMemory->addMemoryRange(0xa000, 0xa000 + BANK_SIZE_EXTERNAL_RAM - size - 1, mMemoryExternalRAMEmpty));
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
        mBankExternalRAM = 0x00;
        mEnableExternalRAM = false;
        updateMemoryMap();
    }

    void MapperBase::enableRam(bool enable)
    {
        mEnableExternalRAM = enable;
    }

    bool MapperBase::updateMemoryMap()
    {
        mMemoryROM[0].setReadMemory(mBankMapROM[mBankROM[0]]);
        mMemoryROM[1].setReadMemory(mBankMapROM[mBankROM[1]]);
        if (!mExternalRAM.empty() && (mEnableExternalRAM || !mRom->getDescription().hasBattery))
        {
            mMemoryExternalRAM.setReadWriteMemory(mBankMapRAM[mBankExternalRAM]);
        }
        else
        {
            mMemoryExternalRAM.read.setReadMethod(&read8null, this);
            mMemoryExternalRAM.write.setWriteMethod(&write8null, this);
        }
        mMemoryExternalRAMEmpty.read.setReadMethod(&read8null, this);
        mMemoryExternalRAMEmpty.write.setWriteMethod(&write8null, this);
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
        updateMemoryMap();
    }

    ////////////////////////////////////////////////////////////////////////////

    MapperROM::MapperROM()
    {
    }

    bool MapperROM::create(const Rom& rom, emu::MemoryBus& memory)
    {
        EMU_VERIFY(MapperBase::create(rom, memory));
        EMU_VERIFY(mMemory->addMemoryRange(MEMORY_BUS::PAGE_TABLE_WRITE, 0x0000, 0x7fff, mMemoryControlRegs.setWriteMethod(&write8, this)));
        return true;
    }

    void MapperROM::write8(void* context, int32_t tick, uint32_t addr, uint8_t value)
    {
        EMU_NOT_IMPLEMENTED();
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
        mBankROM = 0;
        mBankRAM = 0;
        mRamBankMode = false;
        MapperBase::reset();
    }

    void MapperMBC1::serializeGameState(emu::ISerializer& serializer)
    {
        serializer.serialize(mBankROM);
        serializer.serialize(mBankRAM);
        serializer.serialize(mRamBankMode);
        MapperBase::serializeGameState(serializer);
    }

    bool MapperMBC1::updateMemoryMap()
    {
        uint8_t validBankROM = mBankROM;
        if ((validBankROM & MBC1_ROM_BANK_INVALID_MASK) == 0)
            validBankROM |= MBC1_ROM_BANK_INVALID_ADJ;
        setRomBank(validBankROM);
        setRamBank(mBankRAM);
        return MapperBase::updateMemoryMap();
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
            mBankROM = (mBankROM & ~MBC1_ROM_BANK_LOW_MASK) | (value & MBC1_ROM_BANK_LOW_MASK);
            break;
        }
        case 2:
        {
            value = value & MBC1_ROM_BANK_HIGH_VALID;
            if (mRamBankMode)
            {
                mBankRAM = value;
            }
            else
            {
                mBankROM = (mBankROM & ~MBC1_ROM_BANK_HIGH_MASK) | ((value << MBC1_ROM_BANK_HIGH_SHIFT) & MBC1_ROM_BANK_HIGH_MASK);
            }
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

    MapperMBC5::MapperMBC5()
    {
    }

    bool MapperMBC5::create(const Rom& rom, emu::MemoryBus& memory)
    {
        EMU_VERIFY(MapperBase::create(rom, memory));
        EMU_VERIFY(mMemory->addMemoryRange(MEMORY_BUS::PAGE_TABLE_WRITE, 0x0000, 0x7fff, mMemoryControlRegs.setWriteMethod(&MapperMBC5::write8, this)));
        return true;
    }

    void MapperMBC5::reset()
    {
        mRamBankMode = false;
        MapperBase::reset();
    }

    void MapperMBC5::serializeGameState(emu::ISerializer& serializer)
    {
        serializer.serialize(mRamBankMode);
        MapperBase::serializeGameState(serializer);
    }

    bool MapperMBC5::updateMemoryMap()
    {
        return MapperBase::updateMemoryMap();
    }

    void MapperMBC5::write8(int32_t tick, uint32_t addr, uint8_t value)
    {
        switch (addr >> 12)
        {
        case 0:
        case 1:
        {
            bool enable = (value & MBC5_RAM_ENABLE_MASK) == MBC5_RAM_ENABLE_VALUE;
            enableRam(enable);
            break;
        }
        case 2:
        {
            uint16_t bank = getRomBank();
            bank = (bank & ~MBC5_ROM_BANK_LOW_MASK) | (value & MBC5_ROM_BANK_LOW_MASK);
            setRomBank(bank);
            break;
        }
        case 3:
        {
            uint16_t bank = getRomBank();
            bank = (bank & ~MBC5_ROM_BANK_HIGH_MASK) | ((value << MBC5_ROM_BANK_HIGH_SHIFT) & MBC5_ROM_BANK_HIGH_MASK);
            setRomBank(bank);
            break;
        }
        case 4:
        case 5:
        {
            setRamBank(value);
            break;
        }
        default:
            break;
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

        case Rom::Mapper::MBC5:
        {
            auto mapper = new MapperMBC5();
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
