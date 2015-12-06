#include <Core/MemoryBus.h>
#include <Core/Serialization.h>
#include "CpuZ80.h"
#include "GB.h"
#include <memory.h>
#include <stdio.h>

namespace
{
    static const uint8_t FLAG_Z = 0x80;
    static const uint8_t FLAG_N = 0x40;
    static const uint8_t FLAG_H = 0x20;
    static const uint8_t FLAG_C = 0x10;
}

namespace gb
{
    uint8_t CpuZ80::read8(uint16_t addr)
    {
        return memory_bus_read8(*mMemory, mExecutedTicks, addr);
    }

    void CpuZ80::write8(uint16_t addr, uint8_t value)
    {
        memory_bus_write8(*mMemory, mExecutedTicks, addr, value);
    }

    uint16_t CpuZ80::read16(uint16_t addr)
    {
        uint8_t lo = read8(addr);
        uint8_t hi = read8(addr + 1);
        uint16_t value = static_cast<uint16_t>(lo) | (static_cast<uint16_t>(hi) << 8);
        return value;
    }

    uint8_t CpuZ80::fetch8()
    {
        return read8(mRegs.pc++);
    }

    uint16_t CpuZ80::fetch16()
    {
        uint8_t lo = fetch8();
        uint8_t hi = fetch8();
        uint16_t addr = static_cast<uint16_t>(lo) | (static_cast<uint16_t>(hi) << 8);
        return addr;
    }

    ///////////////////////////////////////////////////////////////////////////

    uint8_t CpuZ80::peek8(uint16_t& addr)
    {
        return read8(addr++);
    }

    uint16_t CpuZ80::peek16(uint16_t& addr)
    {
        uint8_t lo = peek8(addr);
        uint8_t hi = peek8(addr);
        uint16_t value = static_cast<uint16_t>(lo) | (static_cast<uint16_t>(hi) << 8);
        return value;
    }

    ///////////////////////////////////////////////////////////////////////////

    void CpuZ80::push8(uint8_t value)
    {
        uint16_t addr = mRegs.sp-- + 0x100;
        write8(addr, value);
    }
    
    uint8_t CpuZ80::pop8()
    {
        uint16_t addr = ++mRegs.sp + 0x100;
        uint8_t value = read8(addr);
        return value;
    }

    void CpuZ80::push16(uint16_t value)
    {
        uint8_t lo = value & 0xff;
        uint8_t hi = (value >> 8) & 0xff;
        push8(hi);
        push8(lo);
    }

    uint16_t CpuZ80::pop16()
    {
        uint8_t lo = pop8();
        uint8_t hi = pop8();
        uint16_t value = (static_cast<uint16_t>(lo) | (static_cast<uint16_t>(hi) << 8));
        return value;
    }

    ///////////////////////////////////////////////////////////////////////////

    CpuZ80::CpuZ80()
        : mClock(nullptr)
    {
    }

    CpuZ80::~CpuZ80()
    {
        destroy();
    }

    bool CpuZ80::create(emu::Clock& clock, MEMORY_BUS& bus, uint32_t master_clock_divider)
    {
        mClock = &clock;
        mClock->addListener(*this);
        return true;
    }

    void CpuZ80::destroy()
    {
        if (mClock)
        {
            mClock->removeListener(*this);
            mClock = nullptr;
        }
    }

    void CpuZ80::reset()
    {
        mRegs.a = 0x01;
        mRegs.b = 0x00;
        mRegs.c = 0x13;
        mRegs.d = 0x00;
        mRegs.e = 0xd8;
        mRegs.h = 0x01;
        mRegs.l = 0x4d;
        mRegs.flags = 0xb0;
        mRegs.sp = 0xfffe;
        mRegs.pc = 0x0100;
        mExecutedTicks = 0;
        mDesiredTicks = 0;
    }

    void CpuZ80::advanceClock(int32_t ticks)
    {
        mExecutedTicks -= ticks;
        mDesiredTicks = 0;
    }

    void CpuZ80::setDesiredTicks(int32_t ticks)
    {
        mDesiredTicks = ticks;
    }

    void CpuZ80::execute()
    {
        while (mExecutedTicks < mDesiredTicks)
        {
            uint8_t insn = fetch8();
            switch (insn)
            {
            case 0:
            default:
                EMU_ASSERT(false);
            }
        }
    }

    uint16_t CpuZ80::disassemble(char* /*buffer*/, size_t /*size*/, uint16_t addr)
    {
        return addr;
    }

    void CpuZ80::serialize(emu::ISerializer& serializer)
    {
        uint32_t version = 1;
        serializer.serialize(version);
        serializer.serialize(mRegs.a);
        serializer.serialize(mRegs.b);
        serializer.serialize(mRegs.c);
        serializer.serialize(mRegs.d);
        serializer.serialize(mRegs.e);
        serializer.serialize(mRegs.h);
        serializer.serialize(mRegs.l);
        serializer.serialize(mRegs.flags);
        serializer.serialize(mRegs.sp);
        serializer.serialize(mRegs.pc);
        serializer.serialize(mRegs.flag_z);
        serializer.serialize(mRegs.flag_n);
        serializer.serialize(mRegs.flag_h);
        serializer.serialize(mRegs.flag_c);
    }
}
