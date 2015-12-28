#include <Core/Clock.h>
#include <Core/MemoryBus.h>
#include <Core/RegisterBank.h>
#include <Core/Serialization.h>
#include "CpuZ80.h"
#include "Display.h"
#include "Joypad.h"
#include "GameLink.h"
#include "GB.h"
#include "Interrupts.h"
#include "Mappers.h"
#include "Registers.h"
#include "Timer.h"
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

    static const uint32_t WRAM_SIZE_GB = 0x2000;
    static const uint32_t WRAM_SIZE_GBC = 0x8000;
    static const uint32_t WRAM_BANK_SIZE = 0x1000;

    static const uint32_t HRAM_SIZE = 0x7f;
}

namespace gb_context
{
    class ContextImpl : public gb::Context
    {
    public:
        ContextImpl()
        {
            initialize();
        }

        ~ContextImpl()
        {
            destroy();
        }

        void initialize()
        {
            mMapper = nullptr;
        }

        bool create(const gb::Rom& rom)
        {
            mModel = gb::Model::GB;

            bool isGBC = mModel >= gb::Model::GBC;
            gb::Display::Config displayConfig;
            displayConfig.model = mModel;

            uint32_t masterClockDivider = MASTER_CLOCK_CPU_DIVIDER;
            uint32_t masterClockFrequency = MASTER_CLOCK_FREQUENCY_GB * masterClockDivider;

            EMU_VERIFY(mClock.create());
            EMU_VERIFY(mMemory.create(MEM_SIZE_LOG2, MEM_PAGE_SIZE_LOG2));
            EMU_VERIFY(mCpu.create(mClock, mMemory.getState(), masterClockDivider));

            mMapper = gb::createMapper(rom, mMemory);
            EMU_VERIFY(mMapper);

            mWRAM.resize(isGBC ? WRAM_SIZE_GBC : WRAM_SIZE_GB, 0);
            mHRAM.resize(HRAM_SIZE, 0);

            mBankWRAM[0] = 0;
            mBankWRAM[1] = 1;

            EMU_VERIFY(mRegistersIO.create(mMemory, 0xff00, 0x80, emu::RegisterBank::Access::ReadWrite));
            EMU_VERIFY(mRegistersIE.create(mMemory, 0xffff, 0x01, emu::RegisterBank::Access::ReadWrite));

            EMU_VERIFY(updateMemoryMap());
            EMU_VERIFY(mMemory.addMemoryRange(0xc000, 0xcfff, mMemoryWRAM[0]));
            EMU_VERIFY(mMemory.addMemoryRange(0xd000, 0xdfff, mMemoryWRAM[1]));
            EMU_VERIFY(mMemory.addMemoryRange(0xe000, 0xefff, mMemoryWRAM[2]));
            EMU_VERIFY(mMemory.addMemoryRange(0xf000, 0xfdff, mMemoryWRAM[3]));
            EMU_VERIFY(mMemory.addMemoryRange(0xff80, 0xfffe, mMemoryHRAM));

            EMU_VERIFY(mInterrupts.create(mCpu, mRegistersIO, mRegistersIE));
            EMU_VERIFY(mGameLink.create(mRegistersIO));
            EMU_VERIFY(mDisplay.create(displayConfig, mClock, masterClockDivider, mMemory, mInterrupts, mRegistersIO));
            EMU_VERIFY(mJoypad.create(mClock, mInterrupts, mRegistersIO));
            EMU_VERIFY(mTimer.create(mClock, masterClockFrequency, mInterrupts, mRegistersIO));

            reset();

            return true;
        }

        void destroy()
        {
            mTimer.destroy();
            mJoypad.destroy();
            mDisplay.destroy();
            mGameLink.destroy();
            mInterrupts.destroy();
            mRegistersIE.destroy();
            mRegistersIO.destroy();
            mWRAM.clear();
            mHRAM.clear();
            if (mMapper)
                delete mMapper;
            mMemory.destroy();
            initialize();
        }

        virtual void dispose()
        {
            delete this;
        }

        virtual void reset()
        {
            mClock.reset();
            mCpu.reset();
            if (mMapper)
                mMapper->reset();
            mInterrupts.reset();
            mGameLink.reset();
            mDisplay.reset();
            mJoypad.reset();
            mTimer.reset();
        }

        virtual void setController(uint32_t index, uint32_t buttons)
        {
            mJoypad.setController(index, buttons);
        }

        virtual void setSoundBuffer(int16_t* /*buffer*/, size_t /*size*/)
        {
        }

        virtual void setRenderSurface(void* surface, size_t pitch)
        {
            mDisplay.setRenderSurface(surface, pitch);
        }

        virtual void update()
        {
            mDisplay.beginFrame();
            mClock.execute(DISPLAY_CLOCK_PER_FRAME);
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

        virtual void serializeGameData(emu::ISerializer& serializer)
        {
            if (mMapper)
                mMapper->serializeGameData(serializer);
        }

        virtual void serializeGameState(emu::ISerializer& serializer)
        {
            uint32_t version = 1;
            mClock.serialize(serializer);
            mCpu.serialize(serializer);
            if (mMapper)
                mMapper->serializeGameState(serializer);
            serializer.serialize(mWRAM);
            serializer.serialize(mHRAM);
            serializer.serialize(mBankWRAM, EMU_ARRAY_SIZE(mBankWRAM));
            mInterrupts.serialize(serializer);
            mGameLink.serialize(serializer);
            mDisplay.serialize(serializer);
            mJoypad.serialize(serializer);
            mTimer.serialize(serializer);
        }

        emu::RegisterBank& getRegistersIO()
        {
            return mRegistersIO;
        }

        emu::RegisterBank& getRegistersIE()
        {
            return mRegistersIE;
        }

    private:
        bool updateMemoryMap()
        {
            mMemoryWRAM[0].setReadWriteMemory(mWRAM.data() + mBankWRAM[0] * WRAM_BANK_SIZE);
            mMemoryWRAM[1].setReadWriteMemory(mWRAM.data() + mBankWRAM[1] * WRAM_BANK_SIZE);
            mMemoryWRAM[2].setReadWriteMemory(mWRAM.data() + mBankWRAM[0] * WRAM_BANK_SIZE);
            mMemoryWRAM[3].setReadWriteMemory(mWRAM.data() + mBankWRAM[1] * WRAM_BANK_SIZE);
            mMemoryHRAM.setReadWriteMemory(mHRAM.data());
            return true;
        }

        gb::Model               mModel;
        emu::Clock              mClock;
        emu::MemoryBus          mMemory;
        gb::CpuZ80              mCpu;
        gb::IMapper*            mMapper;
        MEM_ACCESS_READ_WRITE   mMemoryWRAM[4];
        MEM_ACCESS_READ_WRITE   mMemoryHRAM;
        std::vector<uint8_t>    mWRAM;
        std::vector<uint8_t>    mHRAM;
        uint8_t                 mBankWRAM[2];
        emu::RegisterBank       mRegistersIO;
        emu::RegisterBank       mRegistersIE;
        gb::Interrupts          mInterrupts;
        gb::GameLink            mGameLink;
        gb::Display             mDisplay;
        gb::Joypad              mJoypad;
        gb::Timer               mTimer;
    };
}

namespace gb
{
    Context* Context::create(const Rom& rom)
    {
        gb_context::ContextImpl* context = new gb_context::ContextImpl;
        if (!context->create(rom))
        {
            context->dispose();
            context = nullptr;
        }
        return context;
    }
}