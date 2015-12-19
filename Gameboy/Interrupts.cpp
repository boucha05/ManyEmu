#include <Core/Serialization.h>
#include "Interrupts.h"
#include "CpuZ80.h"

namespace
{
    static const uint8_t INT_MASK_NONE = 0x00;
    static const uint8_t INT_MASK_ALL = 0x1f;
    static const uint8_t INT_BIT_COUNT = 5;

    static const uint8_t intBitMask[INT_BIT_COUNT] = { 0x01, 0x02, 0x04, 0x08, 0x10 };
    static const uint16_t intVector[INT_BIT_COUNT] = { 0x40, 0x48, 0x50, 0x58, 0x60 };
}

namespace gb
{
    class Interrupts::CpuInterruptListener : public CpuZ80::IInterruptListener
    {
    public:
        CpuInterruptListener(CpuZ80& cpu, Interrupts& interrupts)
            : mCpu(cpu)
            , mInterrupts(interrupts)
        {
            create();
        }

        ~CpuInterruptListener()
        {
            destroy();
        }

        void create()
        {
            mCpu.addInterruptListener(*this);
        }

        void destroy()
        {
            mCpu.removeInterruptListener(*this);
        }

        virtual void onInterruptEnable(int32_t tick) override
        {
            mInterrupts.onCpuInterruptEnable(tick);
        }

        virtual void onInterruptDisable(int32_t tick) override
        {
            mInterrupts.onCpuInterruptDisable(tick);
        }

    private:
        CpuZ80&         mCpu;
        Interrupts&     mInterrupts;
    };

    ////////////////////////////////////////////////////////////////////////////

    Interrupts::Interrupts()
        : mCpu(nullptr)
        , mInterruptListener(nullptr)
        , mMask(INT_MASK_NONE)
        , mRegIE(INT_MASK_NONE)
        , mRegIF(INT_MASK_NONE)
    {
    }

    Interrupts::~Interrupts()
    {
    }

    bool Interrupts::create(CpuZ80& cpu, emu::RegisterBank& registersIO, emu::RegisterBank& registersIE)
    {
        mCpu = &cpu;

        EMU_VERIFY(mRegisterAccessors.read.IF.create(registersIO, 0x0f, *this, &Interrupts::readIF));
        EMU_VERIFY(mRegisterAccessors.write.IF.create(registersIO, 0x0f, *this, &Interrupts::writeIF));
        EMU_VERIFY(mRegisterAccessors.read.IE.create(registersIE, 0x00, *this, &Interrupts::readIE));
        EMU_VERIFY(mRegisterAccessors.write.IE.create(registersIE, 0x00, *this, &Interrupts::writeIE));

        mInterruptListener = new CpuInterruptListener(*mCpu, *this);

        return true;
    }

    void Interrupts::destroy()
    {
        if (mInterruptListener)
        {
            delete mInterruptListener;
            mInterruptListener = nullptr;
        }

        mRegisterAccessors = RegisterAccessors();

        mCpu = nullptr;
    }

    void Interrupts::reset()
    {
    }

    void Interrupts::serialize(emu::ISerializer& serializer)
    {
        serializer.serialize(mMask);
        serializer.serialize(mRegIF);
        serializer.serialize(mRegIE);
    }

    void Interrupts::setInterrupt(int tick, Signal signal)
    {
        mRegIF |= 1 << static_cast<uint8_t>(signal);
        checkInterrupts(tick);
    }

    void Interrupts::clearInterrupt(int tick, Signal signal)
    {
        mRegIF &= ~(1 << static_cast<uint8_t>(signal));
    }

    uint8_t Interrupts::readIF(int32_t tick, uint16_t addr)
    {
        return mRegIF;
    }

    void Interrupts::writeIF(int32_t tick, uint16_t addr, uint8_t value)
    {
        mRegIF = value;
        checkInterrupts(tick);
    }

    uint8_t Interrupts::readIE(int32_t tick, uint16_t addr)
    {
        return mRegIE;
    }

    void Interrupts::writeIE(int32_t tick, uint16_t addr, uint8_t value)
    {
        mRegIE = value;
        checkInterrupts(tick);
    }

    void Interrupts::onCpuInterruptEnable(int32_t tick)
    {
        mMask = INT_MASK_ALL;
        checkInterrupts(tick);
    }

    void Interrupts::onCpuInterruptDisable(int32_t tick)
    {
        mMask = INT_MASK_NONE;
    }

    void Interrupts::checkInterrupts(int32_t tick)
    {
        uint8_t signaled = mMask & mRegIE & mRegIF;
        if (signaled)
        {
            for (uint8_t bit = 0; bit < INT_BIT_COUNT; ++bit)
            {
                int bitMask = intBitMask[bit];
                if (signaled & bitMask)
                {
                    mRegIF &= ~bitMask;
                    mCpu->interrupt(tick, intVector[bit]);
                    return;
                }
            }
        }
    }
}
