#ifndef __GAME_SESSION_H__
#define __GAME_SESSION_H__

#include <stdint.h>
#include <string>
#include "nes.h"

class GameSession
{
public:
    GameSession();
    ~GameSession();
    bool loadRom(const std::string& path, const std::string& saveDirectory);
    void unloadRom();
    bool loadGameData();
    bool saveGameData();
    bool loadGameState();
    bool saveGameState();
    void serializeGameState(emu::ISerializer& serializer);
    void setRenderBuffer(void* buffer, uint32_t pitch);
    void setSoundBuffer(void* buffer, size_t size);
    void setController(uint32_t index, uint8_t value);
    void execute();

    bool isValid() const
    {
        return mValid;
    }

private:
    std::string     mRomPath;
    std::string     mSavePath;
    std::string     mGameDataPath;
    std::string     mGameStatePath;
    NES::Rom*       mRom;
    NES::Context*   mContext;
    bool            mValid;
};

#endif
