#include "GameSession.h"
#include "Path.h"
#include "Serialization.h"
#include "Stream.h"
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

    mRom = NES::Rom::load(path.c_str());
    if (!mRom)
        return false;

    mContext = NES::Context::create(*mRom);
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

    NES::FileStream stream(mGameDataPath.c_str(), "rb");
    if (!stream.valid())
        return false;

    NES::BinaryReader reader(stream);
    mContext->serializeGameData(reader);
    return true;
}

bool GameSession::saveGameData()
{
    if (!mValid)
        return false;

    NES::MemoryStream streamTemp;
    NES::BinaryWriter writer(streamTemp);
    mContext->serializeGameData(writer);
    if (streamTemp.getSize() <= 0)
        return false;

    NES::FileStream streamFinal(mGameDataPath.c_str(), "wb");
    bool success = streamFinal.write(streamTemp.getBuffer(), streamTemp.getSize());
    return success;
}

bool GameSession::loadGameState()
{
    if (!mValid)
        return false;

    NES::FileStream stream(mGameStatePath.c_str(), "rb");
    if (!stream.valid())
        return false;

    NES::BinaryReader reader(stream);
    serializeGameState(reader);
    return true;
}

bool GameSession::saveGameState()
{
    if (!mValid)
        return false;

    NES::MemoryStream streamTemp;
    NES::BinaryWriter writer(streamTemp);
    serializeGameState(writer);
    if (streamTemp.getSize() <= 0)
        return false;

    NES::FileStream streamFinal(mGameStatePath.c_str(), "wb");
    bool success = streamFinal.write(streamTemp.getBuffer(), streamTemp.getSize());
    return success;
}

void GameSession::serializeGameState(NES::ISerializer& serializer)
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

void GameSession::setSoundBuffer(void* buffer, uint32_t size)
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
