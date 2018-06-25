#pragma once

#include "../Graphics.h"

namespace gl
{
    class Graphics : public IGraphics
    {
    public:
        static Graphics* create(SDL_Window& window);
        static void destroy(Graphics* graphics);
    };
}