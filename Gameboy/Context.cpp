#include <Core/Clock.h>
#include <Core/MemoryBus.h>
#include <Core/RegisterBank.h>
#include <Core/Serializer.h>
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

        virtual bool getInfo(Info& info) override
        {
            info = Info();
            info.cpuCount = 0;
            return true;
        }

        virtual emu::ICPU* getCpu(uint32_t index) override
        {
            return index == 0 ? &mCpu : nullptr;
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
            mTicksPerFrame = MASTER_TICKS_PER_FRAME * fixedClockDivider;

            EMU_VERIFY(mClock.create());
            EMU_VERIFY(mMemory.create(MEM_SIZE_LOG2, MEM_PAGE_SIZE_LOG2));
            EMU_VERIFY(mCpu.create(mClock, mMemory, mVariableClockDivider, isGBC ? CPU_DEFAULT_A_CGB : CPU_DEFAULT_A_GB));

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
            EMU_VERIFY(mAudio.create(mClock, masterClockFrequency, fixedClockDivider, mRegistersIO));

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

        virtual bool getDisplaySize(uint32_t& sizeX, uint32_t& sizeY) override
        {
            sizeX = gb::Context::DisplaySizeX;
            sizeY = gb::Context::DisplaySizeY;
            return true;
        }

        virtual bool reset() override
        {
            mBankWRAM = 0;
            mRegSVBK = mBankWRAM;
            mRegKEY1 = 0x00;

            mVariableClockDivider = 1;
            if (mModel >= gb::Model::GBC)
                mVariableClockDivider = 2;
            setVariableClockDivider(mVariableClockDivider);

            mMemoryReadAccessor.reset();
            mMemoryWriteAccessor.reset();
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
            return true;
        }

        void setVariableClockDivider(uint32_t variableClockDivider)
        {
            mVariableClockDivider = variableClockDivider;
            mCpu.setClockDivider(variableClockDivider);
            mTimer.setVariableClockDivider(variableClockDivider);
        }

        virtual bool setController(uint32_t index, uint32_t buttons) override
        {
            mJoypad.setController(index, buttons);
            return true;
        }

        virtual bool setSoundBuffer(void* buffer, size_t size) override
        {
            mAudio.setSoundBuffer(static_cast<int16_t*>(buffer), size);
            return true;
        }

        virtual bool setRenderBuffer(void* surface, size_t pitch) override
        {
            mDisplay.setRenderSurface(surface, pitch);
            return true;
        }

        virtual bool execute() override
        {
            mDisplay.beginFrame();
            mTimer.beginFrame();
            mInterrupts.beginFrame(mClock.getDesiredTicks());
            mClock.execute(mTicksPerFrame);
            mClock.advance();
            mClock.clearEvents();
            return true;
        }

        virtual uint8_t read8(uint16_t addr)
        {
            uint8_t value = mMemory.read8(mMemoryReadAccessor, mClock.getDesiredTicks(), addr);
            return value;
        }

        virtual void write8(uint16_t addr, uint8_t value)
        {
            mMemory.write8(mMemoryWriteAccessor, mClock.getDesiredTicks(), addr, value);
        }

        virtual bool serializeGameData(emu::ISerializer& serializer) override
        {
            if (mMapper)
                mMapper->serializeGameData(serializer);
            return true;
        }

        virtual bool serializeGameState(emu::ISerializer& serializer) override
        {
            uint32_t version = 1;
            EMU_UNUSED(version);
            mClock.serialize(serializer);
            mCpu.serialize(serializer);
            if (mMapper)
                mMapper->serializeGameState(serializer);
            serializer
                .value("WRAM", mWRAM)
                .value("HRAM", mHRAM)
                .value("VariableClockDivider", mVariableClockDivider)
                .value("BankWRAM", mBankWRAM)
                .value("RegKEY1", mRegKEY1)
                .value("RegSVBK", mRegSVBK)
                .value("Interrupts", mInterrupts)
                .value("GameLink", mGameLink)
                .value("Display", mDisplay)
                .value("Joypad", mJoypad)
                .value("Timer", mTimer)
                .value("Audio", mAudio);
            if (serializer.isReading())
            {
                updateMemoryMap();
                setVariableClockDivider(mVariableClockDivider);
            }
            return true;
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
            EMU_UNUSED(tick);
            EMU_UNUSED(addr);
            return mRegKEY1;
        }

        void writeKEY1(int32_t tick, uint16_t addr, uint8_t value)
        {
            EMU_UNUSED(tick);
            EMU_UNUSED(addr);
            mRegKEY1 = (mRegKEY1 & ~KEY1_SPEED_SWITCH) | (value & KEY1_SPEED_SWITCH);
        }

        uint8_t readSVBK(int32_t tick, uint16_t addr)
        {
            EMU_UNUSED(tick);
            EMU_UNUSED(addr);
            return mRegSVBK;
        }

        void writeSVBK(int32_t tick, uint16_t addr, uint8_t value)
        {
            EMU_UNUSED(tick);
            EMU_UNUSED(addr);
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

        gb::Model                   mModel;
        emu::Clock                  mClock;
        emu::MemoryBus              mMemory;
        emu::MemoryBus::Accessor    mMemoryReadAccessor;
        emu::MemoryBus::Accessor    mMemoryWriteAccessor;
        gb::CpuZ80                  mCpu;
        gb::IMapper*                mMapper;
        RegisterAccessors           mRegisterAccessors;
        StopListener                mStopListener;
        MEM_ACCESS_READ_WRITE       mMemoryWRAM[4];
        MEM_ACCESS_READ_WRITE       mMemoryHRAM;
        emu::Buffer                 mWRAM;
        emu::Buffer                 mHRAM;
        uint32_t                    mVariableClockDivider;
        uint32_t                    mTicksPerFrame;
        uint8_t*                    mBankMapWRAM[8];
        uint8_t                     mBankWRAM;
        uint8_t                     mRegKEY1;
        uint8_t                     mRegSVBK;
        emu::RegisterBank           mRegistersIO;
        emu::RegisterBank           mRegistersIE;
        gb::Interrupts              mInterrupts;
        gb::GameLink                mGameLink;
        gb::Display                 mDisplay;
        gb::Joypad                  mJoypad;
        gb::Timer                   mTimer;
        gb::Audio                   mAudio;
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