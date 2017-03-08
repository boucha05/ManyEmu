#pragma once

#include "Application.h"
#include <Core/Log.h>

class LogView : public Application::IView
{
public:
    class Enabled
    {
    public:
        Enabled(LogView& logView)
            : mLogView(logView)
        {
            mLogView.enable();
        }

        ~Enabled()
        {
            mLogView.disable();
        }

    private:
        LogView&    mLogView;
    };

    class Disabled
    {
    public:
        Disabled(LogView& logView)
            : mLogView(logView)
        {
            mLogView.disable();
        }

        ~Disabled()
        {
            mLogView.enable();
        }

    private:
        LogView&    mLogView;
    };

    LogView();
    ~LogView();
    void enable();
    void disable();
    virtual const char* getName() const override;
    virtual void onGUI() override;
    ImColor& getSeverityColor(emu::Log::Type type);

private:
    class Impl;
    Impl&   mImpl;
};
