#pragma once
#include <cstdint>

namespace ImGui
{

class ISerializer
{
public:
    virtual bool isReading() = 0;
    virtual void serialize(bool& value, const char* name) = 0;
    virtual void serializeTextSize(size_t& value, const char* name) = 0;
    virtual void serializeTextData(char* value, const char* name) = 0;
    virtual void serialize(float& value, const char* name) = 0;
    virtual void serialize(int32_t& value, const char* name) = 0;
    virtual bool serializeSequenceBegin(size_t& size, const char* name) = 0;
    virtual void serializeSequenceEnd() = 0;
    virtual void serializeSequenceItem() = 0;
};

IMGUI_API void ShutdownDock();
IMGUI_API void RootDock(const ImVec2& pos, const ImVec2& size);
IMGUI_API bool BeginDock(const char* label, bool* opened = nullptr, ImGuiWindowFlags extra_flags = 0);
IMGUI_API void EndDock();
IMGUI_API void SetDockActive();
IMGUI_API void SerializeDock(ISerializer& serializer);

} // namespace ImGui