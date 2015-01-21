#ifndef __CPU_6502_H__
#define __CPU_6502_H__

#include <stdint.h>

struct MEMORY_BUS;

struct CPU_STATE
{
    uint8_t             a;
    uint8_t             x;
    uint8_t             y;
    uint8_t             sr;
    uint16_t            sp;
    uint16_t            pc;
    int32_t             clk;
    uint8_t             flag_c;
    uint8_t             flag_z;
    uint8_t             flag_v;
    uint8_t             flag_n;
    uint8_t             imm;
    uint16_t            addr;
    MEMORY_BUS*         bus;

    static const uint32_t MEM_SIZE_LOG2 = 16;
    static const uint32_t MEM_SIZE = 1 << MEM_SIZE_LOG2;
    static const uint32_t MEM_PAGE_SIZE_LOG2 = 10;
    static const uint32_t MEM_PAGE_SIZE = 1 << MEM_PAGE_SIZE_LOG2;
    static const uint32_t MEM_PAGE_COUNT = 16 - MEM_PAGE_SIZE_LOG2;
};

void cpu_initialize(CPU_STATE& cpu);
bool cpu_create(CPU_STATE& cpu, MEMORY_BUS& bus);
void cpu_destroy(CPU_STATE& cpu);
void cpu_execute(CPU_STATE& cpu, int32_t clock);

class Cpu6502
{
public:
    Cpu6502();
    ~Cpu6502();
    bool create(MEMORY_BUS& bus);
    void destroy();
    
    CPU_STATE& getState()
    {
        return mState;
    }

private:
    CPU_STATE   mState;
};

#endif
