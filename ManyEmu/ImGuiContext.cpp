#include "imgui.h"
#include "ImGuiContext.h"
#include "examples/imgui_impl_sdl.h"

// SDL
#include <SDL.h>

// Data
static IGraphics*   g_Graphics = nullptr;
static bool         g_FontDirty = true;

bool ImGuiContext_Init(SDL_Window* window, IGraphics& graphics)
{
    EMU_VERIFY(ImGui_ImplSDL2_InitForOpenGL(window, graphics.getGLContext()));

    g_Graphics = &graphics;
    EMU_VERIFY(g_Graphics->imGuiRenderer_create());

    return true;
}

void ImGuiContext_Shutdown()
{
    g_Graphics->imGuiRenderer_destroy();
    ImGui_ImplSDL2_Shutdown();
}

void ImGuiContext_NewFrame(SDL_Window* window)
{
    if (g_FontDirty)
    {
        g_Graphics->imGuiRenderer_refreshFontTexture();
        g_FontDirty = false;
    }

    g_Graphics->imGuiRenderer_newFrame();
    ImGui_ImplSDL2_NewFrame(window);
    ImGui::NewFrame();
}

bool ImGuiContext_ProcessEvent(const SDL_Event* event)
{
    return ImGui_ImplSDL2_ProcessEvent(event);
}

bool ImGuiContext_Draw()
{
    ImGui::Render();
    g_Graphics->imGuiRenderer_render();

    // Update and Render additional Platform Windows
    // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
    //  For this specific demo app we could also call SDL_GL_MakeCurrent(window, gl_context) directly)
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
        SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
    }
    return true;
}
