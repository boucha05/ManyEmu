#pragma once

#include "Core/Core.h"
#include <imgui_lib.h>

struct ImGuiRenderer_SetFontTexture_Params
{
    const ImU32*        pixels;
    int                 width;
    int                 height;
};

struct ImGuiRenderer_BeginDraw_Params
{
    const ImDrawVert*   vertexData;
    uint32_t            vertexCount;
    const ImDrawIdx*    indexData;
    uint32_t            indexCount;
    float               displaySizeX;
    float               displaySizeY;
    uint32_t            frameBufferSizeX;
    uint32_t            frameBufferSizeY;
};

struct ImGuiRenderer_DrawPrimitives_Params
{
    float               clipX1;
    float               clipY1;
    float               clipX2;
    float               clipY2;
    void*               textureId;
    uint32_t            elementCount;
    uint32_t            vertexOffset;
    uint32_t            indexOffset;
};

class IGraphics
{
public:
    virtual ~IGraphics() {}

    virtual bool imGuiRenderer_create() = 0;
    virtual void imGuiRenderer_destroy() = 0;
    virtual bool imGuiRenderer_setFontTexture(const ImGuiRenderer_SetFontTexture_Params& params) = 0;
    virtual void* imGuiRenderer_getFontTexture() = 0;
    virtual bool imGuiRenderer_beginDraw(const ImGuiRenderer_BeginDraw_Params& params) = 0;
    virtual void imGuiRenderer_endDraw() = 0;
    virtual void imGuiRenderer_drawPrimitives(const ImGuiRenderer_DrawPrimitives_Params& params) = 0;

    static IGraphics* create();
    static void destroy(IGraphics* graphics);
};
