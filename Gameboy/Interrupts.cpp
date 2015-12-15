#include <Core/Serialization.h>
#include "Interrupts.h"
#include "CpuZ80.h"

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
            mInterrupts.onCpuInterruptDisable(tick);
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
    {
    }

    Interrupts::~Interrupts()
    {
    }

    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;

    bool Interrupts::create(CpuZ80& cpu, emu::RegisterBank& registersIO, emu::RegisterBank& registersIE)
    {
        mCpu = &cpu;
        EMU_VERIFY(registersIO.addRegister(0x0f, *this, &Interrupts::readIF, &Interrupts::writeIF, "IF", "Interrupt Flag"));
        EMU_VERIFY(registersIE.addRegister(0x00, *this, &Interrupts::readIE, &Interrupts::writeIE, "IE", "Interrupt Enable"));

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
        mCpu = nullptr;
    }

    void Interrupts::reset()
    {
    }

    void Interrupts::serialize(emu::ISerializer& serializer)
    {
        serializer.serialize(mRegIF);
        serializer.serialize(mRegIE);
    }

    void Interrupts::setInterrupt(Signal signal)
    {
    }

    void Interrupts::clearInterrupt(Signal signal)
    {
    }

    uint8_t Interrupts::readIF(int32_t ticks, uint16_t addr)
    {
        return mRegIF;
    }

    void Interrupts::writeIF(int32_t ticks, uint16_t addr, uint8_t value)
    {
        mRegIF = value;
    }

    uint8_t Interrupts::readIE(int32_t ticks, uint16_t addr)
    {
        return mRegIE;
    }

    void Interrupts::writeIE(int32_t ticks, uint16_t addr, uint8_t value)
    {
        mRegIE = value;
    }

    void Interrupts::onCpuInterruptEnable(int32_t tick)
    {
    }

    void Interrupts::onCpuInterruptDisable(int32_t tick)
    {
    }
}
