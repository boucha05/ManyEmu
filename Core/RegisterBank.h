#pragma once

#include "Core.h"
#include "MemoryBus.h"
#include <functional>
#include <string>
#include <vector>

namespace emu
{
    class RegisterBank
    {
    public:
        enum class Access: uint8_t
        {
            None,
            Read,
            Write,
            ReadWrite,
        };

        typedef uint8_t (*Read8Func)(void* context, int32_t, uint16_t);
        typedef void(*Write8Func)(void* context, int32_t, uint16_t, uint8_t);

        RegisterBank();
        ~RegisterBank();
        bool create(emu::MemoryBus& memoryBus, uint16_t base, uint16_t size, Access access, bool absolute = false);
        void destroy();
        void clear();
        bool addRegister(uint16_t addr, void* context, Read8Func read8Func, const char* name, const char* description);
        bool addRegister(uint16_t addr, void* context, Read8Func read8Func, Write8Func write8Func, const char* name, const char* description);
        bool addRegister(uint16_t addr, void* context, Write8Func write8Func, const char* name, const char* description);
        void removeRegister(uint16_t addr);
        uint8_t read8(int32_t ticks, uint16_t addr);
        void write8(int32_t ticks, uint16_t addr, uint8_t value);

    private:
        struct Register
        {
            Register();
            Register(void* context, Read8Func read8, Write8Func write8, const std::string& name, const std::string& description);

            void*               mContext;
            Read8Func           mRead8;
            Write8Func          mWrite8;
            std::string         mName;
            std::string         mDescription;
        };

        emu::MemoryBus*         mMemory;
        uint16_t                mBase;
        std::vector<Register>   mRegisters;
        MEM_ACCESS_READ_WRITE   mMapping;
    };
}

