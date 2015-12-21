#ifndef __GAME_SESSION_H__
#define __GAME_SESSION_H__

#include <stdint.h>
#include <string>
#include <Core/Core.h>
#include "Backend.h"

class GameSession
{
public:
    GameSession();
    ~GameSession();
    IBackend* getBackend() { return mBackend; }
    bool loadRom(IBackend& backend, const std::string& path, const std::string& saveDirectory);
    void unloadRom();
    bool getDisplaySize(uint32_t& sizeX, uint32_t& sizeY);
    bool loadGameData();
    bool saveGameData();
    bool loadGameState();
    bool saveGameState();
    bool serializeGameState(emu::ISerializer& serializer);
    bool setRenderBuffer(void* buffer, size_t pitch);
    bool setSoundBuffer(void* buffer, size_t size);
    bool setController(uint32_t index, uint32_t value);
    bool reset();
    bool execute();

    bool isValid() const
    {
        return mValid;
    }

private:
    IBackend*           mBackend;
    emu::IEmulator*     mEmulator;
    std::string         mRomPath;
    std::string         mSavePath;
    std::string         mGameDataPath;
    std::string         mGameStatePath;
    emu::Rom*           mRom;
    emu::Context*       mContext;
    bool                mValid;
};

#endif
