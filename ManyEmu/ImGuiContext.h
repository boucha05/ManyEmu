#pragma once

#include "Graphics.h"

struct SDL_Window;
typedef union SDL_Event SDL_Event;

bool ImGuiContext_Init(SDL_Window* window, IGraphics& graphics);
void ImGuiContext_Shutdown();
void ImGuiContext_NewFrame(SDL_Window* window);
bool ImGuiContext_ProcessEvent(const SDL_Event* event);
bool ImGuiContext_Draw();
