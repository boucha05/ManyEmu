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

    SDL_Window*             mWindow;
    SDL_Renderer*           mRenderer;
    SDL_Surface*            mScreen;
    SDL_Surface*            mSurface;
    bool                    mValid;
    NES::Rom*               mRom;
    NES::Context*           mContext;
};

Application::Application()
    : mWindow(nullptr)
    , mRenderer(nullptr)
    , mScreen(nullptr)
    , mSurface(nullptr)
    , mValid(false)
{
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
    mWindow = SDL_CreateWindow("ManyNES", 100, 100, 256, 240, SDL_WINDOW_SHOWN);
    if (!mWindow)
        return false;

    mRenderer = SDL_CreateRenderer(mWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!mRenderer)
        return false;

    mScreen = SDL_GetWindowSurface(mWindow);
    if (!mScreen)
        return false;

    mSurface = SDL_CreateRGBSurface(0, 256, 240, 8, 0, 0, 0, 0);
    if (!mSurface)
        return false;

    static const SDL_Color colors[64] =
    {
#include "Palette.h"
    };
    SDL_SetPaletteColors(mSurface->format->palette, colors, 0, 64);

#if 0
    if (!runTestRoms())
        return false;
#endif

    mRom = NES::Rom::load("ROMs\\exitbike.nes");
    //mRom = NES::Rom::load("ROMs\\mario.nes");
    //mRom = NES::Rom::load("ROMs\\nestest.nes");
    if (!mRom)
        return false;

    mContext = NES::Context::create(*mRom);
    if (!mContext)
        return false;

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

    if (mSurface)
    {
        SDL_FreeSurface(mSurface);
        mSurface = nullptr;
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
    static uint32_t frameSkip = 640;

    // Allow frames to be skipped if desired
    mContext->setRenderSurface(nullptr, 0);
    while (frameSkip)
    {
        --frameSkip;
        mContext->update();
    }

    if (SDL_LockSurface(mSurface) == 0)
    {
        if (++frameCount == frameTrigger)
            frameTrigger = frameTrigger;

        mContext->setRenderSurface(mSurface->pixels, mSurface->pitch);
        mContext->update();

        SDL_UnlockSurface(mSurface);
    }
}

void Application::render()
{
    SDL_RenderPresent(mRenderer);

    SDL_Rect rect = { 0, 0, 256, 240 };
    SDL_BlitSurface(mSurface, nullptr, mScreen, &rect);

    SDL_UpdateWindowSurface(mWindow);
}

int main()
{
    Application application;
    application.run();
    return 0;
}
