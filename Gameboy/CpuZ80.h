#pragma once

#include <Core/Clock.h>
#include <Core/Core.h>
#include <stdint.h>
#include <map>
#include <vector>
#include "GB.h"

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
        class IInterruptListener
        {
        public:
            virtual void onInterruptEnable(int32_t tick) {}
            virtual void onInterruptDisable(int32_t tick) {}
        };

        class IStopListener
        {
        public:
            virtual void onStop(int32_t tick) {}
        };
        
        CpuZ80();
        ~CpuZ80();
        bool create(emu::Clock& clock, MEMORY_BUS& bus, uint32_t master_clock_divider, uint8_t defaultA);
        void destroy();
        void reset();
        void interrupt(int32_t tick, uint16_t addr);
        virtual void resetClock() override;
        virtual void advanceClock(int32_t ticks) override;
        virtual void setDesiredTicks(int32_t ticks) override;
        virtual void execute() override;
        uint16_t disassemble(char* buffer, size_t size, uint16_t addr);
        void serialize(emu::ISerializer& serializer);
        void addInterruptListener(IInterruptListener& listener);
        void removeInterruptListener(IInterruptListener& listener);
        void addStopListener(IStopListener& listener);
        void removeStopListener(IStopListener& listener);

    private:
        inline uint8_t read8(uint16_t addr);
        inline void write8(uint16_t addr, uint8_t value);
        inline void write16(uint16_t addr, uint16_t value);
        inline uint16_t read16(uint16_t addr);
        inline uint8_t fetch8();
        inline uint16_t fetch16();
        inline uint16_t fetchSigned8();
        inline uint16_t fetchPC();
        inline uint8_t peek8(uint16_t& addr);
        inline uint16_t peek16(uint16_t& addr);
        inline uint16_t peekSigned8(uint16_t& addr);
        inline void push8(uint8_t value);
        inline uint8_t pop8();
        inline void push16(uint16_t value);
        inline uint16_t pop16();

        struct addr
        {
            explicit addr(uint16_t _value)
                : value(_value)
            {
            }

            uint16_t    value;
        };

        // Flags
        inline void flags_z0hc(uint16_t result, uint8_t value1, uint8_t value2, uint8_t carry);
        inline void flags_z1hc(uint16_t result, uint8_t value1, uint8_t value2, uint8_t carry);
        inline void flags_z010(uint8_t result);
        inline void flags_z000(uint8_t result);
        inline void flags_z0h_(uint8_t result, uint8_t value);
        inline void flags_z1h_(uint8_t result, uint8_t value);
        inline void flags_z_0x(uint16_t result);
        inline void flags__11_();
        inline void flags__0hc(uint32_t result, uint16_t value1, uint16_t value2);
        inline void flags_00hc(uint16_t value1, uint16_t value2);
        inline void flags_000c(uint8_t carry);
        inline void flags_z00c(uint8_t result, uint8_t carry);
        inline void flags_z01_(uint8_t result);

        // GMB 8bit - Loadcommands
        void insn_ld(uint8_t& dest, uint8_t src);
        void insn_ld(addr& dest, uint8_t src);

        // GMB 16bit - Loadcommands
        void insn_ld(uint16_t& dest, uint16_t src);
        void insn_ld(addr& dest, uint16_t src);
        void insn_push(uint16_t src);
        void insn_pop(uint16_t& dest);

        // GMB 8bit - Arithmetic / logical Commands
        void insn_add(uint8_t src);
        void insn_adc(uint8_t src);
        void insn_sub(uint8_t src);
        void insn_sbc(uint8_t src);
        void insn_and(uint8_t src);
        void insn_xor(uint8_t src);
        void insn_or(uint8_t src);
        void insn_cp(uint8_t src);
        void insn_inc(uint8_t& dest);
        void insn_inc(addr& dest);
        void insn_dec(uint8_t& dest);
        void insn_dec(addr& dest);
        void insn_daa();
        void insn_cpl();

        // GMB 16bit - Arithmetic/logical Commands
        void insn_add(uint16_t src);
        void insn_inc(uint16_t& dest);
        void insn_dec(uint16_t& dest);
        void insn_add_sp(uint16_t src);
        void insn_ld_sp(uint16_t& dest, uint16_t src);

        // GMB Rotate - and Shift - Commands
        void insn_rlca();
        void insn_rla();
        void insn_rrca();
        void insn_rra();
        void insn_rlc(uint8_t& dest);
        void insn_rlc(addr& dest);
        void insn_rl(uint8_t& dest);
        void insn_rl(addr& dest);
        void insn_rrc(uint8_t& dest);
        void insn_rrc(addr& dest);
        void insn_rr(uint8_t& dest);
        void insn_rr(addr& dest);
        void insn_sla(uint8_t& dest);
        void insn_sla(addr& dest);
        void insn_swap(uint8_t& dest);
        void insn_swap(addr& dest);
        void insn_sra(uint8_t& dest);
        void insn_sra(addr& dest);
        void insn_srl(uint8_t& dest);
        void insn_srl(addr& dest);

        // GMB Singlebit Operation Commands
        void insn_bit(uint8_t mask, uint8_t src);
        void insn_set(uint8_t mask, uint8_t& dest);
        void insn_set(uint8_t mask, addr& dest);
        void insn_res(uint8_t mask, uint8_t& dest);
        void insn_res(uint8_t mask, addr& dest);

        // GMB CPU-Controlcommands
        void insn_ccf();
        void insn_scf();
        void insn_nop();
        void insn_halt();
        void insn_stop();
        void insn_di();
        void insn_ei();

        // GMB Jumpcommands
        void insn_jp(uint16_t dest);
        void insn_jp(bool cond, uint16_t dest);
        void insn_jr(uint16_t dest);
        void insn_jr(bool cond, uint16_t dest);
        void insn_call(uint16_t dest);
        void insn_call(bool cond, uint16_t dest);
        void insn_ret();
        void insn_ret(bool cond);
        void insn_reti();
        void insn_rst(uint8_t dest);
        void insn_invalid();

        void executeCB();
        void executeMain();
        inline void trace();

        void setIME(bool enable);

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
#if EMU_CONFIG_LITTLE_ENDIAN
                uint8_t     flags;
                uint8_t     a;
                uint8_t     c;
                uint8_t     b;
                uint8_t     e;
                uint8_t     d;
                uint8_t     l;
                uint8_t     h;
#else
                uint8_t     a;
                uint8_t     flags;
                uint8_t     b;
                uint8_t     c;
                uint8_t     d;
                uint8_t     e;
                uint8_t     h;
                uint8_t     l;
#endif
                uint16_t    reserved_sp;
                uint16_t    reserved_pc;
                uint8_t     ime;
                uint8_t     halted;
                uint8_t     stopped;
            }               r8;
        };

        typedef std::vector<IInterruptListener*> InterruptListeners;
        typedef std::vector<IStopListener*> StopListeners;

        Registers               mRegs;
        uint8_t                 mDefaultA;
        int32_t                 mDesiredTicks;
        int32_t                 mExecutedTicks;
        MEMORY_BUS*             mMemory;
        emu::Clock*             mClock;
        uint8_t                 mTicksMain[256];
        uint8_t                 mTicksCB[8];
        uint8_t                 mTicksCond_call;
        uint8_t                 mTicksCond_ret;
        uint8_t                 mTicksCond_jp;
        uint8_t                 mTicksCond_jr;
        int32_t                 mFrame;
        InterruptListeners      mInterruptListeners;
        StopListeners           mStopListeners;
    };
}
