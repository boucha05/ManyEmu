#define SDL_MAIN_HANDLED
#define GLEW_STATIC

#include <SDL.h>
#include <gl/glew.h>
#include <assert.h>
#include <string>
#include <vector>
#include "InputController.h"
#include "Tests.h"
#include "nes.h"

class Application
{
public:
    struct Config
    {
        std::string     rom;
        std::string     recorded;
        bool            playback;

        Config()
            : playback(false)
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
    void handleEvents();
    void update();
    void render();

    static const uint32_t BUFFERING = 2;

    Config                      mConfig;
    SDL_Window*                 mWindow;
    SDL_Renderer*               mRenderer;
    SDL_Surface*                mScreen;
    SDL_Texture*                mTexture[BUFFERING];
    bool                        mValid;
    bool                        mFirst;
    uint32_t                    mFrame;
    NES::Rom*                   mRom;
    NES::Context*               mContext;
    NES::KeyboardController*    mKeyboard;
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
    , mFrame(0)
    , mRom(nullptr)
    , mContext(nullptr)
    , mKeyboard(nullptr)
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

    mKeyboard = NES::KeyboardController::create();
    if (!mKeyboard)
        return false;
    mKeyboard->addKey(SDL_SCANCODE_UP, NES::Context::ButtonUp);
    mKeyboard->addKey(SDL_SCANCODE_DOWN, NES::Context::ButtonDown);
    mKeyboard->addKey(SDL_SCANCODE_LEFT, NES::Context::ButtonLeft);
    mKeyboard->addKey(SDL_SCANCODE_RIGHT, NES::Context::ButtonRight);
    mKeyboard->addKey(SDL_SCANCODE_X, NES::Context::ButtonB);
    mKeyboard->addKey(SDL_SCANCODE_Z, NES::Context::ButtonA);
    mKeyboard->addKey(SDL_SCANCODE_A, NES::Context::ButtonSelect);
    mKeyboard->addKey(SDL_SCANCODE_S, NES::Context::ButtonStart);
    mPlayer1 = mKeyboard;

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

    mFirst = true;

    return true;
}

void Application::destroy()
{
    mPlayer1 = nullptr;

    if (mRecorder)
    {
        if (!mConfig.recorded.empty())
            mRecorder->save(mConfig.recorded.c_str());
        mRecorder->dispose();
        mRecorder = nullptr;
    }

    if (mKeyboard)
    {
        mKeyboard->dispose();
        mKeyboard = nullptr;
    }

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
    static uint32_t frameCount = 0;
    static uint32_t frameTrigger = 640;
    static uint32_t frameSkip = 0;//640;

    // Allow frames to be skipped if desired
    mContext->setRenderSurface(nullptr, 0);
    while (frameSkip)
    {
        --frameSkip;
        mContext->update();
    }

    if (mPlayer1)
        mContext->setController(0, mPlayer1->readInput());

    void* pixels = nullptr;
    int pitch = 0;
    if (!SDL_LockTexture(mTexture[mFrame], nullptr, &pixels, &pitch))
    {
        if (++frameCount == frameTrigger)
            frameTrigger = frameTrigger;

        mContext->setRenderSurface(pixels, pitch);
        mContext->update();

        SDL_UnlockTexture(mTexture[mFrame]);
    }
}

void Application::render()
{
    if (++mFrame >= BUFFERING)
        mFrame = 0;
    if (mFirst)
        mFirst = false;
    else
        SDL_RenderCopy(mRenderer, mTexture[mFrame], nullptr, nullptr);
    SDL_RenderPresent(mRenderer);
}

int main()
{
    Application::Config config;
    //config.rom = "ROMs\\exitbike.nes";
    config.rom = "ROMs\\megaman2.nes";
    //config.rom = "ROMs\\mario.nes";
    //config.rom = "ROMs\\nestest.nes";
    //config.recorded = "ROMs\\recorded.dat";
    //config.playback = true;

    Application application;
    application.run(config);
    return 0;
}
