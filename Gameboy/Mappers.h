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

        uint16_t getRomBank()
        {
            return static_cast<uint16_t>(mBankROM[1]);
        }

        void setRomBank(uint16_t value)
        {
            mBankROM[1] = value;
        }

        uint16_t getRamBank()
        {
            return static_cast<uint16_t>(mBankExternalRAM);
        }

        void setRamBank(uint16_t value)
        {
            mBankExternalRAM = value;
        }

    protected:
        void initialize();
        virtual bool updateMemoryMap();
        void enableRam(bool enable);

        const Rom*              mRom;
        emu::MemoryBus*         mMemory;
        const uint8_t*          mBankMapROM[512];
        uint8_t*                mBankMapRAM[256];
        MEM_ACCESS              mMemoryROM[2];
        MEM_ACCESS_READ_WRITE   mMemoryExternalRAM;
        MEM_ACCESS_READ_WRITE   mMemoryExternalRAMEmpty;
        emu::Buffer             mExternalRAM;
        uint32_t                mBankROM[2];
        uint32_t                mBankExternalRAM;
        bool                    mEnableExternalRAM;
    };

    class MapperROM : public MapperBase
    {
    public:
        MapperROM();
        virtual bool create(const Rom& rom, emu::MemoryBus& memory) override;

    private:
        static void write8(void* context, int32_t tick, uint32_t addr, uint8_t value);

        MEM_ACCESS              mMemoryControlRegs;
    };

    class MapperMBC1 : public MapperBase
    {
    public:
        MapperMBC1();
        virtual bool create(const Rom& rom, emu::MemoryBus& memory) override;
        virtual void reset() override;
        virtual void serializeGameState(emu::ISerializer& serializer) override;

    protected:
        virtual bool updateMemoryMap() override;
    private:
        void write8(int32_t tick, uint32_t addr, uint8_t value);

        static void write8(void* context, int32_t tick, uint32_t addr, uint8_t value)
        {
            static_cast<MapperMBC1*>(context)->write8(tick, addr, value);
        }

        MEM_ACCESS              mMemoryControlRegs;
        uint32_t                mBankROM;
        uint32_t                mBankRAM;
        bool                    mRamBankMode;
    };

    class MapperMBC5 : public MapperBase
    {
    public:
        MapperMBC5();
        virtual bool create(const Rom& rom, emu::MemoryBus& memory) override;
        virtual void reset() override;
        virtual void serializeGameState(emu::ISerializer& serializer) override;

    protected:
        virtual bool updateMemoryMap() override;
    private:
        void write8(int32_t tick, uint32_t addr, uint8_t value);

        static void write8(void* context, int32_t tick, uint32_t addr, uint8_t value)
        {
            static_cast<MapperMBC5*>(context)->write8(tick, addr, value);
        }

        MEM_ACCESS              mMemoryControlRegs;
        bool                    mRamBankMode;
    };

    IMapper* createMapper(const Rom& rom, emu::MemoryBus& memory);
}
