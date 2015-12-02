#include <Core/Path.h>
#include <Core/Serialization.h>
#include <Core/Stream.h>
#include "GameSession.h"
#include <Windows.h>

namespace
{
    static const char* FileExtensionData = ".dat";
    static const char* FileExtensionState = ".sav";

    int getExceptionFilter(void* context)
    {
        return EXCEPTION_EXECUTE_HANDLER;
    }
}

GameSession::GameSession(emu::IEmulator& emulator)
    : mEmulator(emulator)
    , mRom(nullptr)
    , mContext(nullptr)
    , mValid(false)
{
}

GameSession::~GameSession()
{
    unloadRom();
}

bool GameSession::loadRom(const std::string& path, const std::string& saveDirectory)
{
    std::string romName;
    std::string romExtension;
    std::string romFilename;
    std::string romDirectory;
    Path::split(path, romDirectory, romFilename);
    Path::splitExt(romFilename, romName, romExtension);
    if (romName.empty())
        return false;

    mRomPath = path;
    mSavePath = Path::join(saveDirectory, romName);
    mGameDataPath = mSavePath + FileExtensionData;
    mGameStatePath = mSavePath + FileExtensionState;

    mRom = mEmulator.loadRom(path.c_str());
    if (!mRom)
        return false;

    mContext = mEmulator.createContext(*mRom);
    if (!mContext)
        return false;

    mValid = true;
    return true;
}

void GameSession::unloadRom()
{
    mValid = false;

    if (mContext)
    {
        mEmulator.destroyContext(*mContext);
        mContext = nullptr;
    }

    if (mRom)
    {
        mEmulator.unloadRom(*mRom);
        mRom = nullptr;
    }
}

bool GameSession::loadGameData()
{
    if (!mValid)
        return false;

    emu::FileStream stream(mGameDataPath.c_str(), "rb");
    if (!stream.valid())
        return false;

    emu::BinaryReader reader(stream);
    return mEmulator.serializeGameData(*mContext, reader);
}

bool GameSession::saveGameData()
{
    if (!mValid)
        return false;

    emu::MemoryStream streamTemp;
    emu::BinaryWriter writer(streamTemp);
    if (!mEmulator.serializeGameData(*mContext, writer))
        return false;
    if (streamTemp.getSize() <= 0)
        return false;

    emu::FileStream streamFinal(mGameDataPath.c_str(), "wb");
    bool success = streamFinal.write(streamTemp.getBuffer(), streamTemp.getSize());
    return success;
}

bool GameSession::loadGameState()
{
    if (!mValid)
        return false;

    emu::FileStream stream(mGameStatePath.c_str(), "rb");
    if (!stream.valid())
        return false;

    emu::BinaryReader reader(stream);
    return serializeGameState(reader);
}

bool GameSession::saveGameState()
{
    if (!mValid)
        return false;

    emu::MemoryStream streamTemp;
    emu::BinaryWriter writer(streamTemp);
    if (!serializeGameState(writer))
        return false;
    if (streamTemp.getSize() <= 0)
        return false;

    emu::FileStream streamFinal(mGameStatePath.c_str(), "wb");
    bool success = streamFinal.write(streamTemp.getBuffer(), streamTemp.getSize());
    return success;
}

bool GameSession::serializeGameState(emu::ISerializer& serializer)
{
    if (!mValid)
        return false;

    uint32_t version = 1;
    serializer.serialize(version);
    if (serializer.isReading())
    {
        if (!reset())
            return false;
    }
    return mEmulator.serializeGameState(*mContext, serializer);
}

bool GameSession::setRenderBuffer(void* buffer, size_t pitch)
{
    if (!mValid)
        return false;

    return mEmulator.setRenderBuffer(*mContext, buffer, pitch);
}

bool GameSession::setSoundBuffer(void* buffer, size_t size)
{
    if (!mValid)
        return false;

    return mEmulator.setSoundBuffer(*mContext, static_cast<int16_t*>(buffer), size);
}

bool GameSession::setController(uint32_t index, uint32_t value)
{
    if (!mValid)
        return false;

    return mEmulator.setController(*mContext, index, value);
}

bool GameSession::reset()
{
    if (!mValid)
        return false;

    return mEmulator.reset(*mContext);
}

bool GameSession::execute()
{
    if (!mValid)
        return false;

    bool success = true;
    __try
    {
        success = mEmulator.execute(*mContext);
    }
    __except (getExceptionFilter(GetExceptionInformation()))
    {
        printf("Dead!\n");
        mValid = false;
    }
    return success;
}
