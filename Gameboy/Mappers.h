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
        virtual bool create(const Rom& rom, emu::MemoryBus& memory);
        virtual void destroy();
        virtual void reset() override;
        virtual void serializeGameData(emu::ISerializer& serializer) override;
        virtual void serializeGameState(emu::ISerializer& serializer) override;

        uint8_t getRomBank()
        {
            return mBankROM[1];
        }

        void setRomBank(uint8_t value)
        {
            mBankROM[1] = value;
        }

        uint8_t getRamBank()
        {
            return mBankExternalRAM;
        }

        void setRamBank(uint8_t value)
        {
            mBankExternalRAM = value;
        }

    protected:
        void initialize();
        virtual bool updateMemoryMap();
        void enableRam(bool enable);

        const Rom*              mRom;
        emu::MemoryBus*         mMemory;
        MEM_ACCESS              mMemoryROM[2];
        MEM_ACCESS_READ_WRITE   mMemoryExternalRAM;
        std::vector<uint8_t>    mExternalRAM;
        uint8_t                 mBankROM[2];
        uint8_t                 mBankExternalRAM;
        bool                    mEnableExternalRAM;
    };

    class MapperROM : public MapperBase
    {
    public:
        MapperROM();
    };

    class MapperMBC1 : public MapperBase
    {
    public:
        MapperMBC1();
        virtual bool create(const Rom& rom, emu::MemoryBus& memory) override;
        virtual void reset() override;
        virtual void serializeGameState(emu::ISerializer& serializer) override;

    private:
        void write8(int32_t tick, uint32_t addr, uint8_t value);

        static void write8(void* context, int32_t tick, uint32_t addr, uint8_t value)
        {
            static_cast<MapperMBC1*>(context)->write8(tick, addr, value);
        }

        MEM_ACCESS              mMemoryControlRegs;
        bool                    mRamBankMode;
    };

    IMapper* createMapper(const Rom& rom, emu::MemoryBus& memory);
}
