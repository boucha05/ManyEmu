#include "Graphics.h"
#include "GL/GLGraphics.h"

IGraphics* IGraphics::create()
{
    return gl::Graphics::create();
}

void IGraphics::destroy(IGraphics* graphics)
{
    gl::Graphics::destroy(static_cast<gl::Graphics*>(graphics));
}