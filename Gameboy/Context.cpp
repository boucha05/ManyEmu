#include <Core/Clock.h>
#include <Core/MemoryBus.h>
#include <Core/RegisterBank.h>
#include <Core/Serialization.h>
#include "Audio.h"
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

    static const uint8_t CPU_DEFAULT_A_GB = 0x01;
    static const uint8_t CPU_DEFAULT_A_CGB = 0x11;

    static const uint32_t DISPLAY_TICKS_PER_LINE = 456;
    static const uint32_t DISPLAY_VISIBLE_LINES = 144;
    static const uint32_t DISPLAY_VBLANK_LINES = 10;
    static const uint32_t DISPLAY_TOTAL_LINES = DISPLAY_VISIBLE_LINES + DISPLAY_VBLANK_LINES;
    static const uint32_t DISPLAY_TICKS_PER_FRAME = DISPLAY_TICKS_PER_LINE * DISPLAY_TOTAL_LINES;

    static const uint32_t MASTER_CLOCK_FREQUENCY_GB = 4194304;
    static const uint32_t MASTER_CLOCK_FREQUENCY_CGB = 2 * MASTER_CLOCK_FREQUENCY_GB;
    static const uint32_t MASTER_CLOCK_FREQUENCY_SGB = 4295454;

    static const uint32_t MASTER_CLOCK_CPU_DIVIDER = 1;
    static const uint32_t MASTER_TICKS_PER_FRAME = DISPLAY_TICKS_PER_FRAME;

    static const uint32_t WRAM_SIZE_GB = 0x2000;
    static const uint32_t WRAM_SIZE_GBC = 0x8000;
    static const uint32_t WRAM_BANK_SIZE = 0x1000;

    static const uint32_t HRAM_SIZE = 0x7f;

    static const uint8_t KEY1_SPEED_SWITCH = 0x01;
    static const uint8_t KEY1_CURRENT_SPEED = 0x80;

    static const uint8_t SVBK_BANK_MASK = 0x07;
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

        bool create(const gb::Rom& rom, gb::Model model)
        {
            mModel = model;

            bool isGBC = mModel >= gb::Model::GBC;
            gb::Display::Config displayConfig;
            displayConfig.model = mModel;

            uint32_t masterClockFrequency = MASTER_CLOCK_FREQUENCY_GB;
            uint32_t fixedClockDivider = 1;
            mVariableClockDivider = 1;
            if (isGBC)
            {
                masterClockFrequency = MASTER_CLOCK_FREQUENCY_CGB;
                fixedClockDivider = 2;
                mVariableClockDivider = 2;
            }
            mTicksPerFrame = DISPLAY_TICKS_PER_FRAME * fixedClockDivider;

            EMU_VERIFY(mClock.create());
            EMU_VERIFY(mMemory.create(MEM_SIZE_LOG2, MEM_PAGE_SIZE_LOG2));
            EMU_VERIFY(mCpu.create(mClock, mMemory.getState(), mVariableClockDivider, isGBC ? CPU_DEFAULT_A_CGB : CPU_DEFAULT_A_GB));

            mMapper = gb::createMapper(rom, mMemory);
            EMU_VERIFY(mMapper);

            mWRAM.resize(isGBC ? WRAM_SIZE_GBC : WRAM_SIZE_GB, 0);
            mHRAM.resize(HRAM_SIZE, 0);

            for (uint32_t bank = 0, offset = 0; bank < EMU_ARRAY_SIZE(mBankMapWRAM); ++bank, offset += WRAM_BANK_SIZE)
            {
                if (offset >= mWRAM.size())
                    offset = 0;
                mBankMapWRAM[bank] = mWRAM.data() + (offset ? offset : WRAM_BANK_SIZE);
            }

            mBankWRAM = 0;
            mRegKEY1 = 0x00;
            mRegSVBK = 0x01;

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
            EMU_VERIFY(mDisplay.create(displayConfig, mClock, fixedClockDivider, mMemory, mInterrupts, mRegistersIO));
            EMU_VERIFY(mJoypad.create(mClock, mInterrupts, mRegistersIO));
            EMU_VERIFY(mTimer.create(mClock, masterClockFrequency, fixedClockDivider, mVariableClockDivider, mInterrupts, mRegistersIO));
            EMU_VERIFY(mAudio.create(mClock, fixedClockDivider, mRegistersIO));

            if (mModel >= gb::Model::GBC)
            {
                EMU_VERIFY(mRegisterAccessors.read.KEY1.create(mRegistersIO, 0x4d, *this, &ContextImpl::readKEY1));
                EMU_VERIFY(mRegisterAccessors.read.SVBK.create(mRegistersIO, 0x70, *this, &ContextImpl::readSVBK));

                EMU_VERIFY(mRegisterAccessors.write.KEY1.create(mRegistersIO, 0x4d, *this, &ContextImpl::writeKEY1));
                EMU_VERIFY(mRegisterAccessors.write.SVBK.create(mRegistersIO, 0x70, *this, &ContextImpl::writeSVBK));
            }

            EMU_VERIFY(mStopListener.create(mCpu, *this));

            reset();

            return true;
        }

        void destroy()
        {
            mStopListener.destroy();
            mRegisterAccessors = RegisterAccessors();
            mAudio.destroy();
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
            mBankWRAM = 0;
            mRegSVBK = mBankWRAM;
            mRegKEY1 = 0x00;

            mVariableClockDivider = 1;
            if (mModel >= gb::Model::GBC)
                mVariableClockDivider = 2;
            setVariableClockDivider(mVariableClockDivider);

            mClock.reset();
            mCpu.reset();
            if (mMapper)
                mMapper->reset();
            mInterrupts.reset();
            mGameLink.reset();
            mDisplay.reset();
            mJoypad.reset();
            mTimer.reset();
            mAudio.reset();
        }

        void setVariableClockDivider(uint32_t variableClockDivider)
        {
            mVariableClockDivider = variableClockDivider;
            mCpu.setClockDivider(variableClockDivider);
            mTimer.setVariableClockDivider(variableClockDivider);
        }

        virtual void setController(uint32_t index, uint32_t buttons)
        {
            mJoypad.setController(index, buttons);
        }

        virtual void setSoundBuffer(int16_t* buffer, size_t size)
        {
            mAudio.setSoundBuffer(buffer, size);
        }

        virtual void setRenderSurface(void* surface, size_t pitch)
        {
            mDisplay.setRenderSurface(surface, pitch);
        }

        virtual void update()
        {
            mDisplay.beginFrame();
            mTimer.beginFrame();
            mInterrupts.beginFrame(mClock.getDesiredTicks());
            mClock.execute(mTicksPerFrame);
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
            serializer.serialize(mVariableClockDivider);
            serializer.serialize(mBankWRAM);
            serializer.serialize(mRegKEY1);
            serializer.serialize(mRegSVBK);
            mInterrupts.serialize(serializer);
            mGameLink.serialize(serializer);
            mDisplay.serialize(serializer);
            mJoypad.serialize(serializer);
            mTimer.serialize(serializer);
            mAudio.serialize(serializer);
            if (serializer.isReading())
            {
                updateMemoryMap();
                setVariableClockDivider(mVariableClockDivider);
            }
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
        class StopListener : public gb::CpuZ80::IStopListener
        {
        public:
            StopListener()
            {
                initialize();
            }

            ~StopListener()
            {
                destroy();
            }

            void initialize()
            {
                mCpu = nullptr;
                mContext = nullptr;
            }

            bool create(gb::CpuZ80& cpu, ContextImpl& context)
            {
                mCpu = &cpu;
                mContext = &context;
                mCpu->addStopListener(*this);
                return true;
            }

            void destroy()
            {
                if (mCpu)
                    mCpu->removeStopListener(*this);
            }

            virtual void onStop(int32_t tick) override
            {
                mContext->onStop(tick);
            }

        private:
            gb::CpuZ80*     mCpu;
            ContextImpl*    mContext;
        };

        struct RegisterAccessors
        {
            struct
            {
                emu::RegisterRead   KEY1;
                emu::RegisterRead   SVBK;
            }                       read;

            struct
            {
                emu::RegisterWrite  KEY1;
                emu::RegisterWrite  SVBK;
            }                       write;
        };

        bool updateMemoryMap()
        {
            mMemoryWRAM[0].setReadWriteMemory(mWRAM.data());
            mMemoryWRAM[1].setReadWriteMemory(mBankMapWRAM[mBankWRAM]);
            mMemoryWRAM[2].setReadWriteMemory(mWRAM.data());
            mMemoryWRAM[3].setReadWriteMemory(mBankMapWRAM[mBankWRAM]);
            mMemoryHRAM.setReadWriteMemory(mHRAM.data());
            return true;
        }

        uint8_t readKEY1(int32_t tick, uint16_t addr)
        {
            return mRegKEY1;
        }

        void writeKEY1(int32_t tick, uint16_t addr, uint8_t value)
        {
            mRegKEY1 = (mRegKEY1 & ~KEY1_SPEED_SWITCH) | (value & KEY1_SPEED_SWITCH);
        }

        uint8_t readSVBK(int32_t tick, uint16_t addr)
        {
            return mRegSVBK;
        }

        void writeSVBK(int32_t tick, uint16_t addr, uint8_t value)
        {
            mRegSVBK = value;
            mBankWRAM = value & SVBK_BANK_MASK;
            updateMemoryMap();
        }

        void onStop(int32_t tick)
        {
            if (mModel >= gb::Model::GBC)
            {
                if (mRegKEY1 & KEY1_SPEED_SWITCH)
                {
                    mRegKEY1 &= ~KEY1_SPEED_SWITCH;
                    mRegKEY1 ^= KEY1_CURRENT_SPEED;
                    setVariableClockDivider((mVariableClockDivider == 1) ? 2 : 1);
                    mCpu.resume(tick);
                }
                else
                {
                    EMU_NOT_IMPLEMENTED();
                }
            }
            else
            {
                EMU_NOT_IMPLEMENTED();
            }
            mCpu.resume(tick);
        }

        gb::Model               mModel;
        emu::Clock              mClock;
        emu::MemoryBus          mMemory;
        gb::CpuZ80              mCpu;
        gb::IMapper*            mMapper;
        RegisterAccessors       mRegisterAccessors;
        StopListener            mStopListener;
        MEM_ACCESS_READ_WRITE   mMemoryWRAM[4];
        MEM_ACCESS_READ_WRITE   mMemoryHRAM;
        std::vector<uint8_t>    mWRAM;
        std::vector<uint8_t>    mHRAM;
        uint32_t                mVariableClockDivider;
        uint32_t                mTicksPerFrame;
        uint8_t*                mBankMapWRAM[8];
        uint8_t                 mBankWRAM;
        uint8_t                 mRegKEY1;
        uint8_t                 mRegSVBK;
        emu::RegisterBank       mRegistersIO;
        emu::RegisterBank       mRegistersIE;
        gb::Interrupts          mInterrupts;
        gb::GameLink            mGameLink;
        gb::Display             mDisplay;
        gb::Joypad              mJoypad;
        gb::Timer               mTimer;
        gb::Audio               mAudio;
    };
}

namespace gb
{
    Context* Context::create(const Rom& rom, Model model)
    {
        gb_context::ContextImpl* context = new gb_context::ContextImpl;
        if (!context->create(rom, model))
        {
            context->dispose();
            context = nullptr;
        }
        return context;
    }
}