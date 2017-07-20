#include "GameSession.h"
#include "Path.h"
#include <Core/Log.h>
#include <Core/Serializer.h>
#include <Core/Stream.h>
#include <Windows.h>

namespace
{
    static const char* FileExtensionData = ".dat";
    static const char* FileExtensionState = ".sav";

    int getExceptionFilter(void* context)
    {
        EMU_UNUSED(context);
        return EXCEPTION_EXECUTE_HANDLER;
    }
}

GameSession::GameSession()
    : mBackend(nullptr)
    , mEmulator(nullptr)
    , mRom(nullptr)
    , mContext(nullptr)
    , mValid(false)
{
}

GameSession::~GameSession()
{
    unloadRom();
}

bool GameSession::loadRom(IBackend& backend, const std::string& path, const std::string& saveDirectory)
{
    mBackend = &backend;
    mEmulator = &mBackend->getEmulator();

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

    mRom = mEmulator->loadRom(path.c_str());
    if (!mRom)
        return false;

    mContext = mEmulator->createContext(*mRom);
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
        mEmulator->destroyContext(*mContext);
        mContext = nullptr;
    }

    if (mRom)
    {
        mEmulator->unloadRom(*mRom);
        mRom = nullptr;
    }

    mBackend = nullptr;
    mEmulator = nullptr;
}

bool GameSession::getDisplaySize(uint32_t& sizeX, uint32_t& sizeY)
{
    if (!mValid)
        return false;
    return mContext->getDisplaySize(sizeX, sizeY);
}

bool GameSession::loadGameData()
{
    if (!mValid)
        return false;

    emu::FileStream stream(mGameDataPath.c_str(), "rb");
    if (!stream.valid())
        return false;

    emu::BinaryReader reader(stream);
    return mContext->serializeGameData(reader);
}

bool GameSession::saveGameData()
{
    if (!mValid)
        return false;

    emu::MemoryStream streamTemp;
    emu::BinaryWriter writer(streamTemp);
    if (!mContext->serializeGameData(writer))
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
    serializer.value("Version", version);
    if (serializer.isReading())
    {
        if (!reset())
            return false;
    }
    return mContext->serializeGameState(serializer);
}

bool GameSession::setRenderBuffer(void* buffer, size_t pitch)
{
    if (!mValid)
        return false;

    return mContext->setRenderBuffer(buffer, pitch);
}

bool GameSession::setSoundBuffer(void* buffer, size_t size)
{
    if (!mValid)
        return false;

    return mContext->setSoundBuffer(static_cast<int16_t*>(buffer), size);
}

bool GameSession::setController(uint32_t index, uint32_t value)
{
    if (!mValid)
        return false;

    return mContext->setController(index, value);
}

bool GameSession::reset()
{
    if (!mValid)
        return false;

    return mContext->reset();
}

bool GameSession::execute()
{
    if (!mValid)
        return false;

    bool success = true;
    __try
    {
        success = mContext->execute();
    }
    __except (getExceptionFilter(GetExceptionInformation()))
    {
        emu::Log::printf(emu::Log::Type::Error, "Dead!\n");
        mValid = false;
    }
    return success;
}
