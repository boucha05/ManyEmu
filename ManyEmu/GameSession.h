#ifndef __GAME_SESSION_H__
#define __GAME_SESSION_H__

#include <stdint.h>
#include <string>
#include <Core/Core.h>
#include "Backend.h"

class GameSession
{
public:
    typedef emu::IContext::SystemInfo SystemInfo;
    typedef emu::IContext::DisplayInfo DisplayInfo;

    GameSession();
    ~GameSession();
    IBackend* getBackend() { return mBackend; }
    bool loadRom(IBackend& backend, const std::string& path, const std::string& saveDirectory);
    void unloadRom();
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

    const SystemInfo& getSystemInfo() const
    {
        return mSystemInfo;
    }

    const DisplayInfo& getDisplayInfo() const
    {
        return mDisplayInfo;
    }

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
    emu::IRom*          mRom;
    emu::IContext*      mContext;
    SystemInfo          mSystemInfo;
    DisplayInfo         mDisplayInfo;
    bool                mValid;
};

#endif
