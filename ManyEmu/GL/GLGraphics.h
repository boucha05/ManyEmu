#pragma once

#include "../Graphics.h"

namespace gl
{
    class Graphics : public IGraphics
    {
    public:
        static Graphics* create();
        static void destroy(Graphics* graphics);
    };
}