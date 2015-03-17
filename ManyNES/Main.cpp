#define SDL_MAIN_HANDLED
#define GLEW_STATIC

#include <SDL.h>
#include <gl/glew.h>
#include <assert.h>
#include <string>
#include <vector>
#include <deque>
#include "InputController.h"
#include "InputManager.h"
#include "Path.h"
#include "Serialization.h"
#include "Stream.h"
#include "Tests.h"
#include "nes.h"

namespace
{
    bool initSDL()
    {
        SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO);
        return true;
    }

    static bool sdlInitialized = initSDL();

    class StandardController : public NES::InputController
    {
    public:
        StandardController(InputManager& inputManager)
            : mInputManager(&inputManager)
        {
        }

        virtual void dispose()
        {
            delete this;
        }

        virtual uint8_t readInput()
        {
            uint8_t buttonMask = 0;
            for (auto mapping : mInputs)
            {
                static const float threshold = 0.4f;
                float value = mInputManager->getInput(mapping.inputId);
                if (value < -threshold)
                    buttonMask |= mapping.buttonMaskMin;
                else if (value > threshold)
                    buttonMask |= mapping.buttonMaskMax;
            }
            return buttonMask;
        }

        virtual void addInput(uint32_t inputId, uint32_t buttonMaskMax, uint32_t buttonMaskMin = 0)
        {
            InputMapping mapping =
            {
                inputId,
                buttonMaskMax,
                buttonMaskMin,
            };
            mInputs.push_back(mapping);
        }

    private:
        struct InputMapping
        {
            uint32_t    inputId;
            uint32_t    buttonMaskMax;
            uint32_t    buttonMaskMin;
        };

        typedef std::vector<InputMapping> InputMappingTable;

        InputMappingTable   mInputs;
        InputManager*       mInputManager;
    };
}

class Application
{
public:
    struct Config
    {
        std::string     rom;
        std::string     recorded;
        std::string     saveFolder;
        uint32_t        frameSkip;
        uint32_t        replayBufferSize;
        uint32_t        replayFrameSeek;
        uint32_t        samplingRate;
        float           soundDelay;
        bool            playback;
        bool            saveAudio;
        bool            autoSave;
        bool            autoLoad;
        bool            profile;

        Config()
            : frameSkip(0)
            , replayBufferSize(10 * 1024 * 1024)
            , replayFrameSeek(5)
            , samplingRate(44100)
            , soundDelay(0.0500f)
            , playback(false)
            , saveAudio(false)
            , autoSave(false)
            , autoLoad(false)
            , profile(false)
        {
        }
    };

    Application();
    ~Application();
    void run(const Config& config);
    void terminate();

private:
    enum
    {
        Input_A,
        Input_B,
        Input_Select,
        Input_Start,
        Input_VerticalAxis,
        Input_HorizontalAxis,
        Input_PlayerCount,
    };

    enum
    {
        Input_Exit,
        Input_TimeDir,
        Input_Player1,
        Input_Player2 = Input_Player1 + Input_PlayerCount,
        Input_Count = Input_Player2 + Input_PlayerCount,
    };

    bool create();
    void destroy();
    bool createSound(const std::string& romName);
    void destroySound();
    bool loadGameData(const std::string& path);
    bool saveGameData(const std::string& path);
    bool loadGameState(const std::string& path);
    bool saveGameState(const std::string& path);
    void serializeGameState(NES::ISerializer& serializer);
    void handleEvents();
    void update();
    void render();
    void audioCallback(int16_t* data, uint32_t size);
    static void audioCallback(void* userData, Uint8* stream, int len);

    static const uint32_t BUFFERING = 2;

    Config                      mConfig;
    std::string                 mGameDataPath;
    std::string                 mGameStatePath;
    SDL_Window*                 mWindow;
    SDL_Renderer*               mRenderer;
    SDL_Surface*                mScreen;
    SDL_Texture*                mTexture[BUFFERING];
    bool                        mValid;
    bool                        mFirst;
    uint32_t                    mBufferIndex;
    uint32_t                    mFrameIndex;
    uint32_t                    mBufferCount;
    SDL_AudioDeviceID           mSoundDevice;
    std::vector<int16_t>        mSoundBuffer;
    std::vector<int16_t>        mSoundQueue;
    uint32_t                    mSoundReadPos;
    uint32_t                    mSoundWritePos;
    uint32_t                    mSoundBufferedSize;
    uint32_t                    mSoundStartSize;
    uint32_t                    mSoundMinSize;
    uint32_t                    mSoundMaxSize;
    bool                        mSoundRunning;
    NES::FileStream*            mSoundFile;
    NES::BinaryWriter*          mSoundWriter;
    NES::FileStream*            mTimingFile;
    NES::TextWriter*            mTimingWriter;
    NES::Rom*                   mRom;
    NES::Context*               mContext;
    InputManager                mInputManager;
    KeyboardDevice*             mKeyboard;
    GamepadDevice*              mGamepad;
    StandardController*         mPlayer1Controller;
    NES::InputRecorder*         mInputRecorder;
    NES::InputPlayback*         mInputPlayback;
    NES::InputController*       mPlayer1;

    struct Playback
    {
        Playback(size_t size)
            : elapsedFrames(0)
            , seekCapacity(0)
            , stream(size)
        {
        }

        typedef std::deque<size_t> SeekQueue;

        uint32_t                    elapsedFrames;
        uint32_t                    seekCapacity;
        NES::CircularMemoryStream   stream;
        SeekQueue                   seekQueue;
    };
    Playback*                   mPlayback;
};

Application::Application()
    : mWindow(nullptr)
    , mRenderer(nullptr)
    , mScreen(nullptr)
    , mValid(false)
    , mFirst(true)
    , mBufferIndex(0)
    , mFrameIndex(0)
    , mBufferCount(0)
    , mSoundDevice(0)
    , mSoundReadPos(0)
    , mSoundWritePos(0)
    , mSoundBufferedSize(0)
    , mSoundStartSize(0)
    , mSoundMinSize(0)
    , mSoundMaxSize(0)
    , mSoundRunning(false)
    , mSoundFile(nullptr)
    , mSoundWriter(nullptr)
    , mTimingFile(nullptr)
    , mTimingWriter(nullptr)
    , mRom(nullptr)
    , mContext(nullptr)
    , mKeyboard(nullptr)
    , mGamepad(nullptr)
    , mPlayer1Controller(nullptr)
    , mInputRecorder(nullptr)
    , mInputPlayback(nullptr)
    , mPlayer1(nullptr)
    , mPlayback(nullptr)
{
    for (uint32_t n = 0; n < BUFFERING; ++n)
        mTexture[n] = nullptr;
}

Application::~Application()
{
    destroy();
}

void Application::run(const Config& config)
{
    mConfig = config;
    mValid = create();
    while (mValid)
    {
        handleEvents();
        update();
        render();
    }
    destroy();
}

void Application::terminate()
{
    mValid = false;
}

bool Application::create()
{
    std::string romName;
    std::string romExtension;
    std::string romFilename;
    std::string romDirectory;
    Path::split(mConfig.rom, romDirectory, romFilename);
    Path::splitExt(romFilename, romName, romExtension);
    if (romName.empty())
        return false;

    std::string saveName = mGameDataPath = Path::join(mConfig.saveFolder, romName);

    mWindow = SDL_CreateWindow("ManyNES", 100, 100, 256 * 2, 240 * 2, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!mWindow)
        return false;

    mRenderer = SDL_CreateRenderer(mWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!mRenderer)
        return false;

    mScreen = SDL_GetWindowSurface(mWindow);
    if (!mScreen)
        return false;

    for (uint32_t n = 0; n < BUFFERING; ++n)
    {
        mTexture[n] = SDL_CreateTexture(mRenderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, 256, 240);
        if (!mTexture[n])
            return false;
    }

#if 0
    if (!runTestRoms())
        return false;
#endif

    mRom = NES::Rom::load(mConfig.rom.c_str());
    if (!mRom)
        return false;

    mContext = NES::Context::create(*mRom);
    if (!mContext)
        return false;

    mGameDataPath = saveName + ".dat";
    loadGameData(mGameDataPath);

    mGameStatePath = saveName + ".sav";
    if (mConfig.autoLoad)
        loadGameState(mGameStatePath);

    mInputManager.create(Input_Count);

    mKeyboard = KeyboardDevice::create();
    if (!mKeyboard)
        return false;

    mKeyboard->addKey(SDL_SCANCODE_ESCAPE, Input_Exit);
    mKeyboard->addKey(SDL_SCANCODE_BACKSPACE, Input_TimeDir, -1.0f, +1.0f);

    mKeyboard->addKey(SDL_SCANCODE_UP, Input_Player1 + Input_VerticalAxis, -1.0f);
    mKeyboard->addKey(SDL_SCANCODE_DOWN, Input_Player1 + Input_VerticalAxis, +1.0f);
    mKeyboard->addKey(SDL_SCANCODE_LEFT, Input_Player1 + Input_HorizontalAxis, -1.0f);
    mKeyboard->addKey(SDL_SCANCODE_RIGHT, Input_Player1 + Input_HorizontalAxis, +1.0f);
    mKeyboard->addKey(SDL_SCANCODE_KP_8, Input_Player1 + Input_VerticalAxis, -1.0f);
    mKeyboard->addKey(SDL_SCANCODE_KP_2, Input_Player1 + Input_VerticalAxis, +1.0f);
    mKeyboard->addKey(SDL_SCANCODE_KP_4, Input_Player1 + Input_HorizontalAxis, -1.0f);
    mKeyboard->addKey(SDL_SCANCODE_KP_6, Input_Player1 + Input_HorizontalAxis, +1.0f);
    mKeyboard->addKey(SDL_SCANCODE_X, Input_Player1 + Input_B);
    mKeyboard->addKey(SDL_SCANCODE_Z, Input_Player1 + Input_A);
    mKeyboard->addKey(SDL_SCANCODE_A, Input_Player1 + Input_Select);
    mKeyboard->addKey(SDL_SCANCODE_S, Input_Player1 + Input_Start);

    mGamepad = GamepadDevice::create(0);
    if (mGamepad)
    {
        mGamepad->addButton(SDL_CONTROLLER_BUTTON_DPAD_UP, Input_Player1 + Input_VerticalAxis, -1.0f);
        mGamepad->addButton(SDL_CONTROLLER_BUTTON_DPAD_DOWN, Input_Player1 + Input_VerticalAxis, +1.0f);
        mGamepad->addButton(SDL_CONTROLLER_BUTTON_DPAD_LEFT, Input_Player1 + Input_HorizontalAxis, -1.0f);
        mGamepad->addButton(SDL_CONTROLLER_BUTTON_DPAD_RIGHT, Input_Player1 + Input_HorizontalAxis, +1.0f);
        mGamepad->addAxis(SDL_CONTROLLER_AXIS_LEFTX, Input_Player1 + Input_HorizontalAxis);
        mGamepad->addAxis(SDL_CONTROLLER_AXIS_LEFTY, Input_Player1 + Input_VerticalAxis);
        mGamepad->addButton(SDL_CONTROLLER_BUTTON_BACK, Input_Player1 + Input_Select);
        mGamepad->addButton(SDL_CONTROLLER_BUTTON_START, Input_Player1 + Input_Start);
        mGamepad->addButton(SDL_CONTROLLER_BUTTON_A, Input_Player1 + Input_A);
        mGamepad->addButton(SDL_CONTROLLER_BUTTON_B, Input_Player1 + Input_B);
        mGamepad->addButton(SDL_CONTROLLER_BUTTON_X, Input_Player1 + Input_B);
        mGamepad->addButton(SDL_CONTROLLER_BUTTON_Y, Input_Player1 + Input_A);
    }

    mPlayer1Controller = new StandardController(mInputManager);
    mPlayer1Controller->addInput(Input_Player1 + Input_HorizontalAxis, NES::Context::ButtonRight, NES::Context::ButtonLeft);
    mPlayer1Controller->addInput(Input_Player1 + Input_VerticalAxis, NES::Context::ButtonDown, NES::Context::ButtonUp);
    mPlayer1Controller->addInput(Input_Player1 + Input_B, NES::Context::ButtonB);
    mPlayer1Controller->addInput(Input_Player1 + Input_A, NES::Context::ButtonA);
    mPlayer1Controller->addInput(Input_Player1 + Input_Select, NES::Context::ButtonSelect);
    mPlayer1Controller->addInput(Input_Player1 + Input_Start, NES::Context::ButtonStart);
    mPlayer1 = mPlayer1Controller;

    if (!mConfig.recorded.empty())
    {
        if (mConfig.playback)
        {
            mInputPlayback = NES::InputPlayback::create(*mPlayer1);
            if (!mInputPlayback)
                return false;
            if (!mInputPlayback->load(mConfig.recorded.c_str()))
                return false;
            mPlayer1 = mInputPlayback;
        }
        else
        {
            mInputRecorder = NES::InputRecorder::create(*mPlayer1);
            if (!mInputRecorder)
                return false;
            mPlayer1 = mInputRecorder;
        }
    }

    if (mConfig.profile)
    {
        std::string path = saveName + ".prof";
        mTimingFile = new NES::FileStream(path.c_str(), "w");
        if (!mTimingFile->valid())
            return false;

        mTimingWriter = new NES::TextWriter(*mTimingFile);
    }

    mPlayback = new Playback(mConfig.replayBufferSize);

    if (!createSound(romName))
        return false;

    mFirst = true;

    return true;
}

void Application::destroy()
{
    destroySound();

    mPlayer1 = nullptr;

    if (mPlayback)
    {
        delete mPlayback;
        mPlayback = nullptr;
    }

    if (mTimingWriter)
    {
        delete mTimingWriter;
        mTimingWriter = nullptr;
    }

    if (mTimingFile)
    {
        delete mTimingFile;
        mTimingFile = nullptr;
    }

    if (mInputRecorder)
    {
        if (!mConfig.recorded.empty())
            mInputRecorder->save(mConfig.recorded.c_str());
        mInputRecorder->dispose();
        mInputRecorder = nullptr;
    }

    if (mPlayer1Controller)
    {
        delete mPlayer1Controller;
        mPlayer1Controller = nullptr;
    }

    if (mGamepad)
    {
        mGamepad->dispose();
        mGamepad = nullptr;
    }

    if (mKeyboard)
    {
        mKeyboard->dispose();
        mKeyboard = nullptr;
    }

    mInputManager.destroy();

    if (mContext)
    {
        if (mConfig.autoSave && !mFirst)
            saveGameState(mGameStatePath);
        saveGameData(mGameDataPath);

        mContext->dispose();
        mContext = nullptr;
    }

    if (mRom)
    {
        mRom->dispose();
        mRom = nullptr;
    }

    for (uint32_t n = 0; n < BUFFERING; ++n)
    {
        if (mTexture[n])
        {
            SDL_DestroyTexture(mTexture[n]);
            mTexture[n] = nullptr;
        }
    }

    if (mRenderer)
    {
        SDL_DestroyRenderer(mRenderer);
        mRenderer = nullptr;
    }

    if (mWindow)
    {
        SDL_DestroyWindow(mWindow);
        mWindow = nullptr;
    }
}

bool Application::createSound(const std::string& romName)
{
    SDL_AudioInit(NULL);

    SDL_AudioSpec audioDesired;
    SDL_AudioSpec audioObtained;
    SDL_zero(audioDesired);
    SDL_zero(audioObtained);
    audioDesired.freq = mConfig.samplingRate;
    audioDesired.format = AUDIO_S16;
    audioDesired.channels = 1;
    audioDesired.samples = 1024;
    audioDesired.callback = audioCallback;
    audioDesired.userdata = this;
    mSoundDevice = SDL_OpenAudioDevice(NULL, 0, &audioDesired, &audioObtained, 0);
    if (mSoundDevice <= 0)
        return false;

    mSoundBuffer.clear();
    mSoundBuffer.resize(mConfig.samplingRate / 60, 0);
    if (mConfig.saveAudio)
    {
        std::string path = Path::join(mConfig.saveFolder, romName) + ".snd";
        mSoundFile = new NES::FileStream(path.c_str(), "wb");
        if (!mSoundFile->valid())
            return false;

        mSoundWriter = new NES::BinaryWriter(*mSoundFile);
    }

    mSoundReadPos = 0;
    mSoundWritePos = 0;
    mSoundBufferedSize = 0;
    mSoundStartSize = static_cast<uint32_t>(mConfig.samplingRate * mConfig.soundDelay);
    mSoundMinSize = mSoundStartSize >> 1;
    mSoundMaxSize = mSoundStartSize + mSoundMinSize;
    mSoundRunning = false;
    mSoundQueue.clear();
    mSoundQueue.resize(((mSoundStartSize + mSoundBuffer.size() + 1023) * 2) & ~1023);
    SDL_PauseAudioDevice(mSoundDevice, 0);

    return true;
}

void Application::destroySound()
{
    if (mSoundWriter)
    {
        delete mSoundWriter;
        mSoundWriter = nullptr;
    }
    if (mSoundFile)
    {
        delete mSoundFile;
        mSoundFile = nullptr;
    }
    mSoundBuffer.clear();

    if (mSoundDevice)
    {
        SDL_CloseAudioDevice(mSoundDevice);
        mSoundDevice = 0;
    }

    SDL_AudioQuit();
}

bool Application::loadGameData(const std::string& path)
{
    if (path.empty())
        return false;

    NES::FileStream stream(path.c_str(), "rb");
    if (!stream.valid())
        return false;

    NES::BinaryReader reader(stream);
    mContext->serializeGameData(reader);
    return true;
}

bool Application::saveGameData(const std::string& path)
{
    if (path.empty())
        return false;

    NES::MemoryStream streamTemp;
    NES::BinaryWriter writer(streamTemp);
    mContext->serializeGameData(writer);
    if (streamTemp.getSize() <= 0)
        return false;

    NES::FileStream streamFinal(path.c_str(), "wb");
    bool success = streamFinal.write(streamTemp.getBuffer(), streamTemp.getSize());
    return success;
}

bool Application::loadGameState(const std::string& path)
{
    if (path.empty())
        return false;

    NES::FileStream stream(path.c_str(), "rb");
    if (!stream.valid())
        return false;

    NES::BinaryReader reader(stream);
    serializeGameState(reader);
    return true;
}

bool Application::saveGameState(const std::string& path)
{
    if (path.empty())
        return false;

    NES::MemoryStream streamTemp;
    NES::BinaryWriter writer(streamTemp);
    serializeGameState(writer);
    if (streamTemp.getSize() <= 0)
        return false;

    NES::FileStream streamFinal(path.c_str(), "wb");
    bool success = streamFinal.write(streamTemp.getBuffer(), streamTemp.getSize());
    return success;
}

void Application::serializeGameState(NES::ISerializer& serializer)
{
    uint32_t version = 1;
    serializer.serialize(version);
    mContext->serializeGameState(serializer);
}

void Application::handleEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
            terminate();
    }
}

void Application::update()
{
    static uint32_t frameTrigger = 640;

    mInputManager.clearInputs();
    mKeyboard->update(mInputManager);
    if (mGamepad)
        mGamepad->update(mInputManager);

    if (mInputManager.isPressed(Input_Exit))
        terminate();

    void* pixels = nullptr;
    int pitch = 0;
    if (!mConfig.frameSkip)
    {
        if (!SDL_LockTexture(mTexture[mBufferIndex], nullptr, &pixels, &pitch))
            mContext->setRenderSurface(pixels, pitch);
    }
    else
    {
        mContext->setRenderSurface(nullptr, 0);
    }

    if (mFrameIndex == frameTrigger)
        frameTrigger = frameTrigger;

    if (mPlayer1)
        mContext->setController(0, mPlayer1->readInput());
    if (mSoundBuffer.size())
    {
        size_t size = mSoundBuffer.size();
        mSoundBuffer.clear();
        mSoundBuffer.resize(size, 0);
        mContext->setSoundBuffer(&mSoundBuffer[0], mSoundBuffer.size());
    }
    
    uint64_t frameStart = SDL_GetPerformanceCounter();
    mContext->update();
    uint64_t frameEnd = SDL_GetPerformanceCounter();

    if (pixels)
        SDL_UnlockTexture(mTexture[mBufferIndex]);

    if (mSoundWriter)
    {
        for (uint32_t pos = 0; pos < mSoundBuffer.size(); ++pos)
        {
            if (mSoundBuffer[pos] != 0)
                pos = pos;
        }
        mSoundWriter->serialize(&mSoundBuffer[0], mSoundBuffer.size() * 2);
    }
    
    SDL_LockAudioDevice(mSoundDevice);
    if (mSoundBufferedSize < mSoundMaxSize)
    {
        uint32_t soundQueueSize = mSoundQueue.size();
        uint32_t size = mSoundBuffer.size();
        const int16_t* data = &mSoundBuffer[0];
        while (size)
        {
            uint32_t currentSize = size;
            uint32_t maxSize = soundQueueSize - mSoundWritePos;
            if (currentSize > maxSize)
                currentSize = maxSize;
            memcpy(&mSoundQueue[mSoundWritePos], data, currentSize * sizeof(int16_t));
            mSoundBufferedSize += currentSize;
            mSoundWritePos += currentSize;
            data += currentSize;
            size -= currentSize;
            if (mSoundWritePos >= soundQueueSize)
                mSoundWritePos -= soundQueueSize;
        }
    }
    SDL_UnlockAudioDevice(mSoundDevice);

    float frameTime = static_cast<float>(frameEnd - frameStart) / SDL_GetPerformanceFrequency();
    if (mTimingWriter)
    {
        mTimingWriter->write("%7d %6.3f\n", mFrameIndex, frameTime * 1000.0f);
    }

    float timeDir = mInputManager.getInput(Input_TimeDir);
    if (timeDir < -0.0001f)
    {
        if (!mPlayback->seekQueue.empty())
        {
            size_t size = mPlayback->seekQueue.back();
            mPlayback->seekQueue.pop_back();
            mPlayback->seekCapacity -= size;
            assert(mPlayback->seekCapacity >= 0);
            bool valid = mPlayback->stream.setReadOffset(size);
            if (valid)
            {
                NES::BinaryReader reader(mPlayback->stream);
                mContext->reset();
                mContext->serializeGameState(reader);
                mPlayback->stream.rewind(size);
            }
        }
    }

    if (++mPlayback->elapsedFrames >= mConfig.replayFrameSeek)
    {
        NES::BinaryWriter writer(mPlayback->stream);
        mPlayback->stream.setReadOffset(0);
        mContext->serializeGameState(writer);
        size_t size = mPlayback->stream.getReadOffset();
        mPlayback->seekQueue.push_back(size);
        mPlayback->seekCapacity += size;
        while (mPlayback->seekCapacity > mConfig.replayBufferSize)
        {
            assert(!mPlayback->seekQueue.empty());
            size_t size = mPlayback->seekQueue.front();
            mPlayback->seekQueue.pop_front();
            mPlayback->seekCapacity -= size;
            assert(mPlayback->seekCapacity >= 0);
        }
        mPlayback->elapsedFrames = 0;
    }

    ++mFrameIndex;
}

void Application::render()
{
    if (++mBufferIndex >= BUFFERING)
        mBufferIndex = 0;

    bool visible = !mFirst;
    mFirst = false;
    if (mConfig.frameSkip)
    {
        --mConfig.frameSkip;
        visible = false;
    }

    if (visible)
    {
        SDL_RenderCopy(mRenderer, mTexture[mBufferIndex], nullptr, nullptr);
        SDL_RenderPresent(mRenderer);
    }
}

void Application::audioCallback(int16_t* data, uint32_t size)
{
    // Fill with empty sound if we are not ready to run
    if ((!mSoundRunning && (mSoundBufferedSize < mSoundStartSize)) || (mSoundRunning && (mSoundBufferedSize < mSoundMinSize)))
    {
        mSoundRunning = false;
        memset(data, 0, 2 * size);
        return;
    }

    mSoundRunning = true;
    uint32_t soundQueueSize = mSoundQueue.size();
    while (size)
    {
        uint32_t currentSize = size;
        uint32_t maxSize = soundQueueSize - mSoundReadPos;
        if (currentSize > maxSize)
            currentSize = maxSize;
        memcpy(data, &mSoundQueue[mSoundReadPos], currentSize * sizeof(int16_t));
        mSoundBufferedSize -= currentSize;
        mSoundReadPos += currentSize;
        data += currentSize;
        size -= currentSize;
        if (mSoundReadPos >= soundQueueSize)
            mSoundReadPos -= soundQueueSize;
    }
}

void Application::audioCallback(void* userData, Uint8* stream, int len)
{
    static_cast<Application*>(userData)->audioCallback(reinterpret_cast<int16_t*>(stream), len >> 1);
}

int main()
{
    Application::Config config;
    config.saveFolder = "Save";
    //config.saveAudio = true;
    config.rom = "C:\\Emu\\NES\\roms\\rygar.nes";
    //config.rom = "ROMs\\exitbike.nes";
    //config.rom = "ROMs\\megaman2.nes";
    //config.rom = "ROMs\\mario.nes";
    //config.rom = "ROMs\\nestest.nes";
    //config.recorded = "ROMs\\recorded.dat";
    //config.playback = true;
    //config.frameSkip = 512;
    //config.autoSave = true;
    config.autoLoad = true;
    //config.profile = true;

    Application application;
    application.run(config);
    return 0;
}
