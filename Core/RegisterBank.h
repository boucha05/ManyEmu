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

        typedef std::function<uint8_t(int32_t, uint16_t)> Read8Func;
        typedef std::function<void(int32_t, uint16_t, uint8_t)> Write8Func;

        RegisterBank();
        ~RegisterBank();
        bool create(emu::MemoryBus& memoryBus, uint16_t base, uint16_t size, Access access, bool absolute = false);
        void destroy();
        void clear();
        bool addRegister(uint16_t addr, Read8Func read8Func, const char* name, const char* description);
        bool addRegister(uint16_t addr, Read8Func read8Func, Write8Func write8Func, const char* name, const char* description);
        bool addRegister(uint16_t addr, Write8Func write8Func, const char* name, const char* description);
        void removeRegister(uint16_t addr);
        uint8_t read8(int32_t ticks, uint16_t addr);
        void write8(int32_t ticks, uint16_t addr, uint8_t value);

        template <typename TClass>
        bool addRegister(uint16_t addr, TClass& instance, uint8_t (TClass::*read8Func)(int32_t, uint16_t), const char* name, const char* description)
        {
            Read8Func read8 = std::bind(read8Func, instance, std::placeholders::_1, std::placeholders::_2);
            return addRegister(addr, read8, name, description);
        }

        template <typename TClass>
        bool addRegister(uint16_t addr, TClass& instance, uint8_t(TClass::*read8Func)(int32_t, uint16_t), void(TClass::*write8Func)(int32_t, uint16_t, uint8_t), const char* name, const char* description)
        {
            Read8Func read8 = std::bind(read8Func, instance, std::placeholders::_1, std::placeholders::_2);
            Write8Func write8 = std::bind(write8Func, instance, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
            return addRegister(addr, read8, write8, name, description);
        }

        template <typename TClass>
        bool addRegister(uint16_t addr, TClass& instance, void(TClass::*write8Func)(int32_t, uint16_t, uint8_t), const char* name, const char* description)
        {
            Write8Func write8 = std::bind(write8Func, instance, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
            return addRegister(addr, write8, name, description);
        }

    private:
        struct Register
        {
            Register();
            Register(Read8Func read8, Write8Func write8, const std::string& name, const std::string& description);

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

