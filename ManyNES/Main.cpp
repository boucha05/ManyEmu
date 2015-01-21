#define SDL_MAIN_HANDLED
#define GLEW_STATIC

#include <SDL.h>
#include <gl/glew.h>
#include <assert.h>
#include <vector>
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
    SDL_Surface*            mScreen;
    SDL_Surface*            mSurface;
    bool                    mValid;
    NES::Rom*               mRom;
    NES::Context*           mContext;
};

Application::Application()
    : mWindow(nullptr)
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
    mWindow = SDL_CreateWindow("ManyNES", 100,
        100, 512, 480, SDL_WINDOW_SHOWN);
    if (!mWindow)
        return false;

    mScreen = SDL_GetWindowSurface(mWindow);
    if (!mScreen)
        return false;

    mSurface = SDL_CreateRGBSurface(0, 256, 256, 8, 0, 0, 0, 0);
    if (!mSurface)
        return false;

    static const SDL_Color colors[64] =
    {
#include "Palette.h"
    };
    SDL_SetPaletteColors(mSurface->format->palette, colors, 0, 64);

    mRom = NES::Rom::load("mario.nes");
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
    if (SDL_LockSurface(mSurface) == 0)
    {
        uint8_t* buffer = static_cast<uint8_t*>(mSurface->pixels);
        for (uint32_t y = 0; y < 256; ++y)
        {
            for (uint32_t x = 0; x < 256; ++x)
            {
                *buffer++ = static_cast<uint8_t>(x * y) & 63;
            }
        }

        mContext->update(mSurface->pixels, mSurface->pitch);

        SDL_UnlockSurface(mSurface);
    }
}

void Application::render()
{
    SDL_Rect rect = { 0, 0, 256, 256 };
    SDL_BlitSurface(mSurface, nullptr, mScreen, &rect);

    SDL_UpdateWindowSurface(mWindow);
}

int main()
{
    Application application;
    application.run();
    return 0;
}
