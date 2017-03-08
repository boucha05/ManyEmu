#include <Core/Core.h>
#include <Core/Log.h>
#include "LogView.h"
#include "Thread.h"
#include <array>

class LogView::Impl
{
public:
    Impl()
        : mCount(0)
        , mListener(*this)
    {
        emu::Log::addListener(mListener);
        mSeverityColor.resize(static_cast<size_t>(emu::Log::Type::COUNT), ImColor(1.0f, 1.0f, 1.0f));
        getSeverityColor(emu::Log::Type::Debug) = ImColor(0.5f, 0.5f, 0.5f);
        getSeverityColor(emu::Log::Type::Warning) = ImColor(0.0f, 1.0f, 0.0f);
        getSeverityColor(emu::Log::Type::Error) = ImColor(1.0f, 0.0f, 0.0f);
        clear();
    }

    ~Impl()
    {
        emu::Log::removeListener(mListener);
    }

    void clear()
    {
        ScopedLock lock(mMutex);
        mMessages.clear();
    }

    void enable()
    {
        ScopedLock lock(mMutex);
        ++mCount;
    }

    void disable()
    {
        ScopedLock lock(mMutex);
        --mCount;
    }

    void onGUI()
    {
        ScopedLock lock(mMutex);
        ImGui::BeginChild("Log");
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        ImGuiListClipper clipper(static_cast<int>(mMessages.size()));
        while (clipper.Step())
        {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
            {
                const auto& message = mMessages[i];
                ImGui::TextColored(mSeverityColor[static_cast<size_t>(message.type)], message.text.c_str());
            }
        }
        ImGui::PopStyleVar();
        ImGui::EndChild();
    }

    void addMessage(emu::Log::Type type, const char* text)
    {
        ScopedLock lock(mMutex);
        if (mCount > 0)
        {
            mMessages.push_back({ type, text });
        }
    }

    ImColor& getSeverityColor(emu::Log::Type type)
    {
        return mSeverityColor[static_cast<size_t>(type)];
    }

private:
    struct LogListener : public emu::Log::IListener
    {
        LogListener(LogView::Impl& impl)
        {
            mImpl = &impl;
        }

        virtual void message(emu::Log::Type type, const char* text) override
        {
            mImpl->addMessage(type, text);
        }

        LogView::Impl*  mImpl;
    };

    struct Message
    {
        emu::Log::Type  type;
        std::string     text;
    };

    Mutex                                   mMutex;
    int32_t                                 mCount;
    std::vector<Message>                    mMessages;
    LogListener                             mListener;
    std::vector<ImColor>                    mSeverityColor;
};

LogView::LogView()
    : mImpl(*new Impl())
{
}

LogView::~LogView()
{
    delete &mImpl;
}

void LogView::enable()
{
    mImpl.enable();
}

void LogView::disable()
{
    mImpl.disable();
}

const char* LogView::getName() const
{
    return "Log";
}

void LogView::onGUI()
{
    mImpl.onGUI();
}

ImColor& LogView::getSeverityColor(emu::Log::Type type)
{
    return mImpl.getSeverityColor(type);
}