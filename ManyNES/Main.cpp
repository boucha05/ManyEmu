#define SDL_MAIN_HANDLED
#define GLEW_STATIC

#include <SDL.h>
#include <gl/glew.h>
#include <assert.h>
#include <string>
#include <vector>
#include "Tests.h"
#include "nes.h"

class Application
{
public:
    Application();
    ~Application();
    void run();
    void terminate();

private:
    bool create();
    void destroy();
    void handleEvents();
    void update();
    void render();

    static const uint32_t BUFFERING = 2;

    SDL_Window*             mWindow;
    SDL_Renderer*           mRenderer;
    SDL_Surface*            mScreen;
    SDL_Texture*            mTexture[BUFFERING];
    bool                    mValid;
    bool                    mFirst;
    uint32_t                mFrame;
    NES::Rom*               mRom;
    NES::Context*           mContext;
};

Application::Application()
    : mWindow(nullptr)
    , mRenderer(nullptr)
    , mScreen(nullptr)
    , mValid(false)
    , mFirst(true)
    , mFrame(0)
{
    for (uint32_t n = 0; n < BUFFERING; ++n)
        mTexture[n] = nullptr;
}

Application::~Application()
{
    destroy();
}

void Application::run()
{
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

    //mRom = NES::Rom::load("ROMs\\exitbike.nes");
    mRom = NES::Rom::load("ROMs\\mario.nes");
    //mRom = NES::Rom::load("ROMs\\nestest.nes");
    if (!mRom)
        return false;

    mContext = NES::Context::create(*mRom);
    if (!mContext)
        return false;

    mFirst = true;

    return true;
}

void Application::destroy()
{
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
    Application application;
    application.run();
    return 0;
}
