#pragma once

#include "Application.h"

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
    void update();
    virtual const char* getName() const override;
    virtual void onGUI() override;

private:
    class Impl;
    Impl&   mImpl;
};
