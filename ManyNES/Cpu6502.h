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
    uint8_t             sp;
    uint16_t            pc;
    int32_t             clk;
    uint8_t             flag_c;
    uint8_t             flag_z;
    uint8_t             flag_v;
    uint8_t             flag_n;
    uint8_t             imm;
    uint16_t            addr;
    MEMORY_BUS*         bus;
};

void cpu_initialize(CPU_STATE& cpu);
bool cpu_create(CPU_STATE& cpu, MEMORY_BUS& bus);
void cpu_destroy(CPU_STATE& cpu);
void cpu_reset(CPU_STATE& cpu);
void cpu_execute(CPU_STATE& cpu, int32_t clock);

class Cpu6502
{
public:
    Cpu6502();
    ~Cpu6502();
    bool create(MEMORY_BUS& bus);
    void destroy();
    void reset();
    void execute(int32_t cycles);
    uint16_t disassemble(char* buffer, size_t size, uint16_t addr);
    
    CPU_STATE& getState()
    {
        return mState;
    }

private:
    CPU_STATE   mState;
};

#endif
