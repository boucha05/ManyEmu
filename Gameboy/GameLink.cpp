#include <Core/Log.h>
#include <Core/Serializer.h>
#include "GameLink.h"
#include "CpuZ80.h"

namespace gb
{
    GameLink::GameLink()
        : mRegSB(0x00)
        , mRegSC(0x00)
    {
    }

    GameLink::~GameLink()
    {
    }

    bool GameLink::create(emu::RegisterBank& registers)
    {
        EMU_VERIFY(mRegisterAccessors.read.SB.create(registers, 0x01, *this, &GameLink::readSB));
        EMU_VERIFY(mRegisterAccessors.write.SB.create(registers, 0x01, *this, &GameLink::writeSB));
        EMU_VERIFY(mRegisterAccessors.read.SC.create(registers, 0x02, *this, &GameLink::readSC));
        EMU_VERIFY(mRegisterAccessors.write.SC.create(registers, 0x02, *this, &GameLink::writeSC));
        return true;
    }

    void GameLink::destroy()
    {
        mRegisterAccessors = RegisterAccessors();
    }

    void GameLink::reset()
    {
        mRegSB = 0;
        mRegSC = 0;
    }

    void GameLink::serialize(emu::ISerializer& serializer)
    {
        serializer
            .value("RegSB", mRegSB)
            .value("RegSC", mRegSC);
    }

    uint8_t GameLink::readSB(int32_t tick, uint16_t addr)
    {
        EMU_UNUSED(tick);
        EMU_UNUSED(addr);
        return mRegSB;
    }

    void GameLink::writeSB(int32_t tick, uint16_t addr, uint8_t value)
    {
        EMU_UNUSED(tick);
        EMU_UNUSED(addr);
        mRegSB = value;
    }

    uint8_t GameLink::readSC(int32_t tick, uint16_t addr)
    {
        EMU_UNUSED(tick);
        EMU_UNUSED(addr);
        return mRegSC;
    }

    void GameLink::writeSC(int32_t tick, uint16_t addr, uint8_t value)
    {
        EMU_UNUSED(tick);
        EMU_UNUSED(addr);
        mRegSC = value;
        if ((mRegSC & 0x81) == 0x81)
        {
            emu::Log::printf(emu::Log::Type::Debug, "%c", mRegSB);
        }
    }
}
