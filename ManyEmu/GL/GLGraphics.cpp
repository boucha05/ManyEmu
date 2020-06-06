#define NOMINMAX

#include "GLGraphics.h"
#include <Core/Log.h>
#include <memory>

#include <SDL.h>
#include <SDL_syswm.h>
#include <GL/gl3w.h>
#include <GL/GLU.h>

#include "examples/imgui_impl_opengl3.h"

#if defined(EMU_DEBUG)
#define GL_VERIFY(expr)     EMU_VERIFY(((expr), ::_checkError(__FILE__, __LINE__)))
#else
#define GL_VERIFY(expr)     expr
#endif

namespace
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

    struct TextureImpl : public ITexture
    {
        bool initialize(uint32_t width, uint32_t height)
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

            GL_VERIFY(glGenTextures(1, &mTextureHandle));
            GL_VERIFY(glBindTexture(GL_TEXTURE_2D, mTextureHandle));
            GL_VERIFY(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
            GL_VERIFY(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
            GL_VERIFY(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
            GL_VERIFY(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
            GL_VERIFY(glTexStorage2D(GL_TEXTURE_2D, mipCount, GL_RGBA8, width, height));
            GL_VERIFY(glBindTexture(GL_TEXTURE_2D, 0));
            return true;
        }

        ~TextureImpl()
        {
            if (mTextureHandle)
            {
                glDeleteTextures(1, &mTextureHandle);
                mTextureHandle = 0;
            }
        }

        bool update(const void* data, size_t size)
        {
            auto imageSize = mWidth * mHeight * 4;
            EMU_VERIFY(size >= imageSize);

            GL_VERIFY(glBindTexture(GL_TEXTURE_2D, mTextureHandle));
            GL_VERIFY(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, data));
            GL_VERIFY(glGenerateMipmap(GL_TEXTURE_2D));
            GL_VERIFY(glBindTexture(GL_TEXTURE_2D, 0));
            return true;
        }

        uint32_t        mWidth = 0;
        uint32_t        mHeight = 0;
        GLuint          mTextureHandle;
    };

    struct ImGuiRenderer
    {
        bool initialize()
        {
            return ImGui_ImplOpenGL3_Init("#version 150");
        }

        ~ImGuiRenderer()
        {
            ImGui_ImplOpenGL3_Shutdown();
        }
    };

    class GraphicsImpl : public gl::Graphics
    {
    public:
        bool initialize(SDL_Window& window)
        {
            mWindow = &window;

            SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
            SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
            SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
            SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
            mGLContext = SDL_GL_CreateContext(mWindow);

            gl3wInit();
            return true;
        }

        ~GraphicsImpl()
        {
            if (mGLContext) SDL_GL_DeleteContext(mGLContext);
        }

        void* getGLContext() override
        {
            return mGLContext;
        }

        void getDrawableSize(uint32_t& width, uint32_t& height) override
        {
            int w, h;
            SDL_GL_GetDrawableSize(mWindow, &w, &h);
            width = w;
            height = h;
        }

        void swapBuffers() override
        {
            SDL_GL_SwapWindow(mWindow);
        }

        ITexture* createTexture(uint32_t width, uint32_t height) override
        {
            auto instance = std::make_unique<TextureImpl>();
            return instance->initialize(width, height) ? instance.release() : nullptr;
        }

        void destroyTexture(ITexture* texture) override
        {
            delete static_cast<TextureImpl*>(texture);
        }

        void updateTexture(ITexture& texture, const void* data, size_t size) override
        {
            static_cast<TextureImpl&>(texture).update(data, size);
        }

        bool imGuiRenderer_create() override
        {
            auto renderer = std::make_unique<ImGuiRenderer>();
            auto success = renderer->initialize();
            if (success)
                mImGuiRenderer.reset(renderer.release());
            return true;
        }

        void imGuiRenderer_destroy() override
        {
            mImGuiRenderer.reset();
        }

        void imGuiRenderer_refreshFontTexture() override
        {
            ImGui_ImplOpenGL3_DestroyFontsTexture();
            ImGui_ImplOpenGL3_CreateFontsTexture();
        }

        void imGuiRenderer_newFrame() override
        {
            ImGui_ImplOpenGL3_NewFrame();
        }

        void imGuiRenderer_render() override
        {
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }

        ImTextureID imGuiRenderer_getTextureID(ITexture& texture) override
        {
            return reinterpret_cast<ImTextureID>(static_cast<intptr_t>(static_cast<TextureImpl&>(texture).mTextureHandle));
        }

    private:
        SDL_Window*                     mWindow = nullptr;
        SDL_GLContext                   mGLContext = nullptr;
        std::unique_ptr<ImGuiRenderer>  mImGuiRenderer;
    };
}

namespace gl
{
    Graphics* Graphics::create(SDL_Window& window)
    {
        auto instance = std::make_unique<GraphicsImpl>();
        return instance->initialize(window) ? instance.release() : nullptr;
    }

    void Graphics::destroy(Graphics* graphics)
    {
        delete static_cast<GraphicsImpl*>(graphics);
    }
}
