#define NOMINMAX
#include <GL/gl3w.h>
#include <GL/GLU.h>

#include "Texture.h"
#include <Core/Core.h>
#include <Core/Log.h>

#define GL_VERIFY(expr)     EMU_VERIFY(((expr), gl::_checkError(__FILE__, __LINE__)))

namespace gl
{
    bool _checkError(const char* file, int line)
    {
        auto value = glGetError();
        auto success = value == 0;
        if (!success)
        {
            auto message = gluErrorString(value);
            emu::Log::printf(emu::Log::Type::Error, "%s(%d): GL check: %s\n", file, line, message);
        }
        return success;
    }

    ImTextureID getPublicTextureID(GLuint nativeID)
    {
        return reinterpret_cast<ImTextureID>(static_cast<intptr_t>(nativeID));
    }

    GLuint getNativeTextureID(ImTextureID publicID)
    {
        return static_cast<GLuint>(reinterpret_cast<intptr_t>(publicID));
    }
}

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
    uint32_t mipSize = std::max(width, height);
    uint32_t mipCount = 0;
    while (mipSize)
    {
        mipSize >>= 1;
        ++mipCount;
    }

    GLuint nativeTexture;
    GL_VERIFY(glGenTextures(1, &nativeTexture));
    mTexture = gl::getPublicTextureID(nativeTexture);
    GL_VERIFY(glBindTexture(GL_TEXTURE_2D, nativeTexture));
    GL_VERIFY(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
    GL_VERIFY(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL_VERIFY(glTexStorage2D(GL_TEXTURE_2D, mipCount, GL_RGBA8, width, height));
    GL_VERIFY(glBindTexture(GL_TEXTURE_2D, 0));
    return true;
}

void Texture::destroy()
{
    if (mTexture)
    {
        GLuint nativeTextureID = gl::getNativeTextureID(mTexture);
        glDeleteTextures(1, &nativeTextureID);
        mTexture = 0;
    }
}

bool Texture::update(const void* data, size_t size)
{
    auto imageSize = mWidth * mHeight * 4;
    EMU_VERIFY(size >= imageSize);

    GL_VERIFY(glBindTexture(GL_TEXTURE_2D, gl::getNativeTextureID(mTexture)));
    GL_VERIFY(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, data));
    GL_VERIFY(glGenerateMipmap(GL_TEXTURE_2D));
    GL_VERIFY(glBindTexture(GL_TEXTURE_2D, 0));
    return true;
}
