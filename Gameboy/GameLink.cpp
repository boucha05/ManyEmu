#include <Core/Serialization.h>
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

    bool GameLink::create(emu::RegisterBank& registersIO)
    {
        EMU_VERIFY(registersIO.addRegister(0x01, this, &GameLink::readSB, &GameLink::writeSB, "SB", "Serial Transfer Data"));
        EMU_VERIFY(registersIO.addRegister(0x02, this, &GameLink::readSC, &GameLink::writeSC, "SC", "Serial Transfer Control"));
        return true;
    }

    void GameLink::destroy()
    {
    }

    void GameLink::reset()
    {
        mRegSB = 0;
        mRegSC = 0;
    }

    void GameLink::serialize(emu::ISerializer& serializer)
    {
        serializer.serialize(mRegSB);
        serializer.serialize(mRegSC);
    }

    uint8_t GameLink::readSB(int32_t tick, uint16_t addr)
    {
        return mRegSB;
    }

    void GameLink::writeSB(int32_t tick, uint16_t addr, uint8_t value)
    {
        mRegSB = value;
    }

    uint8_t GameLink::readSC(int32_t tick, uint16_t addr)
    {
        return mRegSC;
    }

    void GameLink::writeSC(int32_t tick, uint16_t addr, uint8_t value)
    {
        mRegSC = value;
        if ((mRegSC & 0x81) == 0x81)
        {
            printf("%c", mRegSB);
        }
    }
}
