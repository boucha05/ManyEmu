#define SDL_MAIN_HANDLED
#define GLEW_STATIC

#include <SDL.h>
#include <gl/glew.h>
#include <string>
#include <vector>
#include <deque>
#include <Core/InputController.h>
#include <Core/InputManager.h>
#include <Core/Job.h>
#include <Core/Path.h>
#include <Core/Serialization.h>
#include <Core/Stream.h>
#include "Backend.h"
#include "GameSession.h"
#include <Windows.h>

#define DUMP_ROM_LIST 0

#if DUMP_ROM_LIST
#include <io.h>
#include "Gameboy/GB.h"
#endif

namespace
{
    bool initSDL()
    {
        SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO);
        return true;
    }

    static bool sdlInitialized = initSDL();
}

class Application
{
public:
    struct Config
    {
        typedef std::vector<std::string> StringArray;

        StringArray     roms;               // ROM files to execute. Only the first rom specified is playable for now.
        std::string     romFolder;          // Location where the ROM files are to be found.
        std::string     recorded;           // File where inputs are recorded (created if playback is falsed, read if playback is true)
        std::string     saveFolder;         // Savegame folder
        uint32_t        frameSkip;          // Number of simulated frames to skip before the first frame is rendered (used for reproducing bugs faster)
        uint32_t        replayBufferSize;   // Size of buffer used to rewind game in time
        uint32_t        replayFrameSeek;    // Number of frames between two rewind snapshots
        uint32_t        samplingRate;       // Sound buffer sampling rate
        float           soundDelay;         // Sound delay in seconds
        bool            playback;           // Replay recorded controller input
        bool            saveAudio;          // Save audio to file (not very efficient, for debugging only)
        bool            autoSave;           // Automatically save game state before exiting game
        bool            autoLoad;           // Automatically load savegame at startup
        bool            profile;            // Saves some timings in a file called profiling.prof (for debugging only)
        bool            display;            // Display video output (enabled by default, for debugging only)

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
            , display(true)
        {
        }
    };

    Application();
    ~Application();
    void run(const Config& config);
    void terminate();

private:
    struct Game
    {
        IBackend*       mBackend;
        GameSession*    mSession;
    };
    
    bool create();
    void destroy();
    bool createSound();
    void destroySound();
    GameSession* createGameSession(const std::string& path, const std::string& saveDirectory);
    void destroyGameSession(GameSession& gameSession);
    void handleEvents();
    void update();
    void render();
    void audioCallback(int16_t* data, uint32_t size);
    static void audioCallback(void* userData, Uint8* stream, int len);

    static const uint32_t BUFFERING = 2;

    typedef std::vector<GameSession*> GameSessionArray;

    Config                      mConfig;
    JobScheduler                mJobScheduler;
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
    size_t                      mSoundReadPos;
    size_t                      mSoundWritePos;
    size_t                      mSoundBufferedSize;
    uint32_t                    mSoundStartSize;
    uint32_t                    mSoundMinSize;
    uint32_t                    mSoundMaxSize;
    bool                        mSoundRunning;
    emu::FileStream*            mSoundFile;
    emu::BinaryWriter*          mSoundWriter;
    emu::FileStream*            mTimingFile;
    emu::TextWriter*            mTimingWriter;
    InputManager                mInputManager;
    KeyboardDevice*             mKeyboard;
    GamepadDevice*              mGamepad;
    StandardController*         mPlayer1Controller;
    emu::InputRecorder*         mInputRecorder;
    emu::InputPlayback*         mInputPlayback;
    emu::InputController*       mPlayer1;

    GameSessionArray            mGameSessions;
    uint32_t                    mActiveGameSession;

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
        size_t                      seekCapacity;
        emu::CircularMemoryStream   stream;
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
    , mKeyboard(nullptr)
    , mGamepad(nullptr)
    , mPlayer1Controller(nullptr)
    , mInputRecorder(nullptr)
    , mInputPlayback(nullptr)
    , mPlayer1(nullptr)
    , mActiveGameSession(0)
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
    Path::makeDirs(mConfig.saveFolder);

    mJobScheduler.create(SDL_GetCPUCount());

    mWindow = SDL_CreateWindow("ManyEmu", 100, 100, 256 * 2, 240 * 2, (mConfig.display ? SDL_WINDOW_SHOWN : SDL_WINDOW_HIDDEN) | SDL_WINDOW_RESIZABLE);
    if (!mWindow)
        return false;

    mRenderer = SDL_CreateRenderer(mWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!mRenderer)
        return false;

    //SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

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
    mPlayer1 = mPlayer1Controller;

    if (!mConfig.recorded.empty())
    {
        if (mConfig.playback)
        {
            mInputPlayback = emu::InputPlayback::create(*mPlayer1);
            if (!mInputPlayback)
                return false;
            if (!mInputPlayback->load(mConfig.recorded.c_str()))
                return false;
            mPlayer1 = mInputPlayback;
        }
        else
        {
            mInputRecorder = emu::InputRecorder::create(*mPlayer1);
            if (!mInputRecorder)
                return false;
            mPlayer1 = mInputRecorder;
        }
    }

    if (mConfig.profile)
    {
        std::string path = Path::join(mConfig.saveFolder, "timings.prof");
        mTimingFile = new emu::FileStream(path.c_str(), "w");
        if (!mTimingFile->valid())
            return false;

        mTimingWriter = new emu::TextWriter(*mTimingFile);
    }

    mPlayback = new Playback(mConfig.replayBufferSize);

    if (!createSound())
        return false;

    mFirst = true;

    for (auto rom : mConfig.roms)
    {
        auto gameSession = createGameSession(Path::join(mConfig.romFolder, rom), mConfig.saveFolder);
        EMU_VERIFY(gameSession);
        mGameSessions.push_back(gameSession);
    }

    if (mGameSessions.size() > 0)
    {
        mGameSessions[mActiveGameSession]->getBackend()->configureController(*mPlayer1Controller, 0);
    }

    return true;
}

void Application::destroy()
{
    for (auto gameSession : mGameSessions)
        destroyGameSession(*gameSession);
    mGameSessions.clear();

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

    mJobScheduler.destroy();
}

bool Application::createSound()
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
        std::string path = Path::join(mConfig.saveFolder, "audio.snd");
        mSoundFile = new emu::FileStream(path.c_str(), "wb");
        if (!mSoundFile->valid())
            return false;

        mSoundWriter = new emu::BinaryWriter(*mSoundFile);
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

GameSession* Application::createGameSession(const std::string& path, const std::string& saveDirectory)
{
    std::string root;
    std::string ext;
    Path::splitExt(path, root, ext);
    Path::normalizeCase(ext);
    auto backend = BackendRegistry::getInstance().getBackend(ext.c_str());
    if (!backend)
        return nullptr;

    auto& gameSession = *new GameSession();
    if (!gameSession.loadRom(*backend, path, saveDirectory))
    {
        delete &gameSession;
        return nullptr;
    }
    gameSession.loadGameData();
    if (mConfig.autoLoad)
        gameSession.loadGameState();
    return &gameSession;
}

void Application::destroyGameSession(GameSession& gameSession)
{
    if (mConfig.autoSave && !mFirst)
        gameSession.saveGameState();
    gameSession.saveGameData();
    gameSession.unloadRom();
    delete &gameSession;
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

    if (mActiveGameSession >= mGameSessions.size())
        return;

    auto& gameSession = *mGameSessions[mActiveGameSession];
    if (!gameSession.isValid())
        return;

    void* pixels = nullptr;
    int pitch = 0;
    if (!mConfig.frameSkip)
    {
        if (!SDL_LockTexture(mTexture[mBufferIndex], nullptr, &pixels, &pitch))
            gameSession.setRenderBuffer(pixels, pitch);
    }
    else
    {
        gameSession.setRenderBuffer(nullptr, 0);
    }

    if (mFrameIndex == frameTrigger)
        frameTrigger = frameTrigger;

    if (mPlayer1)
        gameSession.setController(0, mPlayer1->readInput());
    if (mSoundBuffer.size())
    {
        size_t size = mSoundBuffer.size();
        mSoundBuffer.clear();
        mSoundBuffer.resize(size, 0);
        gameSession.setSoundBuffer(&mSoundBuffer[0], mSoundBuffer.size());
    }
    
    uint64_t frameStart = SDL_GetPerformanceCounter();
    gameSession.execute();
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
        auto soundQueueSize = mSoundQueue.size();
        auto size = mSoundBuffer.size();
        const int16_t* data = &mSoundBuffer[0];
        while (size)
        {
            auto currentSize = size;
            auto maxSize = soundQueueSize - mSoundWritePos;
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
            EMU_ASSERT(mPlayback->seekCapacity >= 0);
            bool valid = mPlayback->stream.setReadOffset(size);
            if (valid)
            {
                emu::BinaryReader reader(mPlayback->stream);
                gameSession.serializeGameState(reader);
                mPlayback->stream.rewind(size);
            }
        }
    }

    if (++mPlayback->elapsedFrames >= mConfig.replayFrameSeek)
    {
        emu::BinaryWriter writer(mPlayback->stream);
        mPlayback->stream.setReadOffset(0);
        gameSession.serializeGameState(writer);
        size_t size = mPlayback->stream.getReadOffset();
        mPlayback->seekQueue.push_back(size);
        mPlayback->seekCapacity += size;
        while (mPlayback->seekCapacity > mConfig.replayBufferSize)
        {
            EMU_ASSERT(!mPlayback->seekQueue.empty());
            size_t size = mPlayback->seekQueue.front();
            mPlayback->seekQueue.pop_front();
            mPlayback->seekCapacity -= size;
            EMU_ASSERT(mPlayback->seekCapacity >= 0);
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
        int screenSizeX = 0;
        int screenSizeY = 0;
        SDL_RenderClear(mRenderer);
        SDL_GetRendererOutputSize(mRenderer, &screenSizeX, &screenSizeY);

        SDL_Rect imageRect;
        imageRect.x = 0;
        imageRect.y = 0;
        SDL_QueryTexture(mTexture[mBufferIndex], nullptr, nullptr, &imageRect.w, &imageRect.h);

        if (mActiveGameSession <= mGameSessions.size())
        {
            auto& gameSession = *mGameSessions[mActiveGameSession];
            if (gameSession.isValid())
            {
                uint32_t sizeX;
                uint32_t sizeY;
                gameSession.getDisplaySize(sizeX, sizeY);
                imageRect.w = sizeX;
                imageRect.h = sizeY;
            }
        }

        SDL_Rect screenRect;
        screenRect.w = 0;
        screenRect.h = 0;
        screenRect.x = 0;
        screenRect.y = 0;

        int scaledX = screenSizeY * imageRect.w / imageRect.h;
        if (screenSizeX > scaledX)
        {
            screenRect.w = scaledX;
            screenRect.h = screenSizeY;
            screenRect.x = (screenSizeX - screenRect.w + 1) >> 1;
        }
        else
        {
            screenRect.w = screenSizeX;
            screenRect.h = screenSizeX * imageRect.h / imageRect.w;
            screenRect.y = (screenSizeY - screenRect.h + 1) >> 1;
        }

        SDL_RenderCopy(mRenderer, mTexture[mBufferIndex], &imageRect, &screenRect);
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
    size_t soundQueueSize = mSoundQueue.size();
    while (size)
    {
        size_t currentSize = size;
        size_t maxSize = soundQueueSize - mSoundReadPos;
        if (currentSize > maxSize)
            currentSize = maxSize;
        memcpy(data, &mSoundQueue[mSoundReadPos], currentSize * sizeof(int16_t));
        mSoundBufferedSize -= currentSize;
        mSoundReadPos += currentSize;
        data += currentSize;
        size -= static_cast<uint32_t>(currentSize);
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

    // Sorry, the UI is not implemented yet. For now, you will need to manually
    // configure the emulation right here
    
#if 1
    // Configuration for Gameboy development
    config.saveFolder = "C:\\Emu\\Gameboy\\save";
    config.romFolder = "C:\\Emu\\Gameboy\\roms";
    //config.roms.push_back("Tetris (V1.1) (JU) [!].gb");
    config.roms.push_back("Metroid 2 - Return of Samus (UA) [b1].gb");
    //config.frameSkip = 600;
    //config.display = false;
    //config.romFolder = "..\\Gameboy\\ROMs\\cpu_instrs\\individual";
    //config.roms.push_back("01-special.gb");             // passed
    //config.roms.push_back("02-interrupts.gb");          // Failed #4
    //config.roms.push_back("03-op sp,hl.gb");            // passed
    //config.roms.push_back("04-op r,imm.gb");            // passed
    //config.roms.push_back("05-op rp.gb");               // passed
    //config.roms.push_back("06-ld r,r.gb");              // passed
    //config.roms.push_back("07-jr,jp,call,ret,rst.gb");  // passed
    //config.roms.push_back("08-misc instrs.gb");         // passed
    //config.roms.push_back("09-op r,r.gb");              // passed
    //config.roms.push_back("10-bit ops.gb");             // passed
    //config.roms.push_back("11-op a,(hl).gb");           // passed
#else
    // Configuration for NES development
    config.saveFolder = "C:\\Emu\\NES\\save";
    //config.saveAudio = true;
    config.romFolder = "C:\\Emu\\NES\\roms";
    //config.roms.push_back("smb3.nes");
    //config.roms.push_back("exitbike.nes");
    config.roms.push_back("megaman2.nes");
    config.roms.push_back("mario.nes");
    config.roms.push_back("zelda2.nes");
    //config.recorded = "ROMs\\recorded.dat";
    //config.playback = true;
    //config.frameSkip = 512;
    //config.autoSave = true;
    config.autoLoad = true;
    //config.profile = true;
#endif

#if DUMP_ROM_LIST
    std::string filter = Path::join(config.romFolder, "*.gb*");
    _finddata_t data;
    auto handle = _findfirst(filter.c_str(), &data);
    int valid = 1;
    printf("File;ROM size;RAM size;Title;UseCGB;OnlyCGB;UseSGB;RAM;Battery;Timer;Rumble;Mapper;Destination;Cartridge type;Version;Licensee old;Licensee new;Manufacturer;Header checksum;Global checksum\n");
    while (handle && valid)
    {
        std::string path = Path::join(config.romFolder, data.name);
        gb::Rom::Description desc;
        gb::Rom::readDescription(desc, path.c_str());
        printf("%s;%d;%d;%s;%d;%d;%d;%d;%d;%d;%d;%s;%s;0x%02x;%d;0x%02x;0x%04x;0x%08x;0x%02x;0x%04x\n",
            data.name,
            desc.romSize,
            desc.ramSize,
            desc.title,
            desc.useCGB,
            desc.onlyCGB,
            desc.useSGB,
            desc.hasRam,
            desc.hasBattery,
            desc.hasTimer,
            desc.hasRumble,
            gb::Rom::getMapperName(desc.mapper),
            gb::Rom::getDestinationName(desc.destination),
            desc.cartridgeType,
            desc.version,
            desc.licenseeOld,
            desc.licenseeNew,
            desc.manufacturer,
            desc.headerChecksum,
            desc.globalChecksum);
        valid = !_findnext(handle, &data);
    }
#endif

    Application application;
    application.run(config);
    return 0;
}
