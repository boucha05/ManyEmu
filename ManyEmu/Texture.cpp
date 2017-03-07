#include <Core/Core.h>
#include "Texture.h"

Texture::Texture()
{
    initialize();
}

Texture::~Texture()
{
    destroy();
}

void Texture::initialize()
{
    mWidth = 0;
    mHeight = 0;
    mTexture = 0;
}

bool Texture::create(uint32_t width, uint32_t height)
{
    EMU_VERIFY(width > 0);
    EMU_VERIFY(height > 0);
    mWidth = width;
    mHeight = height;
    glGenTextures(1, &mTexture);
    glBindTexture(GL_TEXTURE_2D, mTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    return true;
}

void Texture::destroy()
{
    if (mTexture)
    {
        glDeleteTextures(1, &mTexture);
        mTexture = 0;
    }
}

bool Texture::update(const void* data, size_t size)
{
    auto imageSize = mWidth * mHeight * 4;
    EMU_VERIFY(size >= imageSize);

    glBindTexture(GL_TEXTURE_2D, mTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, data);
    return true;
}
