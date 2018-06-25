#include "Graphics.h"
#include "GL/GLGraphics.h"

IGraphics* IGraphics::create(SDL_Window& window)
{
    return gl::Graphics::create(window);
}

void IGraphics::destroy(IGraphics* graphics)
{
    gl::Graphics::destroy(static_cast<gl::Graphics*>(graphics));
}