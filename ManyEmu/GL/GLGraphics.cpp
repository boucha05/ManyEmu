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
        GLuint       g_FontTexture = 0;
        int          g_ShaderHandle = 0, g_VertHandle = 0, g_FragHandle = 0;
        int          g_AttribLocationTex = 0, g_AttribLocationProjMtx = 0;
        int          g_AttribLocationPosition = 0, g_AttribLocationUV = 0, g_AttribLocationColor = 0;
        unsigned int g_VboHandle = 0, g_VaoHandle = 0, g_ElementsHandle = 0;

        bool initialize()
        {
            // Backup GL state
            glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
            glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);

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

            g_ShaderHandle = glCreateProgram();
            g_VertHandle = glCreateShader(GL_VERTEX_SHADER);
            g_FragHandle = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(g_VertHandle, 1, &vertex_shader, 0);
            glShaderSource(g_FragHandle, 1, &fragment_shader, 0);
            glCompileShader(g_VertHandle);
            glCompileShader(g_FragHandle);
            glAttachShader(g_ShaderHandle, g_VertHandle);
            glAttachShader(g_ShaderHandle, g_FragHandle);
            glLinkProgram(g_ShaderHandle);

            g_AttribLocationTex = glGetUniformLocation(g_ShaderHandle, "Texture");
            g_AttribLocationProjMtx = glGetUniformLocation(g_ShaderHandle, "ProjMtx");
            g_AttribLocationPosition = glGetAttribLocation(g_ShaderHandle, "Position");
            g_AttribLocationUV = glGetAttribLocation(g_ShaderHandle, "UV");
            g_AttribLocationColor = glGetAttribLocation(g_ShaderHandle, "Color");

            glGenBuffers(1, &g_VboHandle);
            glGenBuffers(1, &g_ElementsHandle);

            glGenVertexArrays(1, &g_VaoHandle);
            glBindVertexArray(g_VaoHandle);
            glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
            glEnableVertexAttribArray(g_AttribLocationPosition);
            glEnableVertexAttribArray(g_AttribLocationUV);
            glEnableVertexAttribArray(g_AttribLocationColor);

#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
            glVertexAttribPointer(g_AttribLocationPosition, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, pos));
            glVertexAttribPointer(g_AttribLocationUV, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, uv));
            glVertexAttribPointer(g_AttribLocationColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, col));
#undef OFFSETOF

            // Restore modified GL state
            glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
            glBindVertexArray(last_vertex_array);

            return true;
        }

        ~ImGuiRenderer()
        {
            if (g_VaoHandle) glDeleteVertexArrays(1, &g_VaoHandle);
            if (g_VboHandle) glDeleteBuffers(1, &g_VboHandle);
            if (g_ElementsHandle) glDeleteBuffers(1, &g_ElementsHandle);
            g_VaoHandle = g_VboHandle = g_ElementsHandle = 0;

            if (g_ShaderHandle && g_VertHandle) glDetachShader(g_ShaderHandle, g_VertHandle);
            if (g_VertHandle) glDeleteShader(g_VertHandle);
            g_VertHandle = 0;

            if (g_ShaderHandle && g_FragHandle) glDetachShader(g_ShaderHandle, g_FragHandle);
            if (g_FragHandle) glDeleteShader(g_FragHandle);
            g_FragHandle = 0;

            if (g_ShaderHandle) glDeleteProgram(g_ShaderHandle);
            g_ShaderHandle = 0;

            destroyFontTexture();
        }

        bool createFontTexture(const ImGuiRenderer_SetFontTexture_Params& params)
        {
            // Upload texture to graphics system
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
            glGenTextures(1, &g_FontTexture);
            glBindTexture(GL_TEXTURE_2D, g_FontTexture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, params.width, params.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, params.pixels);

            // Restore state
            glBindTexture(GL_TEXTURE_2D, last_texture);

            return true;
        }

        void destroyFontTexture()
        {
            if (g_FontTexture)
            {
                glDeleteTextures(1, &g_FontTexture);
                g_FontTexture = 0;
            }
        }

        GLint last_program;
        GLint last_texture;
        GLint last_active_texture;
        GLint last_array_buffer;
        GLint last_element_array_buffer;
        GLint last_vertex_array;
        GLint last_blend_src;
        GLint last_blend_dst;
        GLint last_blend_equation_rgb;
        GLint last_blend_equation_alpha;
        GLint last_viewport[4];
        GLint last_scissor_box[4];
        GLboolean last_enable_blend;
        GLboolean last_enable_cull_face;
        GLboolean last_enable_depth_test;
        GLboolean last_enable_scissor_test;
        int32_t mFrameBufferSizeX = 0;
        int32_t mFrameBufferSizeY = 0;

        void beginDraw(const ImGuiRenderer_BeginDraw_Params& params)
        {
            // Backup GL state
            glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
            glGetIntegerv(GL_ACTIVE_TEXTURE, &last_active_texture);
            glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
            glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
            glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
            glGetIntegerv(GL_BLEND_SRC, &last_blend_src);
            glGetIntegerv(GL_BLEND_DST, &last_blend_dst);
            glGetIntegerv(GL_BLEND_EQUATION_RGB, &last_blend_equation_rgb);
            glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &last_blend_equation_alpha);
            glGetIntegerv(GL_VIEWPORT, last_viewport);
            glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
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
            glUseProgram(g_ShaderHandle);
            glUniform1i(g_AttribLocationTex, 0);
            glUniformMatrix4fv(g_AttribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);
            glBindVertexArray(g_VaoHandle);

            glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
            glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)params.vertexCount * sizeof(ImDrawVert), (GLvoid*)params.vertexData, GL_STREAM_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ElementsHandle);
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
            glUseProgram(last_program);
            glActiveTexture(last_active_texture);
            glBindTexture(GL_TEXTURE_2D, last_texture);
            glBindVertexArray(last_vertex_array);
            glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
            glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
            glBlendFunc(last_blend_src, last_blend_dst);
            if (last_enable_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
            if (last_enable_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
            if (last_enable_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
            if (last_enable_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
            glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
            glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);
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
            return mImGuiRenderer ? (void *)(intptr_t)mImGuiRenderer->g_FontTexture : nullptr;
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
