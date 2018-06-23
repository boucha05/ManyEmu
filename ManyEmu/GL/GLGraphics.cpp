#include "GLGraphics.h"
#include <memory>

// SDL,GL3W
#include <SDL.h>
#include <SDL_syswm.h>
#include <GL/gl3w.h>    // This example is using gl3w to access OpenGL functions (because it is small). You may use glew/glad/glLoadGen/etc. whatever already works for you.

namespace
{
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
            glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &mLastArrayBuffer);
            glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &mLastVertexArray);

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
            glShaderSource(mVertHandle, 1, &vertex_shader, 0);
            glShaderSource(mFragHandle, 1, &fragment_shader, 0);
            glCompileShader(mVertHandle);
            glCompileShader(mFragHandle);
            glAttachShader(mShaderHandle, mVertHandle);
            glAttachShader(mShaderHandle, mFragHandle);
            glLinkProgram(mShaderHandle);

            mAttribLocationTex = glGetUniformLocation(mShaderHandle, "Texture");
            mAttribLocationProjMtx = glGetUniformLocation(mShaderHandle, "ProjMtx");
            mAttribLocationPosition = glGetAttribLocation(mShaderHandle, "Position");
            mAttribLocationUV = glGetAttribLocation(mShaderHandle, "UV");
            mAttribLocationColor = glGetAttribLocation(mShaderHandle, "Color");

            glGenBuffers(1, &mVboHandle);
            glGenBuffers(1, &mElementsHandle);

            glGenVertexArrays(1, &mVaoHandle);
            glBindVertexArray(mVaoHandle);
            glBindBuffer(GL_ARRAY_BUFFER, mVboHandle);
            glEnableVertexAttribArray(mAttribLocationPosition);
            glEnableVertexAttribArray(mAttribLocationUV);
            glEnableVertexAttribArray(mAttribLocationColor);

#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
            glVertexAttribPointer(mAttribLocationPosition, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, pos));
            glVertexAttribPointer(mAttribLocationUV, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, uv));
            glVertexAttribPointer(mAttribLocationColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, col));
#undef OFFSETOF

            // Restore modified GL state
            glBindBuffer(GL_ARRAY_BUFFER, mLastArrayBuffer);
            glBindVertexArray(mLastVertexArray);

            return true;
        }

        ~ImGuiRenderer()
        {
            if (mVaoHandle) glDeleteVertexArrays(1, &mVaoHandle);
            if (mVboHandle) glDeleteBuffers(1, &mVboHandle);
            if (mElementsHandle) glDeleteBuffers(1, &mElementsHandle);
            mVaoHandle = mVboHandle = mElementsHandle = 0;

            if (mShaderHandle && mVertHandle) glDetachShader(mShaderHandle, mVertHandle);
            if (mVertHandle) glDeleteShader(mVertHandle);
            mVertHandle = 0;

            if (mShaderHandle && mFragHandle) glDetachShader(mShaderHandle, mFragHandle);
            if (mFragHandle) glDeleteShader(mFragHandle);
            mFragHandle = 0;

            if (mShaderHandle) glDeleteProgram(mShaderHandle);
            mShaderHandle = 0;

            destroyFontTexture();
        }

        bool createFontTexture(const ImGuiRenderer_SetFontTexture_Params& params)
        {
            // Upload texture to graphics system
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &mLastTexture);
            glGenTextures(1, &mFontTexture);
            glBindTexture(GL_TEXTURE_2D, mFontTexture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, params.width, params.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, params.pixels);

            // Restore state
            glBindTexture(GL_TEXTURE_2D, mLastTexture);

            return true;
        }

        void destroyFontTexture()
        {
            if (mFontTexture)
            {
                glDeleteTextures(1, &mFontTexture);
                mFontTexture = 0;
            }
        }

        void beginDraw(const ImGuiRenderer_BeginDraw_Params& params)
        {
            // Backup GL state
            glGetIntegerv(GL_CURRENT_PROGRAM, &mLastProgram);
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &mLastTexture);
            glGetIntegerv(GL_ACTIVE_TEXTURE, &mLastActiveTexture);
            glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &mLastArrayBuffer);
            glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &mLastElementArrayBuffer);
            glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &mLastVertexArray);
            glGetIntegerv(GL_BLEND_SRC, &mLastBlendSrc);
            glGetIntegerv(GL_BLEND_DST, &mLastBlendDst);
            glGetIntegerv(GL_BLEND_EQUATION_RGB, &mLastBlendEquationRgb);
            glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &mLastBlendEquationAlpha);
            glGetIntegerv(GL_VIEWPORT, mLastViewport);
            glGetIntegerv(GL_SCISSOR_BOX, mLastScissorBox);
            glIsEnabled(GL_BLEND);
            glIsEnabled(GL_CULL_FACE);
            glIsEnabled(GL_DEPTH_TEST);
            glIsEnabled(GL_SCISSOR_TEST);

            // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
            glEnable(GL_BLEND);
            glBlendEquation(GL_FUNC_ADD);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDisable(GL_CULL_FACE);
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_SCISSOR_TEST);
            glActiveTexture(GL_TEXTURE0);

            // Setup orthographic projection matrix
            mFrameBufferSizeX = params.frameBufferSizeX;
            mFrameBufferSizeY = params.frameBufferSizeY;
            glViewport(0, 0, (GLsizei)mFrameBufferSizeX, (GLsizei)mFrameBufferSizeY);
            const float ortho_projection[4][4] =
            {
                { 2.0f / mFrameBufferSizeX, 0.0f,                      0.0f, 0.0f },
                { 0.0f,                     2.0f / -mFrameBufferSizeY, 0.0f, 0.0f },
                { 0.0f,                     0.0f,                     -1.0f, 0.0f },
                {-1.0f,                     1.0f,                      0.0f, 1.0f },
            };
            glUseProgram(mShaderHandle);
            glUniform1i(mAttribLocationTex, 0);
            glUniformMatrix4fv(mAttribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);
            glBindVertexArray(mVaoHandle);

            glBindBuffer(GL_ARRAY_BUFFER, mVboHandle);
            glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)params.vertexCount * sizeof(ImDrawVert), (GLvoid*)params.vertexData, GL_STREAM_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mElementsHandle);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)params.indexCount * sizeof(ImDrawIdx), (GLvoid*)params.indexData, GL_STREAM_DRAW);
        }

        void drawPrimitives(const ImGuiRenderer_DrawPrimitives_Params& params)
        {
            glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)params.textureId);
            glScissor((int)params.clipX1, (int)(mFrameBufferSizeY - params.clipY2), (int)(params.clipX2 - params.clipX1), (int)(params.clipY2 - params.clipY1));
            glDrawElementsBaseVertex(GL_TRIANGLES, (GLsizei)params.elementCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, (const GLvoid*)(params.indexOffset * sizeof(ImDrawIdx)), params.vertexOffset);
        }

        void endDraw()
        {
            // Restore modified GL state
            glUseProgram(mLastProgram);
            glActiveTexture(mLastActiveTexture);
            glBindTexture(GL_TEXTURE_2D, mLastTexture);
            glBindVertexArray(mLastVertexArray);
            glBindBuffer(GL_ARRAY_BUFFER, mLastArrayBuffer);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mLastElementArrayBuffer);
            glBlendEquationSeparate(mLastBlendEquationRgb, mLastBlendEquationAlpha);
            glBlendFunc(mLastBlendSrc, mLastBlendDst);
            if (mLastEnableBlend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
            if (mLastEnableCullFace) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
            if (mLastEnableDepthTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
            if (mLastEnableScissorTest) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
            glViewport(mLastViewport[0], mLastViewport[1], (GLsizei)mLastViewport[2], (GLsizei)mLastViewport[3]);
            glScissor(mLastScissorBox[0], mLastScissorBox[1], (GLsizei)mLastScissorBox[2], (GLsizei)mLastScissorBox[3]);
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
            mImGuiRenderer->destroyFontTexture();
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
            mImGuiRenderer->beginDraw(params);
            return true;
        }

        void imGuiRenderer_endDraw() override
        {
            if (mImGuiRenderer)
                mImGuiRenderer->endDraw();
        }

        void imGuiRenderer_drawPrimitives(const ImGuiRenderer_DrawPrimitives_Params& params) override
        {
            if (mImGuiRenderer)
                mImGuiRenderer->drawPrimitives(params);
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
