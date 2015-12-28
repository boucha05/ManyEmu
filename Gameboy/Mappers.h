#pragma once

#include <Core/Core.h>
#include <Core/RegisterBank.h>
#include "GB.h"

namespace gb
{
    class IMapper
    {
    public:
        virtual ~IMapper() { }
        virtual void reset() = 0;
        virtual void serializeGameData(emu::ISerializer& serializer) = 0;
        virtual void serializeGameState(emu::ISerializer& serializer) = 0;
    };

    class MapperBase : public IMapper
    {
    public:
        MapperBase();
        virtual ~MapperBase() override;
        virtual bool create(const Rom& rom, emu::MemoryBus& memory, uint32_t ramSize, bool battery);
        virtual void destroy();
        virtual void reset() override;
        virtual void serializeGameData(emu::ISerializer& serializer) override;
        virtual void serializeGameState(emu::ISerializer& serializer) override;

    private:
        void initialize();
        virtual bool updateMemoryMap();

        const Rom*              mRom;
        emu::MemoryBus*         mMemory;
        MEM_ACCESS              mMemoryROM[2];
        MEM_ACCESS_READ_WRITE   mMemoryExternalRAM;
        std::vector<uint8_t>    mExternalRAM;
        uint8_t                 mBankROM[2];
        uint8_t                 mBankExternalRAM;
    };

    class MapperROM : public MapperBase
    {
    public:
        MapperROM();

        static MapperROM* allocate(const Rom& rom, emu::MemoryBus& memory, uint32_t ramSize, bool battery);
    };
}
