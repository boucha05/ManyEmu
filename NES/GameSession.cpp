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

GameSession::GameSession()
    : mRom(nullptr)
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

    mRom = nes::Rom::load(path.c_str());
    if (!mRom)
        return false;

    mContext = nes::Context::create(*mRom);
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
        mContext->dispose();
        mContext = nullptr;
    }

    if (mRom)
    {
        mRom->dispose();
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
    mContext->serializeGameData(reader);
    return true;
}

bool GameSession::saveGameData()
{
    if (!mValid)
        return false;

    emu::MemoryStream streamTemp;
    emu::BinaryWriter writer(streamTemp);
    mContext->serializeGameData(writer);
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
    serializeGameState(reader);
    return true;
}

bool GameSession::saveGameState()
{
    if (!mValid)
        return false;

    emu::MemoryStream streamTemp;
    emu::BinaryWriter writer(streamTemp);
    serializeGameState(writer);
    if (streamTemp.getSize() <= 0)
        return false;

    emu::FileStream streamFinal(mGameStatePath.c_str(), "wb");
    bool success = streamFinal.write(streamTemp.getBuffer(), streamTemp.getSize());
    return success;
}

void GameSession::serializeGameState(emu::ISerializer& serializer)
{
    if (!mValid)
        return;

    uint32_t version = 1;
    serializer.serialize(version);
    if (serializer.isReading())
        mContext->reset();
    mContext->serializeGameState(serializer);
}

void GameSession::setRenderBuffer(void* buffer, uint32_t pitch)
{
    if (!mValid)
        return;

    mContext->setRenderSurface(buffer, pitch);
}

void GameSession::setSoundBuffer(void* buffer, size_t size)
{
    if (!mValid)
        return;

    mContext->setSoundBuffer(static_cast<int16_t*>(buffer), size);
}

void GameSession::setController(uint32_t index, uint8_t value)
{
    if (!mValid)
        return;

    mContext->setController(index, value);
}

void GameSession::execute()
{
    if (!mValid)
        return;

    __try
    {
        mContext->update();
    }
    __except (getExceptionFilter(GetExceptionInformation()))
    {
        printf("Dead!\n");
        mValid = false;
    }
}
