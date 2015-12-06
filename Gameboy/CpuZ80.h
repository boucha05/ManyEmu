#pragma once

#include <Core/Clock.h>
#include <stdint.h>
#include <map>

struct MEMORY_BUS;

namespace emu
{
    class ISerializer;
}

namespace gb
{
    class CpuZ80 : public emu::Clock::IListener
    {
    public:
        CpuZ80();
        ~CpuZ80();
        bool create(emu::Clock& clock, MEMORY_BUS& bus, uint32_t master_clock_divider);
        void destroy();
        void reset();
        void advanceClock(int32_t ticks);
        void setDesiredTicks(int32_t ticks);
        void execute();
        uint16_t disassemble(char* buffer, size_t size, uint16_t addr);
        void serialize(emu::ISerializer& serializer);

    private:
        inline uint8_t read8(uint16_t addr);
        inline void write8(uint16_t addr, uint8_t value);
        inline uint16_t read16(uint16_t addr);
        inline uint8_t fetch8();
        inline uint16_t fetch16();
        inline uint16_t fetchPC();
        inline uint8_t peek8(uint16_t& addr);
        inline uint16_t peek16(uint16_t& addr);
        inline void push8(uint8_t value);
        inline uint8_t pop8();
        inline void push16(uint16_t value);
        inline uint16_t pop16();

        void executeCB();
        void executeMain();

        union Registers
        {
            struct
            {
                uint16_t    af;
                uint16_t    bc;
                uint16_t    de;
                uint16_t    hl;
                uint16_t    sp;
                uint16_t    pc;
            }               r16;

            struct
            {
                uint8_t     a;
                uint8_t     flags;
                uint8_t     b;
                uint8_t     c;
                uint8_t     d;
                uint8_t     e;
                uint8_t     h;
                uint8_t     l;
                uint16_t    reserved_sp;
                uint16_t    reserved_pc;
                uint8_t     flag_z;
                uint8_t     flag_n;
                uint8_t     flag_h;
                uint8_t     flag_c;
            }               r8;
        };

        Registers               mRegs;
        int32_t                 mDesiredTicks;
        int32_t                 mExecutedTicks;
        MEMORY_BUS*             mMemory;
        emu::Clock*             mClock;
    };
}
