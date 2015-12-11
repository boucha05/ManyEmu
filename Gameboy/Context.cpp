#include <Core/Clock.h>
#include <Core/MemoryBus.h>
#include <Core/Serialization.h>
#include "CpuZ80.h"
#include "GB.h"
#include "Registers.h"
#include <vector>

namespace
{
    static const uint32_t MEM_SIZE_LOG2 = 16;
    static const uint32_t MEM_SIZE = 1 << MEM_SIZE_LOG2;
    static const uint32_t MEM_PAGE_SIZE_LOG2 = 10;

    static const uint32_t DISPLAY_CLOCK_PER_LINE = 456;
    static const uint32_t DISPLAY_VISIBLE_LINES = 144;
    static const uint32_t DISPLAY_VBLANK_LINES = 10;
    static const uint32_t DISPLAY_TOTAL_LINES = DISPLAY_VISIBLE_LINES + DISPLAY_VBLANK_LINES;
    static const uint32_t DISPLAY_CLOCK_PER_FRAME = DISPLAY_CLOCK_PER_LINE * DISPLAY_TOTAL_LINES;

    static const uint32_t MASTER_CLOCK_FREQUENCY_GB = 4194304;
    static const uint32_t MASTER_CLOCK_FREQUENCY_CGB = 2 * MASTER_CLOCK_FREQUENCY_GB;
    static const uint32_t MASTER_CLOCK_FREQUENCY_SGB = 4295454;

    static const uint32_t MASTER_CLOCK_CPU_DIVIDER = 1;
    static const uint32_t MASTER_CLOCK_PRE_FRAME = DISPLAY_CLOCK_PER_FRAME;

    static const uint32_t ROM_BANK_SIZE = 0x4000;

    static const uint32_t VRAM_SIZE_GB = 0x2000;
    static const uint32_t VRAM_SIZE_GBC = 0x4000;
    static const uint32_t VRAM_BANK_SIZE = 0x2000;

    static const uint32_t BANK_SIZE_EXTERNAL_RAM = 0x2000;

    static const uint32_t WRAM_SIZE_GB = 0x2000;
    static const uint32_t WRAM_SIZE_GBC = 0x8000;
    static const uint32_t WRAM_BANK_SIZE = 0x1000;

    static const uint32_t OAM_SIZE = 0xa0;

    static const uint32_t HRAM_SIZE = 0x7f;
}

namespace
{
    class ContextImpl : public gb::Context
    {
    public:
        ContextImpl()
            : mRom(nullptr)
        {
        }

        ~ContextImpl()
        {
            destroy();
        }

        bool create(const gb::Rom& rom)
        {
            mModel = gb::Model::GBC;

            bool isGBC = mModel >= gb::Model::GBC;

            mRom = &rom;

            EMU_VERIFY(mClock.create());
            EMU_VERIFY(mMemory.create(MEM_SIZE_LOG2, MEM_PAGE_SIZE_LOG2));
            mCpu.create(mClock, mMemory.getState(), MASTER_CLOCK_CPU_DIVIDER);

            mVRAM.resize(isGBC ? VRAM_SIZE_GBC : VRAM_SIZE_GB, 0);
            mWRAM.resize(isGBC ? WRAM_SIZE_GBC : WRAM_SIZE_GB, 0);
            mOAM.resize(OAM_SIZE, 0);
            mHRAM.resize(HRAM_SIZE, 0);

            mBankROM[0] = 0;
            mBankROM[1] = 1;
            mBankVRAM = 0;
            mBankExternalRAM = 0xff;
            mBankWRAM[0] = 0;
            mBankWRAM[1] = 1;
            EMU_VERIFY(updateMemoryMap());

            reset();

            return true;
        }

        void destroy()
        {
            mVRAM.clear();
            mExternalRAM.clear();
            mWRAM.clear();
            mOAM.clear();
            mHRAM.clear();
            mMemory.destroy();
            mRom = nullptr;
        }

        virtual void dispose()
        {
            delete this;
        }

        virtual void reset()
        {
            mClock.reset();
            mCpu.reset();
            mRegisters.reset(mModel >= gb::Model::SGB);
        }

        virtual void setController(uint32_t /*index*/, uint32_t /*buttons*/)
        {
        }

        virtual void setSoundBuffer(int16_t* /*buffer*/, size_t /*size*/)
        {
        }

        virtual void setRenderSurface(void* surface, size_t pitch)
        {
        }

        virtual void update()
        {
            mClock.setTargetExecution(DISPLAY_CLOCK_PER_FRAME);
            while (mClock.canExecute())
            {
                mClock.beginExecute();
                mCpu.execute();
                mClock.endExecute();
            }
            mClock.advance();
            mClock.clearEvents();
        }

        virtual uint8_t read8(uint16_t addr)
        {
            uint8_t value = memory_bus_read8(mMemory.getState(), mClock.getDesiredTicks(), addr);
            return value;
        }

        virtual void write8(uint16_t addr, uint8_t value)
        {
            memory_bus_write8(mMemory.getState(), mClock.getDesiredTicks(), addr, value);
        }

        virtual void serializeGameData(emu::ISerializer& /*serializer*/)
        {
        }

        virtual void serializeGameState(emu::ISerializer& serializer)
        {
            uint32_t version = 1;
            mClock.serialize(serializer);
            mCpu.serialize(serializer);
            serializer.serialize(mVRAM);
            serializer.serialize(mOAM);
            serializer.serialize(mHRAM);
            serializer.serialize(mBankROM, EMU_ARRAY_SIZE(mBankROM));
            serializer.serialize(mBankVRAM);
            serializer.serialize(mBankExternalRAM);
            serializer.serialize(mBankWRAM, EMU_ARRAY_SIZE(mBankWRAM));
            mRegisters.serialize(serializer);
        }

    private:
        bool updateMemoryMap()
        {
            const auto& romContent = mRom->getContent();
            EMU_VERIFY(mMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_READ, 0x0000, 0x3fff, mMemoryROM[0].setReadMemory(romContent.rom + mBankROM[0] * ROM_BANK_SIZE)));
            EMU_VERIFY(mMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_READ, 0x4000, 0x7fff, mMemoryROM[1].setReadMemory(romContent.rom + mBankROM[1] * ROM_BANK_SIZE)));
            EMU_VERIFY(mMemory.addMemoryRange(0x8000, 0x9fff, mMemoryVRAM.setReadWriteMemory(mVRAM.data() + mBankVRAM * VRAM_BANK_SIZE)));
            if (!mExternalRAM.empty())
            {
                EMU_VERIFY(mMemory.addMemoryRange(0xa000, 0xbfff, mMemoryExternalRAM.setReadWriteMemory(mExternalRAM.data())));
            }
            EMU_VERIFY(mMemory.addMemoryRange(0xc000, 0xcfff, mMemoryWRAM[0].setReadWriteMemory(mWRAM.data() + mBankWRAM[0] * WRAM_BANK_SIZE)));
            EMU_VERIFY(mMemory.addMemoryRange(0xd000, 0xdfff, mMemoryWRAM[1].setReadWriteMemory(mWRAM.data() + mBankWRAM[1] * WRAM_BANK_SIZE)));
            EMU_VERIFY(mMemory.addMemoryRange(0xe000, 0xefff, mMemoryWRAM[2].setReadWriteMemory(mWRAM.data() + mBankWRAM[0] * WRAM_BANK_SIZE)));
            EMU_VERIFY(mMemory.addMemoryRange(0xf000, 0xfdff, mMemoryWRAM[3].setReadWriteMemory(mWRAM.data() + mBankWRAM[1] * WRAM_BANK_SIZE)));
            EMU_VERIFY(mMemory.addMemoryRange(0xfe00, 0xfe9f, mMemoryOAM.setReadWriteMemory(mOAM.data())));
            EMU_VERIFY(mMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_READ, 0xff00, 0xff7f, mMemoryIO.read.setReadMethod(&ioRegsRead, this, 0xff00)));
            EMU_VERIFY(mMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_WRITE, 0xff00, 0xff7f, mMemoryIO.read.setWriteMethod(&ioRegsWrite, this, 0xff00)));
            EMU_VERIFY(mMemory.addMemoryRange(0xff80, 0xfffe, mMemoryHRAM.setReadWriteMemory(mHRAM.data())));
            EMU_VERIFY(mMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_READ, 0xffff, 0xffff, mMemoryIntEnable.read.setReadMethod(&intEnableRead, this, 0xffff)));
            EMU_VERIFY(mMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_WRITE, 0xffff, 0xffff, mMemoryIntEnable.read.setWriteMethod(&intEnableWrite, this, 0xffff)));
            return true;
        }

        static uint8_t ioRegsRead(void* context, int32_t ticks, uint32_t addr)
        {
            EMU_NOT_IMPLEMENTED();
            return 0x00;
        }

        static void ioRegsWrite(void* /*context*/, int32_t /*ticks*/, uint32_t /*addr*/, uint8_t /*value*/)
        {
            EMU_NOT_IMPLEMENTED();
        }

        static uint8_t intEnableRead(void* context, int32_t ticks, uint32_t addr)
        {
            EMU_NOT_IMPLEMENTED();
            return 0x00;
        }

        static void intEnableWrite(void* /*context*/, int32_t /*ticks*/, uint32_t /*addr*/, uint8_t /*value*/)
        {
            EMU_NOT_IMPLEMENTED();
        }

        gb::Model               mModel;
        const gb::Rom*          mRom;
        emu::Clock              mClock;
        emu::MemoryBus          mMemory;
        gb::CpuZ80              mCpu;
        MEM_ACCESS              mMemoryROM[2];
        MEM_ACCESS_READ_WRITE   mMemoryVRAM;
        MEM_ACCESS_READ_WRITE   mMemoryExternalRAM;
        MEM_ACCESS_READ_WRITE   mMemoryWRAM[4];
        MEM_ACCESS_READ_WRITE   mMemoryOAM;
        MEM_ACCESS_READ_WRITE   mMemoryIO;
        MEM_ACCESS_READ_WRITE   mMemoryHRAM;
        MEM_ACCESS_READ_WRITE   mMemoryIntEnable;
        std::vector<uint8_t>    mVRAM;
        std::vector<uint8_t>    mExternalRAM;
        std::vector<uint8_t>    mWRAM;
        std::vector<uint8_t>    mOAM;
        std::vector<uint8_t>    mHRAM;
        uint8_t                 mBankROM[2];
        uint8_t                 mBankVRAM;
        uint8_t                 mBankExternalRAM;
        uint8_t                 mBankWRAM[2];
        gb::Registers           mRegisters;
    };
}

namespace gb
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