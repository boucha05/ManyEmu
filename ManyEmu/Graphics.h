#pragma once

#include "Core/Core.h"
#include <SDL.h>
#include <imgui.h>

struct ITexture {};

class IGraphics
{
public:
    virtual ~IGraphics() {}

    virtual void* getGLContext() = 0;
    virtual void getDrawableSize(uint32_t& width, uint32_t& height) = 0;
    virtual void swapBuffers() = 0;

    virtual ITexture* createTexture(uint32_t width, uint32_t height) = 0;
    virtual void destroyTexture(ITexture* texture) = 0;
    virtual void updateTexture(ITexture& texture, const void* data, size_t size) = 0;

    virtual bool imGuiRenderer_create() = 0;
    virtual void imGuiRenderer_destroy() = 0;
    virtual void imGuiRenderer_refreshFontTexture() = 0;
    virtual void imGuiRenderer_newFrame() = 0;
    virtual void imGuiRenderer_render() = 0;
    virtual ImTextureID imGuiRenderer_getTextureID(ITexture& texture) = 0;

    static IGraphics* create(SDL_Window& window);
    static void destroy(IGraphics* graphics);
};
