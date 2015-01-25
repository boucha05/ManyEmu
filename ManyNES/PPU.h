#ifndef __PPU_H__
#define __PPU_H__

#include <stdint.h>

namespace NES
{
    class PPU
    {
    public:
        PPU();
        ~PPU();
        bool create();
        void destroy();
        void reset();
        void update(void* surface, uint32_t pitch);
        uint8_t regRead(int32_t ticks, uint32_t addr);
        void regWrite(int32_t ticks, uint32_t addr, uint8_t value);
        void startVBlank();

    private:
        uint8_t     mRegPPUCTRL;
    };
}

#endif
