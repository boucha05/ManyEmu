#ifndef __CPU_6502_H__
#define __CPU_6502_H__

#include <Core/Clock.h>
#include <stdint.h>
#include <map>

struct MEMORY_BUS;

struct CPU_STATE
{
    uint8_t             a;
    uint8_t             x;
    uint8_t             y;
    uint8_t             sr;
    uint8_t             sp;
    uint16_t            pc;
    int32_t             desired_ticks;
    int32_t             executed_ticks;
    uint8_t             flag_c;
    uint8_t             flag_z;
    uint8_t             flag_v;
    uint8_t             flag_n;
    bool                irq;
    MEMORY_BUS*         bus;
    uint32_t            master_clock_divider;
    uint32_t            insn_ticks[256];
};

void cpu_initialize(CPU_STATE& cpu);
bool cpu_create(CPU_STATE& cpu, MEMORY_BUS& bus, uint32_t master_clock_divider);
void cpu_destroy(CPU_STATE& cpu);
void cpu_reset(CPU_STATE& cpu);
void cpu_execute(CPU_STATE& cpu);

namespace emu
{
    class ISerializer;
}

namespace nes
{
    class Cpu6502 : public emu::Clock::IListener
    {
    public:
        Cpu6502();
        ~Cpu6502();
        bool create(emu::Clock& clock, MEMORY_BUS& bus, uint32_t master_clock_divider);
        void destroy();
        void reset();
        void irq(bool active);
        void nmi();
        void advanceClock(int32_t ticks);
        void setDesiredTicks(int32_t ticks);
        void execute();
        uint16_t disassemble(char* buffer, size_t size, uint16_t addr);
        void serialize(emu::ISerializer& serializer);

        CPU_STATE& getState()
        {
            return mState;
        }

    private:
        CPU_STATE               mState;
        emu::Clock*             mClock;
    };
}

#endif
