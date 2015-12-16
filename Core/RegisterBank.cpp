#include "Core.h"
#include "RegisterBank.h"

namespace
{
    uint8_t nullRead8(void* context, int32_t ticks, uint16_t addr)
    {
        EMU_NOT_IMPLEMENTED();
        return 0x00;
    }

    void nullWrite8(void* context, int32_t ticks, uint16_t addr, uint8_t value)
    {
        EMU_NOT_IMPLEMENTED();
    }

    uint8_t regsRead8(void* context, int32_t ticks, uint32_t addr)
    {
        return static_cast<emu::RegisterBank*>(context)->read8(ticks, addr);
    }

    void regsWrite8(void* context, int32_t ticks, uint32_t addr, uint8_t value)
    {
        static_cast<emu::RegisterBank*>(context)->write8(ticks, addr, value);
    }
}

namespace emu
{
    RegisterBank::Register::Register()
        : mContext(nullptr)
        , mRead8(nullRead8)
        , mWrite8(nullWrite8)
        , mName("???")
        , mDescription("")
    {
    }

    RegisterBank::Register::Register(void* context, Read8Func read8, Write8Func write8, const std::string& name, const std::string& description)
        : mContext(context)
        , mRead8(read8)
        , mWrite8(write8)
        , mName(name)
        , mDescription(description)
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

    bool RegisterBank::addRegister(uint16_t addr, void* context, Read8Func read8Func, const char* name, const char* description)
    {
        return addRegister(addr, context, read8Func, nullWrite8, name, description);
    }

    bool RegisterBank::addRegister(uint16_t addr, void* context, Read8Func read8Func, Write8Func write8Func, const char* name, const char* description)
    {
        uint16_t offset = addr - mBase;
        EMU_VERIFY(mMemory && (offset < mRegisters.size()) && name && description);
        mRegisters[offset] = Register(context, read8Func, write8Func, name, description);
        return true;
    }

    bool RegisterBank::addRegister(uint16_t addr, void* context, Write8Func write8Func, const char* name, const char* description)
    {
        return addRegister(addr, context, nullRead8, write8Func, name, description);
    }

    void RegisterBank::removeRegister(uint16_t addr)
    {
        uint16_t offset = addr - mBase;
        if (offset < mRegisters.size())
            mRegisters[offset] = Register();
    }

    uint8_t RegisterBank::read8(int32_t ticks, uint16_t addr)
    {
        uint16_t offset = addr - mBase;
        EMU_ASSERT(offset < mRegisters.size());
        const auto& reg = mRegisters[offset];
        return reg.mRead8(reg.mContext, ticks, addr);
    }

    void RegisterBank::write8(int32_t ticks, uint16_t addr, uint8_t value)
    {
        uint16_t offset = addr - mBase;
        EMU_ASSERT(offset < mRegisters.size());
        const auto& reg = mRegisters[offset];
        return reg.mWrite8(reg.mContext, ticks, addr, value);
    }
}
