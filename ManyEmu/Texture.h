#pragma once

#include "Application.h"
#include <gl/glew.h>

class Texture
{
public:
    Texture();
    ~Texture();
    bool create(uint32_t width, uint32_t height);
    void destroy();
    bool update(const void* data, size_t size);

    uint32_t getWidth() const
    {
        return mWidth;
    }

    uint32_t getHeight() const
    {
        return mHeight;
    }

    ImTextureID getImTextureID() const
    {
        return reinterpret_cast<ImTextureID>(static_cast<intptr_t>(mTexture));
    }

private:
    void initialize();

    uint32_t    mWidth;
    uint32_t    mHeight;
    GLuint      mTexture;
};
