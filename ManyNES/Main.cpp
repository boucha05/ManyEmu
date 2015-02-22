#define SDL_MAIN_HANDLED
#define GLEW_STATIC

#include <SDL.h>
#include <gl/glew.h>
#include <assert.h>
#include <string>
#include <vector>
#include "InputController.h"
#include "Path.h"
#include "Serialization.h"
#include "Stream.h"
#include "Tests.h"
#include "nes.h"

namespace
{
    bool initSDL()
    {
        SDL_Init(SDL_INIT_GAMECONTROLLER);
        return true;
    }

    static bool sdlInitialized = initSDL();
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
        bool            playback;
        bool            saveAudio;

        Config()
            : frameSkip(0)
            , playback(false)
            , saveAudio(false)
        {
        }
    };

    Application();
    ~Application();
    void run(const Config& config);
    void terminate();

private:
    bool create();
    void destroy();
    bool loadGameData();
    bool saveGameData();
    void handleEvents();
    void update();
    void render();

    static const uint32_t BUFFERING = 2;

    Config                      mConfig;
    std::string                 mGameDataPath;
    SDL_Window*                 mWindow;
    SDL_Renderer*               mRenderer;
    SDL_Surface*                mScreen;
    SDL_Texture*                mTexture[BUFFERING];
    bool                        mValid;
    bool                        mFirst;
    uint32_t                    mBufferIndex;
    uint32_t                    mFrameIndex;
    uint32_t                    mBufferCount;
    std::vector<int16_t>        mSoundBuffer;
    NES::FileStream*            mSoundFile;
    NES::BinaryWriter*          mSoundWriter;
    NES::Rom*                   mRom;
    NES::Context*               mContext;
    NES::GroupController*       mPlayer1Controllers;
    NES::KeyboardController*    mKeyboard;
    NES::GameController*        mGameController;
    NES::InputRecorder*         mRecorder;
    NES::InputPlayback*         mPlayback;
    NES::InputController*       mPlayer1;
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
    , mSoundFile(nullptr)
    , mSoundWriter(nullptr)
    , mRom(nullptr)
    , mContext(nullptr)
    , mPlayer1Controllers(nullptr)
    , mKeyboard(nullptr)
    , mGameController(nullptr)
    , mRecorder(nullptr)
    , mPlayback(nullptr)
    , mPlayer1(nullptr)
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
    if (!romName.empty())
        mGameDataPath = Path::join(mConfig.saveFolder, romName) + ".dat";

    mWindow = SDL_CreateWindow("ManyNES", 100, 100, 256, 240, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
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

    loadGameData();

    mPlayer1Controllers = NES::GroupController::create();
    if (!mPlayer1Controllers)
        return false;
    mPlayer1 = mPlayer1Controllers;

    mKeyboard = NES::KeyboardController::create();
    if (!mKeyboard)
        return false;
    mKeyboard->addKey(SDL_SCANCODE_UP, NES::Context::ButtonUp);
    mKeyboard->addKey(SDL_SCANCODE_DOWN, NES::Context::ButtonDown);
    mKeyboard->addKey(SDL_SCANCODE_LEFT, NES::Context::ButtonLeft);
    mKeyboard->addKey(SDL_SCANCODE_RIGHT, NES::Context::ButtonRight);
    mKeyboard->addKey(SDL_SCANCODE_KP_8, NES::Context::ButtonUp);
    mKeyboard->addKey(SDL_SCANCODE_KP_2, NES::Context::ButtonDown);
    mKeyboard->addKey(SDL_SCANCODE_KP_4, NES::Context::ButtonLeft);
    mKeyboard->addKey(SDL_SCANCODE_KP_6, NES::Context::ButtonRight);
    mKeyboard->addKey(SDL_SCANCODE_X, NES::Context::ButtonB);
    mKeyboard->addKey(SDL_SCANCODE_Z, NES::Context::ButtonA);
    mKeyboard->addKey(SDL_SCANCODE_A, NES::Context::ButtonSelect);
    mKeyboard->addKey(SDL_SCANCODE_S, NES::Context::ButtonStart);
    mPlayer1Controllers->addController(*mKeyboard);

    mGameController = NES::GameController::create(0);
    if (mGameController)
    {
        mGameController->addButton(SDL_CONTROLLER_BUTTON_DPAD_UP, NES::Context::ButtonUp);
        mGameController->addButton(SDL_CONTROLLER_BUTTON_DPAD_DOWN, NES::Context::ButtonDown);
        mGameController->addButton(SDL_CONTROLLER_BUTTON_DPAD_LEFT, NES::Context::ButtonLeft);
        mGameController->addButton(SDL_CONTROLLER_BUTTON_DPAD_RIGHT, NES::Context::ButtonRight);
        mGameController->addAxis(SDL_CONTROLLER_AXIS_LEFTX, NES::Context::ButtonLeft, NES::Context::ButtonRight);
        mGameController->addAxis(SDL_CONTROLLER_AXIS_LEFTY, NES::Context::ButtonUp, NES::Context::ButtonDown);
        mGameController->addButton(SDL_CONTROLLER_BUTTON_BACK, NES::Context::ButtonSelect);
        mGameController->addButton(SDL_CONTROLLER_BUTTON_START, NES::Context::ButtonStart);
        mGameController->addButton(SDL_CONTROLLER_BUTTON_A, NES::Context::ButtonA);
        mGameController->addButton(SDL_CONTROLLER_BUTTON_B, NES::Context::ButtonB);
        mGameController->addButton(SDL_CONTROLLER_BUTTON_X, NES::Context::ButtonB);
        mGameController->addButton(SDL_CONTROLLER_BUTTON_Y, NES::Context::ButtonA);
        mPlayer1Controllers->addController(*mGameController);
    }

    if (!mConfig.recorded.empty())
    {
        if (mConfig.playback)
        {
            mPlayback = NES::InputPlayback::create(*mPlayer1);
            if (!mPlayback)
                return false;
            if (!mPlayback->load(mConfig.recorded.c_str()))
                return false;
            mPlayer1 = mPlayback;
        }
        else
        {
            mRecorder = NES::InputRecorder::create(*mPlayer1);
            if (!mRecorder)
                return false;
            mPlayer1 = mRecorder;
        }
    }

    mSoundBuffer.clear();
    mSoundBuffer.resize(44100 / 60, 0);
    if (mConfig.saveAudio)
    {
        std::string path = Path::join(mConfig.saveFolder, romName) + ".snd";
        mSoundFile = new NES::FileStream(path.c_str(), "wb");
        if (!mSoundFile)
            return false;

        mSoundWriter = new NES::BinaryWriter(*mSoundFile);
        if (!mSoundWriter)
            return false;
    }

    mFirst = true;

    return true;
}

void Application::destroy()
{
    mPlayer1 = nullptr;

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

    if (mRecorder)
    {
        if (!mConfig.recorded.empty())
            mRecorder->save(mConfig.recorded.c_str());
        mRecorder->dispose();
        mRecorder = nullptr;
    }

    if (mGameController)
    {
        mGameController->dispose();
        mGameController = nullptr;
    }

    if (mKeyboard)
    {
        mKeyboard->dispose();
        mKeyboard = nullptr;
    }

    if (mPlayer1Controllers)
    {
        mPlayer1Controllers->dispose();
        mPlayer1Controllers = nullptr;
    }

    if (mContext)
    {
        saveGameData();

        mContext->dispose();
        mContext = nullptr;
    }

    if (mRom)
    {
        mRom->dispose();
        mRom = nullptr;
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

    for (uint32_t n = 0; n < BUFFERING; ++n)
    {
        if (mTexture[n])
        {
            SDL_DestroyTexture(mTexture[n]);
            mTexture[n] = nullptr;
        }
    }
}

bool Application::loadGameData()
{
    if (mGameDataPath.empty())
        return false;

    NES::FileStream stream(mGameDataPath.c_str(), "rb");
    if (!stream.valid())
        return false;

    NES::BinaryReader reader(stream);
    mContext->serializeGameData(reader);
    return true;
}

bool Application::saveGameData()
{
    if (mGameDataPath.empty())
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

void Application::handleEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
            terminate();

        if (event.type == SDL_KEYDOWN)
        {
            switch (event.key.keysym.sym)
            {
            case SDLK_ESCAPE:
                terminate();
                break;
            }
        }
    }
}

void Application::update()
{
    static uint32_t frameTrigger = 640;

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

    if (++mFrameIndex == frameTrigger)
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
    mContext->update();

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

int main()
{
    Application::Config config;
    config.saveFolder = "Save";
    config.saveAudio = true;
    //config.rom = "C:\\Emu\\NES\\roms\\zelda2.nes";
    //config.rom = "C:\\Emu\\NES\\roms\\cvania2.nes";
    //config.rom = "C:\\Emu\\NES\\roms\\kidicar.nes";
    config.rom = "ROMs\\exitbike.nes";
    //config.rom = "ROMs\\megaman2.nes";
    //config.rom = "ROMs\\mario.nes";
    //config.rom = "ROMs\\nestest.nes";
    //config.recorded = "ROMs\\recorded.dat";
    //config.playback = true;
    //config.frameSkip = 512;

    Application application;
    application.run(config);
    return 0;
}
