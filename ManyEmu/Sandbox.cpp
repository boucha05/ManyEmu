#include <SDL.h>
#include <string>
#include <vector>
#include <deque>
#include <Core/InputController.h>
#include <Core/Log.h>
#include <Core/Serializer.h>
#include <Core/Stream.h>
#include "Backend.h"
#include "GameSession.h"
#include "GameView.h"
#include "InputManager.h"
#include "Path.h"
#include "Texture.h"

#define DUMP_ROM_LIST 0

#if DUMP_ROM_LIST
#include <io.h>
#include "Gameboy/GB.h"
#endif

#include "Sandbox.h"

namespace
{
    bool verifyMemory(const void* src1, const void* src2, size_t size, size_t& pos)
    {
        for (pos = 0; pos < size; ++pos)
        {
            if (static_cast<const uint8_t*>(src1)[pos] != static_cast<const uint8_t*>(src2)[pos])
                return false;
        }
        return true;
    }

    class SandboxImpl : public Sandbox
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
            bool            rewindEnabled;      // Enable rewind feature
            bool            playback;           // Replay recorded controller input
            bool            enableAudio;        // Enable audio
            bool            stubAudio;          // Redirect audio to a fake output
            bool            saveAudio;          // Save audio to file (not very efficient, for debugging only)
            bool            autoSave;           // Automatically save game state before exiting game
            bool            autoLoad;           // Automatically load savegame at startup
            bool            profile;            // Saves some timings in a file called profiling.prof (for debugging only)
            bool            display;            // Display video output (enabled by default, for debugging only)
            bool            stubDisplay;        // Use fake display
            bool            vsync;              // Wait vsync

            Config()
                : frameSkip(0)
                , replayBufferSize(10 * 1024 * 1024)
                , replayFrameSeek(5)
                , samplingRate(44100)
                , soundDelay(0.0500f)
                , rewindEnabled(true)
                , playback(false)
                , enableAudio(true)
                , stubAudio(false)
                , saveAudio(false)
                , autoSave(false)
                , autoLoad(false)
                , profile(false)
                , display(true)
                , stubDisplay(false)
                , vsync(true)
            {
            }
        };

        SandboxImpl();
        ~SandboxImpl();
        virtual bool create(Application& application) override;
        virtual void destroy(Application& application) override;

        virtual const char* getName() const
        {
            return "Sandbox";
        }

        virtual void onGUI() override
        {
            if (!mValid)
                return;
        }

        virtual void update() override;

        virtual void serialize(emu::ISerializer& serializer)
        {
            (void)serializer;
        }

    private:
        struct Game
        {
            IBackend*       mBackend;
            GameSession*    mSession;
        };

        void terminate();
        void overrideConfig();
        bool createSound();
        void destroySound();
        GameSession* createGameSession(Application& application, const std::string& path, const std::string& saveDirectory);
        void destroyGameSession(GameSession& gameSession);
        void audioCallback(int16_t* data, uint32_t size);
        static void audioCallback(void* userData, Uint8* stream, int len);

        typedef std::vector<GameSession*> GameSessionArray;

        Config                      mConfig;
        uint32_t                    mTexSizeX;
        uint32_t                    mTexSizeY;
        Texture                     mTexture;
        std::vector<uint8_t>        mFakeTexture;
        bool                        mValid;
        bool                        mFirst;
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
        emu::FileStream*            mTimingFile;
        InputManager                mInputManager;
        KeyboardDevice*             mKeyboard;
        GamepadDevice*              mGamepad;
        StandardController*         mPlayer1Controller;
        emu::InputRecorder*         mInputRecorder;
        emu::InputPlayback*         mInputPlayback;
        emu::InputController*       mPlayer1;
        emu::MemoryStream           mTestStream0;
        emu::MemoryStream           mTestStream1;
        emu::MemoryStream           mTestStream2;

        GameSessionArray            mGameSessions;
        uint32_t                    mActiveGameSession;

        GameView*                   mGameView;

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

    SandboxImpl::SandboxImpl()
        : mValid(false)
        , mFirst(true)
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
        , mTimingFile(nullptr)
        , mKeyboard(nullptr)
        , mGamepad(nullptr)
        , mPlayer1Controller(nullptr)
        , mInputRecorder(nullptr)
        , mInputPlayback(nullptr)
        , mPlayer1(nullptr)
        , mActiveGameSession(0)
        , mPlayback(nullptr)
        , mGameView(nullptr)
    {
    }

    SandboxImpl::~SandboxImpl()
    {
    }

    void SandboxImpl::terminate()
    {
        mValid = false;
    }

    bool SandboxImpl::create(Application& application)
    {
        overrideConfig();

        Path::makeDirs(mConfig.saveFolder);

        mTexSizeX = 256;
        mTexSizeY = 240;
        EMU_VERIFY(mTexture.create(mTexSizeX, mTexSizeY));
        mFakeTexture.resize(mTexSizeX * mTexSizeY * 4);

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
        }

        mPlayback = new Playback(mConfig.replayBufferSize);

        if (!createSound())
            return false;

        mFirst = true;

        for (auto rom : mConfig.roms)
        {
            auto gameSession = createGameSession(application, Path::join(mConfig.romFolder, rom), mConfig.saveFolder);
            EMU_VERIFY(gameSession);
            mGameSessions.push_back(gameSession);
        }

        if (mGameSessions.size() > 0)
        {
            mGameSessions[mActiveGameSession]->getBackend()->configureController(*mPlayer1Controller, 0);
        }

        mGameView = &application.getGameView();

        mValid = true;
        return true;
    }

    void SandboxImpl::destroy(Application& application)
    {
        EMU_UNUSED(application);

        mGameView = nullptr;

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
        mTexture.destroy();
    }

    bool SandboxImpl::createSound()
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
            std::string path = "..\\audio.snd";
            mSoundFile = new emu::FileStream(path.c_str(), "wb");
            if (!mSoundFile->valid())
                return false;
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

    void SandboxImpl::destroySound()
    {
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

    GameSession* SandboxImpl::createGameSession(Application& application, const std::string& path, const std::string& saveDirectory)
    {
        std::string root;
        std::string ext;
        Path::splitExt(path, root, ext);
        Path::normalizeCase(ext);
        auto backend = application.getBackendRegistry().getBackend(ext.c_str());
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

    void SandboxImpl::destroyGameSession(GameSession& gameSession)
    {
        if (mConfig.autoSave && !mFirst)
            gameSession.saveGameState();
        gameSession.saveGameData();
        gameSession.unloadRom();
        delete &gameSession;
    }

    void SandboxImpl::update()
    {
        if (mGameView)
            mGameView->clear();

        if (!mValid)
            return;

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
        gameSession.setRenderBuffer(nullptr, 0);
        if (mConfig.display && !mConfig.frameSkip)
        {
            if (!mConfig.stubDisplay)
            {
                pixels = mFakeTexture.data();
                pitch = mTexSizeX * 4;
            }
            gameSession.setRenderBuffer(mFakeTexture.data(), mTexSizeX * 4);
        }

        if (mFrameIndex == frameTrigger)
            frameTrigger = frameTrigger;

        if (mPlayer1)
            gameSession.setController(0, mPlayer1->readInput());
        if (mSoundBuffer.size() && mConfig.enableAudio)
        {
            size_t size = mSoundBuffer.size();
            mSoundBuffer.clear();
            mSoundBuffer.resize(size, 0);
            gameSession.setSoundBuffer(&mSoundBuffer[0], mSoundBuffer.size());
        }
        else
        {
            gameSession.setSoundBuffer(nullptr, 0);
        }

        static bool testGameState = false;
        if (testGameState)
        {
            // Capture state before running the frame
            mTestStream0.clear();
            emu::BinaryWriter writer(mTestStream0);
            gameSession.serializeGameState(writer);
        }

        uint64_t frameStart = SDL_GetPerformanceCounter();
        gameSession.execute();
        uint64_t frameEnd = SDL_GetPerformanceCounter();

        if (testGameState)
        {
            // Capture state after running the frame
            mTestStream1.clear();
            emu::BinaryWriter writer1(mTestStream1);
            gameSession.serializeGameState(writer1);

            // Rewind to the beginning of the frame
            emu::BinaryReader reader1(mTestStream0);
            gameSession.serializeGameState(reader1);

            // Execute the frame again
            gameSession.execute();

            // Capture the state after running the frame again
            mTestStream2.clear();
            emu::BinaryWriter writer2(mTestStream2);
            gameSession.serializeGameState(writer2);

            // Compare to make sure the result is the same
            size_t diffPos = 0;
            bool same = mTestStream1.getSize() == mTestStream2.getSize();
            same = same && verifyMemory(mTestStream1.getBuffer(), mTestStream2.getBuffer(), mTestStream1.getSize(), diffPos);
            if (!same)
            {
                emu::Log::printf(emu::Log::Type::Error, "Frame %d: serialization failed at offset %d\n!", mFrameIndex, static_cast<uint32_t>(diffPos));
                EMU_ASSERT(false);
                terminate();
            }
        }

        if (pixels)
        {
            mTexture.update(pixels, mFakeTexture.size());
        }

        if (mSoundFile)
        {
            for (uint32_t pos = 0; pos < mSoundBuffer.size(); ++pos)
            {
                if (mSoundBuffer[pos] != 0)
                    pos = pos;
            }
            mSoundFile->write(&mSoundBuffer[0], mSoundBuffer.size() * 2);
        }

        if (mConfig.enableAudio && !mConfig.stubAudio)
        {
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
        }

        float frameTime = static_cast<float>(frameEnd - frameStart) / SDL_GetPerformanceFrequency();
        if (mTimingFile)
        {
            char temp[64];
            sprintf(temp, "%7d %6.3f\n", mFrameIndex, frameTime * 1000.0f);
            mTimingFile->write(temp, strlen(temp));
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

        if (mConfig.rewindEnabled && (++mPlayback->elapsedFrames >= mConfig.replayFrameSeek))
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
                size_t localSize = mPlayback->seekQueue.front();
                mPlayback->seekQueue.pop_front();
                mPlayback->seekCapacity -= localSize;
                EMU_ASSERT(mPlayback->seekCapacity >= 0);
            }
            mPlayback->elapsedFrames = 0;
        }

        ++mFrameIndex;

        bool visible = !mFirst;
        mFirst = false;
        if (mConfig.frameSkip)
        {
            --mConfig.frameSkip;
            visible = false;
        }
        if (visible)
        {
            mGameView->setGameSession(*mGameSessions[mActiveGameSession], mTexture);
        }
    }

    void SandboxImpl::audioCallback(int16_t* data, uint32_t size)
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

    void SandboxImpl::audioCallback(void* userData, Uint8* stream, int len)
    {
        static_cast<SandboxImpl*>(userData)->audioCallback(reinterpret_cast<int16_t*>(stream), len >> 1);
    }

    void SandboxImpl::overrideConfig()
    {
        auto& config = mConfig;

        // Sorry, the UI is not fully implemented yet. For now, you will need to manually
        // configure the emulation right here

#if 1
    // Configuration for Gameboy development
        config.saveFolder = "C:\\Emu\\Gameboy\\save";
        config.romFolder = "C:\\Emu\\Gameboy\\roms";
        //config.roms.push_back("mario & yoshi (e) [o2].gb");
        //config.roms.push_back("Donkey Kong Country (UE) (M5) [C][t1].gbc");
        //config.roms.push_back("Legend of Zelda, The - Oracle of Ages (U) [C][h2].gbc");
        //config.roms.push_back("Donkey Kong Land 2 (UE) [S][b2].gb");
        //config.roms.push_back("Mario Tennis (U) [C][!].gbc");
        //config.roms.push_back("p-shntae.gbc");
        //config.roms.push_back("rayman (u) [c][t1].gbc");
        //config.roms.push_back("Asterix - Search for Dogmatix (E) (M6) [C][!].gbc");
        //config.roms.push_back("3D Pocket Pool (E) (M6) [C][!].gbc");
        //config.roms.push_back("super mario land (v1.1) (jua) [t1].gb");
        config.roms.push_back("Tetris (V1.1) (JU) [!].gb");
        //config.roms.push_back("Metroid 2 - Return of Samus (UA) [b1].gb");
        //config.roms.push_back("Legend of Zelda, The - Link's Awakening (V1.2) (U) [!].gb");
        //config.saveAudio = true;
#if 0
        config.recorded = "recorded.dat";
        config.playback = true;
        config.vsync = false;
        config.frameSkip = 100000;
        config.rewindEnabled = false;
        config.enableAudio = true;
        config.stubAudio = true;
        config.stubDisplay = true;
#endif
        //config.display = false;
        //config.autoSave = true;
        //config.autoLoad = true;
        //config.romFolder = "..\\Gameboy\\ROMs\\cpu_instrs\\individual";
        //config.roms.push_back("01-special.gb");             // passed
        //config.roms.push_back("02-interrupts.gb");          // passed
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
        emu::Log::printf(emu::Log::Type::Debug, "File;ROM size;RAM size;Title;UseCGB;OnlyCGB;UseSGB;RAM;Battery;Timer;Rumble;Mapper;Destination;Cartridge type;Version;Licensee old;Licensee new;Manufacturer;Header checksum;Global checksum\n");
        while (handle && valid)
        {
            std::string path = Path::join(config.romFolder, data.name);
            gb::Rom::Description desc;
            gb::Rom::readDescription(desc, path.c_str());
            emu::Log::printf(emu::Log::Type::Debug, "%s;%d;%d;%s;%d;%d;%d;%d;%d;%d;%d;%s;%s;0x%02x;%d;0x%02x;0x%04x;0x%08x;0x%02x;0x%04x\n",
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
    }
}

Sandbox* Sandbox::create()
{
    return new SandboxImpl();
}

void Sandbox::destroy(Sandbox& instance)
{
    delete &static_cast<SandboxImpl&>(instance);
}