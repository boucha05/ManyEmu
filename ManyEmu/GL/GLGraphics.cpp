#include "GLGraphics.h"
#include <Core/Log.h>
#include <memory>

#include <SDL.h>
#include <SDL_syswm.h>
#include <GL/gl3w.h>
#include <GL/GLU.h>

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

    struct ImGuiRenderer
    {
        GLuint          mFontTexture = 0;
        GLuint          mShaderHandle = 0;
        GLuint          mVertHandle = 0;
        GLuint          mFragHandle = 0;
        GLuint          mAttribLocationTex = 0;
        GLuint          mAttribLocationProjMtx = 0;
        GLuint          mAttribLocationPosition = 0;
        GLuint          mAttribLocationUV = 0;
        GLuint          mAttribLocationColor = 0;
        GLuint          mVboHandle = 0;
        GLuint          mVaoHandle = 0;
        GLuint          mElementsHandle = 0;
        int32_t         mFrameBufferSizeX = 0;
        int32_t         mFrameBufferSizeY = 0;
        GLint           mLastProgram;
        GLint           mLastTexture;
        GLint           mLastActiveTexture;
        GLint           mLastArrayBuffer;
        GLint           mLastElementArrayBuffer;
        GLint           mLastVertexArray;
        GLint           mLastBlendSrc;
        GLint           mLastBlendDst;
        GLint           mLastBlendEquationRgb;
        GLint           mLastBlendEquationAlpha;
        GLint           mLastViewport[4];
        GLint           mLastScissorBox[4];
        GLboolean       mLastEnableBlend;
        GLboolean       mLastEnableCullFace;
        GLboolean       mLastEnableDepthTest;
        GLboolean       mLastEnableScissorTest;

        bool initialize()
        {
            // Backup GL state
            GL_VERIFY(glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &mLastArrayBuffer));
            GL_VERIFY(glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &mLastVertexArray));

            const GLchar *vertex_shader =
                "#version 330\n"
                "uniform mat4 ProjMtx;\n"
                "in vec2 Position;\n"
                "in vec2 UV;\n"
                "in vec4 Color;\n"
                "out vec2 Frag_UV;\n"
                "out vec4 Frag_Color;\n"
                "void main()\n"
                "{\n"
                "   Frag_UV = UV;\n"
                "   Frag_Color = Color;\n"
                "   gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
                "}\n";

            const GLchar* fragment_shader =
                "#version 330\n"
                "uniform sampler2D Texture;\n"
                "in vec2 Frag_UV;\n"
                "in vec4 Frag_Color;\n"
                "out vec4 Out_Color;\n"
                "void main()\n"
                "{\n"
                "   Out_Color = Frag_Color * texture( Texture, Frag_UV.st);\n"
                "}\n";

            mShaderHandle = glCreateProgram();
            mVertHandle = glCreateShader(GL_VERTEX_SHADER);
            mFragHandle = glCreateShader(GL_FRAGMENT_SHADER);
            GL_VERIFY(glShaderSource(mVertHandle, 1, &vertex_shader, 0));
            GL_VERIFY(glShaderSource(mFragHandle, 1, &fragment_shader, 0));
            GL_VERIFY(glCompileShader(mVertHandle));
            GL_VERIFY(glCompileShader(mFragHandle));
            GL_VERIFY(glAttachShader(mShaderHandle, mVertHandle));
            GL_VERIFY(glAttachShader(mShaderHandle, mFragHandle));
            GL_VERIFY(glLinkProgram(mShaderHandle));

            mAttribLocationTex = glGetUniformLocation(mShaderHandle, "Texture");
            mAttribLocationProjMtx = glGetUniformLocation(mShaderHandle, "ProjMtx");
            mAttribLocationPosition = glGetAttribLocation(mShaderHandle, "Position");
            mAttribLocationUV = glGetAttribLocation(mShaderHandle, "UV");
            mAttribLocationColor = glGetAttribLocation(mShaderHandle, "Color");

            GL_VERIFY(glGenBuffers(1, &mVboHandle));
            GL_VERIFY(glGenBuffers(1, &mElementsHandle));

            GL_VERIFY(glGenVertexArrays(1, &mVaoHandle));
            GL_VERIFY(glBindVertexArray(mVaoHandle));
            GL_VERIFY(glBindBuffer(GL_ARRAY_BUFFER, mVboHandle));
            GL_VERIFY(glEnableVertexAttribArray(mAttribLocationPosition));
            GL_VERIFY(glEnableVertexAttribArray(mAttribLocationUV));
            GL_VERIFY(glEnableVertexAttribArray(mAttribLocationColor));

#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
            GL_VERIFY(glVertexAttribPointer(mAttribLocationPosition, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, pos)));
            GL_VERIFY(glVertexAttribPointer(mAttribLocationUV, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, uv)));
            GL_VERIFY(glVertexAttribPointer(mAttribLocationColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, col)));
#undef OFFSETOF

            // Restore modified GL state
            GL_VERIFY(glBindBuffer(GL_ARRAY_BUFFER, mLastArrayBuffer));
            GL_VERIFY(glBindVertexArray(mLastVertexArray));

            return true;
        }

        ~ImGuiRenderer()
        {
            EMU_CHECK(destroyResources());
        }

        bool destroyResources()
        {
            if (mVaoHandle) GL_VERIFY(glDeleteVertexArrays(1, &mVaoHandle));
            if (mVboHandle) GL_VERIFY(glDeleteBuffers(1, &mVboHandle));
            if (mElementsHandle) GL_VERIFY(glDeleteBuffers(1, &mElementsHandle));
            mVaoHandle = mVboHandle = mElementsHandle = 0;

            if (mShaderHandle && mVertHandle) GL_VERIFY(glDetachShader(mShaderHandle, mVertHandle));
            if (mVertHandle) GL_VERIFY(glDeleteShader(mVertHandle));
            mVertHandle = 0;

            if (mShaderHandle && mFragHandle) GL_VERIFY(glDetachShader(mShaderHandle, mFragHandle));
            if (mFragHandle) GL_VERIFY(glDeleteShader(mFragHandle));
            mFragHandle = 0;

            if (mShaderHandle) GL_VERIFY(glDeleteProgram(mShaderHandle));
            mShaderHandle = 0;

            EMU_ASSERT(destroyFontTexture());
            return true;
        }

        bool createFontTexture(const ImGuiRenderer_SetFontTexture_Params& params)
        {
            // Upload texture to graphics system
            GL_VERIFY(glGetIntegerv(GL_TEXTURE_BINDING_2D, &mLastTexture));
            GL_VERIFY(glGenTextures(1, &mFontTexture));
            GL_VERIFY(glBindTexture(GL_TEXTURE_2D, mFontTexture));
            GL_VERIFY(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
            GL_VERIFY(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
            GL_VERIFY(glPixelStorei(GL_UNPACK_ROW_LENGTH, 0));
            GL_VERIFY(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, params.width, params.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, params.pixels));

            // Restore state
            GL_VERIFY(glBindTexture(GL_TEXTURE_2D, mLastTexture));

            return true;
        }

        bool destroyFontTexture()
        {
            if (mFontTexture)
            {
                GL_VERIFY(glDeleteTextures(1, &mFontTexture));
                mFontTexture = 0;
            }
            return true;
        }

        bool beginDraw(const ImGuiRenderer_BeginDraw_Params& params)
        {
            // Backup GL state
            GL_VERIFY(glGetIntegerv(GL_CURRENT_PROGRAM, &mLastProgram));
            GL_VERIFY(glGetIntegerv(GL_TEXTURE_BINDING_2D, &mLastTexture));
            GL_VERIFY(glGetIntegerv(GL_ACTIVE_TEXTURE, &mLastActiveTexture));
            GL_VERIFY(glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &mLastArrayBuffer));
            GL_VERIFY(glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &mLastElementArrayBuffer));
            GL_VERIFY(glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &mLastVertexArray));
            GL_VERIFY(glGetIntegerv(GL_BLEND_SRC, &mLastBlendSrc));
            GL_VERIFY(glGetIntegerv(GL_BLEND_DST, &mLastBlendDst));
            GL_VERIFY(glGetIntegerv(GL_BLEND_EQUATION_RGB, &mLastBlendEquationRgb));
            GL_VERIFY(glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &mLastBlendEquationAlpha));
            GL_VERIFY(glGetIntegerv(GL_VIEWPORT, mLastViewport));
            GL_VERIFY(glGetIntegerv(GL_SCISSOR_BOX, mLastScissorBox));
            GL_VERIFY(glIsEnabled(GL_BLEND));
            GL_VERIFY(glIsEnabled(GL_CULL_FACE));
            GL_VERIFY(glIsEnabled(GL_DEPTH_TEST));
            GL_VERIFY(glIsEnabled(GL_SCISSOR_TEST));

            // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
            GL_VERIFY(glEnable(GL_BLEND));
            GL_VERIFY(glBlendEquation(GL_FUNC_ADD));
            GL_VERIFY(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
            GL_VERIFY(glDisable(GL_CULL_FACE));
            GL_VERIFY(glDisable(GL_DEPTH_TEST));
            GL_VERIFY(glEnable(GL_SCISSOR_TEST));
            GL_VERIFY(glActiveTexture(GL_TEXTURE0));

            // Setup orthographic projection matrix
            mFrameBufferSizeX = params.frameBufferSizeX;
            mFrameBufferSizeY = params.frameBufferSizeY;
            GL_VERIFY(glViewport(0, 0, (GLsizei)mFrameBufferSizeX, (GLsizei)mFrameBufferSizeY));
            const float ortho_projection[4][4] =
            {
                { 2.0f / mFrameBufferSizeX, 0.0f,                      0.0f, 0.0f },
                { 0.0f,                     2.0f / -mFrameBufferSizeY, 0.0f, 0.0f },
                { 0.0f,                     0.0f,                     -1.0f, 0.0f },
                {-1.0f,                     1.0f,                      0.0f, 1.0f },
            };
            GL_VERIFY(glUseProgram(mShaderHandle));
            GL_VERIFY(glUniform1i(mAttribLocationTex, 0));
            GL_VERIFY(glUniformMatrix4fv(mAttribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]));
            GL_VERIFY(glBindVertexArray(mVaoHandle));

            GL_VERIFY(glBindBuffer(GL_ARRAY_BUFFER, mVboHandle));
            GL_VERIFY(glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)params.vertexCount * sizeof(ImDrawVert), (GLvoid*)params.vertexData, GL_STREAM_DRAW));

            GL_VERIFY(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mElementsHandle));
            GL_VERIFY(glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)params.indexCount * sizeof(ImDrawIdx), (GLvoid*)params.indexData, GL_STREAM_DRAW));
            return true;
        }

        bool drawPrimitives(const ImGuiRenderer_DrawPrimitives_Params& params)
        {
            GL_VERIFY(glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)params.textureId));
            GL_VERIFY(glScissor((int)params.clipX1, (int)(mFrameBufferSizeY - params.clipY2), (int)(params.clipX2 - params.clipX1), (int)(params.clipY2 - params.clipY1)));
            GL_VERIFY(glDrawElementsBaseVertex(GL_TRIANGLES, (GLsizei)params.elementCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, (const GLvoid*)(params.indexOffset * sizeof(ImDrawIdx)), params.vertexOffset));
            return true;
        }

        bool endDraw()
        {
            // Restore modified GL state
            GL_VERIFY(glUseProgram(mLastProgram));
            GL_VERIFY(glActiveTexture(mLastActiveTexture));
            GL_VERIFY(glBindTexture(GL_TEXTURE_2D, mLastTexture));
            GL_VERIFY(glBindVertexArray(mLastVertexArray));
            GL_VERIFY(glBindBuffer(GL_ARRAY_BUFFER, mLastArrayBuffer));
            GL_VERIFY(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mLastElementArrayBuffer));
            GL_VERIFY(glBlendEquationSeparate(mLastBlendEquationRgb, mLastBlendEquationAlpha));
            GL_VERIFY(glBlendFunc(mLastBlendSrc, mLastBlendDst));
            if (mLastEnableBlend) GL_VERIFY(glEnable(GL_BLEND)); else GL_VERIFY(glDisable(GL_BLEND));
            if (mLastEnableCullFace) GL_VERIFY(glEnable(GL_CULL_FACE)); else GL_VERIFY(glDisable(GL_CULL_FACE));
            if (mLastEnableDepthTest) GL_VERIFY(glEnable(GL_DEPTH_TEST)); else GL_VERIFY(glDisable(GL_DEPTH_TEST));
            if (mLastEnableScissorTest) GL_VERIFY(glEnable(GL_SCISSOR_TEST)); else GL_VERIFY(glDisable(GL_SCISSOR_TEST));
            GL_VERIFY(glViewport(mLastViewport[0], mLastViewport[1], (GLsizei)mLastViewport[2], (GLsizei)mLastViewport[3]));
            GL_VERIFY(glScissor(mLastScissorBox[0], mLastScissorBox[1], (GLsizei)mLastScissorBox[2], (GLsizei)mLastScissorBox[3]));
            return true;
        }
    };

    class GraphicsImpl : public gl::Graphics
    {
    public:
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

        bool imGuiRenderer_setFontTexture(const ImGuiRenderer_SetFontTexture_Params& params) override
        {
            EMU_VERIFY(mImGuiRenderer);
            EMU_VERIFY(mImGuiRenderer->destroyFontTexture());
            EMU_VERIFY(mImGuiRenderer->createFontTexture(params));
            return true;
        }

        void* imGuiRenderer_getFontTexture() override
        {
            return mImGuiRenderer ? (void *)(intptr_t)mImGuiRenderer->mFontTexture : nullptr;
        }

        bool imGuiRenderer_beginDraw(const ImGuiRenderer_BeginDraw_Params& params) override
        {
            EMU_UNUSED(mImGuiRenderer);
            EMU_VERIFY(mImGuiRenderer->beginDraw(params));
            return true;
        }

        void imGuiRenderer_endDraw() override
        {
            if (mImGuiRenderer)
                EMU_CHECK(mImGuiRenderer->endDraw());
        }

        void imGuiRenderer_drawPrimitives(const ImGuiRenderer_DrawPrimitives_Params& params) override
        {
            if (mImGuiRenderer)
                EMU_CHECK(mImGuiRenderer->drawPrimitives(params));
        }

    private:
        std::unique_ptr<ImGuiRenderer>  mImGuiRenderer;
    };
}

namespace gl
{
    Graphics* Graphics::create()
    {
        return new GraphicsImpl();
    }

    void Graphics::destroy(Graphics* graphics)
    {
        delete static_cast<GraphicsImpl*>(graphics);
    }
}
