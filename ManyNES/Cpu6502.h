#ifndef __CPU_6502_H__
#define __CPU_6502_H__

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
    uint16_t            addr;
    MEMORY_BUS*         bus;
    uint32_t            master_clock_divider;
    uint32_t            insn_ticks[256];
};

void cpu_initialize(CPU_STATE& cpu);
bool cpu_create(CPU_STATE& cpu, MEMORY_BUS& bus, uint32_t master_clock_divider);
void cpu_destroy(CPU_STATE& cpu);
void cpu_reset(CPU_STATE& cpu);
void cpu_execute(CPU_STATE& cpu, int32_t num_ticks);

class Cpu6502
{
public:
    typedef void(*TimerCallback)(void* context, int32_t ticks);

    Cpu6502();
    ~Cpu6502();
    bool create(MEMORY_BUS& bus, uint32_t master_clock_divider);
    void destroy();
    void reset();
    void execute(int32_t numTicks);
    uint16_t disassemble(char* buffer, size_t size, uint16_t addr);
    void addTimedEvent(TimerCallback callback, void* context, int32_t ticks);
    
    CPU_STATE& getState()
    {
        return mState;
    }

private:
    CPU_STATE               mState;

    struct TimerEvent
    {
        TimerCallback       callback;
        void*               context;
    };
    typedef std::multimap<int32_t, TimerEvent> TimerQueue;
    TimerQueue              mTimers;
    int32_t                 mDesiredTicks;
    int32_t                 mExecutedTicks;
};

#endif
