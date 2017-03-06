#include "Core.h"
#include "RegisterBank.h"

#define REPORT_INVALID_REGISTERS    1

namespace
{
    uint8_t nullRead8(void* context, int32_t ticks, uint16_t addr)
    {
        EMU_UNUSED(context);
        EMU_UNUSED(ticks);
        EMU_UNUSED(addr);
#if !REPORT_INVALID_REGISTERS
        EMU_NOT_IMPLEMENTED();
#endif
        return 0x00;
    }

    void nullWrite8(void* context, int32_t ticks, uint16_t addr, uint8_t value)
    {
        EMU_UNUSED(context);
        EMU_UNUSED(ticks);
        EMU_UNUSED(addr);
        EMU_UNUSED(value);
#if !REPORT_INVALID_REGISTERS
        EMU_NOT_IMPLEMENTED();
#endif
    }

    uint8_t regsRead8(void* context, int32_t ticks, uint32_t addr)
    {
        return static_cast<emu::RegisterBank*>(context)->read8(ticks, static_cast<uint16_t>(addr));
    }

    void regsWrite8(void* context, int32_t ticks, uint32_t addr, uint8_t value)
    {
        static_cast<emu::RegisterBank*>(context)->write8(ticks, static_cast<uint16_t>(addr), value);
    }
}

namespace emu
{
    RegisterBank::Reader::Reader()
        : mContext(nullptr)
        , mFunc(nullRead8)
        , mCount(0)
    {
    }

    RegisterBank::Reader::Reader(void* context, Read8Func func)
        : mContext(context)
        , mFunc(func)
        , mCount(0)
    {
    }

    ////////////////////////////////////////////////////////////////////////////

    RegisterBank::Writer::Writer()
        : mContext(nullptr)
        , mFunc(nullWrite8)
        , mCount(0)
    {
    }

    RegisterBank::Writer::Writer(void* context, Write8Func func)
        : mContext(context)
        , mFunc(func)
        , mCount(0)
    {
    }

    ////////////////////////////////////////////////////////////////////////////

    RegisterBank::RegisterBank()
        : mMemory(nullptr)
        , mBase(0)
    {
    }

    RegisterBank::~RegisterBank()
    {
    }

    bool RegisterBank::create(emu::MemoryBus& memory, uint16_t base, uint16_t size, Access access, bool absolute)
    {
        mMemory = &memory;
        mBase = absolute ? base : 0;
        mRegisters.resize(size);

        bool readAccess = (access == Access::Read) || (access == Access::ReadWrite);
        bool writeAccess = (access == Access::Write) || (access == Access::ReadWrite);
        EMU_VERIFY(readAccess || writeAccess);
        if (readAccess)
        {
            mMapping.read.setReadMethod(regsRead8, this, mBase);
            mMemory->addMemoryRange(MEMORY_BUS::PAGE_TABLE_READ, base, base + size - 1, mMapping.read);
        }
        if (writeAccess)
        {
            mMapping.write.setWriteMethod(regsWrite8, this, mBase);
            mMemory->addMemoryRange(MEMORY_BUS::PAGE_TABLE_WRITE, base, base + size - 1, mMapping.write);
        }
        return true;
    }

    void RegisterBank::destroy()
    {
        mRegisters.clear();
        mBase = 0;
        mMemory = nullptr;
    }

    void RegisterBank::clear()
    {
        auto size = mRegisters.size();
        mRegisters.clear();
        mRegisters.resize(size);
    }

    bool RegisterBank::addReader(uint16_t addr, void* context, Read8Func func)
    {
        uint16_t offset = addr - mBase;
        EMU_VERIFY(mMemory && (offset < mRegisters.size()));
        mRegisters[offset].reader = Reader(context, func);
        return true;
    }

    bool RegisterBank::addWriter(uint16_t addr, void* context, Write8Func func)
    {
        uint16_t offset = addr - mBase;
        EMU_VERIFY(mMemory && (offset < mRegisters.size()));
        mRegisters[offset].writer = Writer(context, func);
        return true;
    }

    void RegisterBank::removeReader(uint16_t addr)
    {
        uint16_t offset = addr - mBase;
        if (offset < mRegisters.size())
            mRegisters[offset].reader = Reader();
    }

    void RegisterBank::removeWriter(uint16_t addr)
    {
        uint16_t offset = addr - mBase;
        if (offset < mRegisters.size())
            mRegisters[offset].writer = Writer();
    }

    uint8_t RegisterBank::read8(int32_t ticks, uint16_t addr)
    {
        uint16_t offset = addr - mBase;
        EMU_ASSERT(offset < mRegisters.size());
        auto& reg = mRegisters[offset].reader;
#if REPORT_INVALID_REGISTERS
        if (reg.mFunc == &nullRead8)
        {
            if (!reg.mCount)
                printf("Register read not implemented: $%04X\n", mBase + addr);
        }
        ++reg.mCount;
#endif
        return reg.mFunc(reg.mContext, ticks, addr);
    }

    void RegisterBank::write8(int32_t ticks, uint16_t addr, uint8_t value)
    {
        uint16_t offset = addr - mBase;
        EMU_ASSERT(offset < mRegisters.size());
        auto& reg = mRegisters[offset].writer;
#if REPORT_INVALID_REGISTERS
        if (reg.mFunc == &nullWrite8)
        {
            if (!reg.mCount)
                printf("Register write not implemented: $%04X, $%02X\n", mBase + addr, value);
        }
        ++reg.mCount;
#endif
        return reg.mFunc(reg.mContext, ticks, addr, value);
    }

    ////////////////////////////////////////////////////////////////////////////

    RegisterRead::RegisterRead()
    {
        initialize();
    }

    RegisterRead::~RegisterRead()
    {
        destroy();
    }

    void RegisterRead::initialize()
    {
        mBank = nullptr;
        mAddr = 0;
        mRead8Method = [](int32_t ticks, uint16_t addr)
        {
            EMU_UNUSED(ticks);
            EMU_UNUSED(addr);
            return 0;
        };
    }

    bool RegisterRead::create(emu::RegisterBank& bank, uint16_t addr, Read8Method read8)
    {
        mBank = &bank;
        mAddr = addr;
        mRead8Method = read8;
        return mBank->addReader(mAddr, this, onRead8);
    }

    bool RegisterRead::create(emu::RegisterBank& bank, uint16_t addr, void* context, emu::RegisterBank::Read8Func func)
    {
        return create(bank, addr, [=](int32_t tick, uint16_t addr)
        {
            return func(context, tick, addr);
        });
    }

    void RegisterRead::destroy()
    {
        if (mBank)
        {
            mBank->removeReader(mAddr);
        }
        initialize();
    }

    RegisterRead& RegisterRead::operator = (const RegisterRead& other)
    {
        destroy();
        mBank = other.mBank;
        mAddr = other.mAddr;
        mRead8Method = other.mRead8Method;
        return *this;
    }

    uint8_t RegisterRead::onRead8(void* context, int32_t ticks, uint16_t addr)
    {
        auto& object = *static_cast<RegisterRead*>(context);
        return object.mRead8Method(ticks, addr);
    }

    ////////////////////////////////////////////////////////////////////////////

    RegisterWrite::RegisterWrite()
    {
        initialize();
    }

    RegisterWrite::~RegisterWrite()
    {
        destroy();
    }

    void RegisterWrite::initialize()
    {
        mBank = nullptr;
        mAddr = 0;
        mWrite8Method = [](int32_t ticks, uint16_t addr, uint8_t value)
        {
            EMU_UNUSED(ticks);
            EMU_UNUSED(addr);
            EMU_UNUSED(value);
        };
    }

    bool RegisterWrite::create(emu::RegisterBank& bank, uint16_t addr, Write8Method write8)
    {
        mBank = &bank;
        mAddr = addr;
        mWrite8Method = write8;
        return mBank->addWriter(mAddr, this, onWrite8);
    }

    bool RegisterWrite::create(emu::RegisterBank& bank, uint16_t addr, void* context, emu::RegisterBank::Write8Func func)
    {
        return create(bank, addr, [=](int32_t tick, uint16_t addr, uint8_t value)
        {
            return func(context, tick, addr, value);
        });
    }

    void RegisterWrite::destroy()
    {
        if (mBank)
        {
            mBank->removeWriter(mAddr);
        }
        initialize();
    }

    RegisterWrite& RegisterWrite::operator = (const RegisterWrite& other)
    {
        destroy();
        mBank = other.mBank;
        mAddr = other.mAddr;
        mWrite8Method = other.mWrite8Method;
        return *this;
    }

    void RegisterWrite::onWrite8(void* context, int32_t ticks, uint16_t addr, uint8_t value)
    {
        auto& object = *static_cast<RegisterWrite*>(context);
        object.mWrite8Method(ticks, addr, value);
    }
}
