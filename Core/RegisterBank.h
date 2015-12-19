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
        bool create(MemoryBus& memoryBus, uint16_t base, uint16_t size, Access access, bool absolute = false);
        void destroy();
        void clear();
        bool addReader(uint16_t addr, void* context, Read8Func func);
        bool addWriter(uint16_t addr, void* context, Write8Func func);
        void removeReader(uint16_t addr);
        void removeWriter(uint16_t addr);
        uint8_t read8(int32_t ticks, uint16_t addr);
        void write8(int32_t ticks, uint16_t addr, uint8_t value);

    private:
        struct Reader
        {
            Reader();
            Reader(void* context, Read8Func func);

            void*       mContext;
            Read8Func   mFunc;
            uint32_t    mCount;
        };

        struct Writer
        {
            Writer();
            Writer(void* context, Write8Func func);

            void*       mContext;
            Write8Func  mFunc;
            uint32_t    mCount;
        };

        struct Register
        {
            Reader              reader;
            Writer              writer;
        };

        MemoryBus*              mMemory;
        uint16_t                mBase;
        std::vector<Register>   mRegisters;
        MEM_ACCESS_READ_WRITE   mMapping;
    };

    class RegisterRead
    {
    public:
        typedef std::function<uint8_t(int32_t tick, uint16_t addr)> Read8Method;

        RegisterRead();
        ~RegisterRead();
        bool create(RegisterBank& bank, uint16_t addr, Read8Method read8);
        bool create(RegisterBank& bank, uint16_t addr, void* context, RegisterBank::Read8Func func);
        void destroy();
        RegisterRead& operator = (const RegisterRead& other);

        template <typename T>
        bool create(RegisterBank& bank, uint16_t addr, T& instance, RegisterBank::Read8Func func)
        {
            return create(bank, addr, [=, &instance](int32_t tick, uint16_t addr)
            {
                return func(&instance, tick, addr);
            });
        }

        template <typename TClass, typename TMethod>
        bool create(RegisterBank& bank, uint16_t addr, TClass& instance, TMethod method)
        {
            return create(bank, addr, [=, &instance](int32_t tick, uint16_t addr)
            {
                return (instance.*method)(tick, addr);
            });
        }

    private:
        void initialize();
        static uint8_t onRead8(void* context, int32_t ticks, uint16_t addr);

        RegisterBank*   mBank;
        uint16_t        mAddr;
        Read8Method     mRead8Method;
    };

    class RegisterWrite
    {
    public:
        typedef std::function<void(int32_t tick, uint16_t addr, uint8_t value)> Write8Method;

        RegisterWrite();
        ~RegisterWrite();
        bool create(RegisterBank& bank, uint16_t addr, Write8Method write8);
        bool create(RegisterBank& bank, uint16_t addr, void* context, RegisterBank::Write8Func func);
        void destroy();
        RegisterWrite& operator = (const RegisterWrite& other);

        template <typename T>
        bool create(RegisterBank& bank, uint16_t addr, T& instance, RegisterBank::Write8Func func)
        {
            return create(bank, addr, [=, &instance](int32_t tick, uint16_t addr, uint8_t value)
            {
                return func(&instance, tick, addr, value);
            });
        }

        template <typename TClass, typename TMethod>
        bool create(RegisterBank& bank, uint16_t addr, TClass& instance, TMethod method)
        {
            return create(bank, addr, [=, &instance](int32_t tick, uint16_t addr, uint8_t value)
            {
                return (instance.*method)(tick, addr, value);
            });
        }

    private:
        void initialize();
        static void onWrite8(void* context, int32_t ticks, uint16_t addr, uint8_t value);

        RegisterBank*   mBank;
        uint16_t        mAddr;
        Write8Method    mWrite8Method;
    };
}

