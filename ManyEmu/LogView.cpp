#include <Core/Core.h>
#include "LogView.h"

#include <fcntl.h>
#include <io.h>

class LogView::Impl
{
public:
    Impl()
        : mCount(0)
    {
        _pipe(mPipes, PIPE_SIZE, O_TEXT);
        mStdOut = dup(fileno(stdout));
        mStdErr = dup(fileno(stderr));
        mBuffer.resize(PIPE_SIZE);
        clear();
    }

    ~Impl()
    {
        _close(mStdErr);
        _close(mStdOut);
        _close(mPipes[READ]);
        _close(mPipes[WRITE]);
    }

    void clear()
    {
        mMessages.clear();
        mMessages.emplace_back();
    }

    void enable()
    {
        if (mCount++ == 0)
        {
            fflush(stdout);
            fflush(stderr);
            _dup2(mPipes[WRITE], fileno(stdout));
            _dup2(mPipes[WRITE], fileno(stderr));
        }
    }

    void disable()
    {
        if (--mCount == 0)
        {
            fflush(stdout);
            fflush(stderr);
            _dup2(mStdOut, fileno(stdout));
            _dup2(mStdErr, fileno(stderr));

            update();
        }
    }

    void update()
    {
        fflush(stdout);
        fflush(stderr);
        if (!_eof(mPipes[READ]))
        {
            unsigned int size = _read(mPipes[READ], mBuffer.data(), PIPE_SIZE);
            unsigned int start = 0;
            for (unsigned int pos = 0; pos < size; ++pos)
            {
                if (mBuffer[pos] == '\n')
                {
                    mMessages.back().append(mBuffer.data() + start, pos - start);
                    mMessages.emplace_back();
                    start = pos + 1;
                }
            }
            mMessages.back().append(mBuffer.data() + start, size - start);
        }
    }

    void onGUI()
    {
        ImGui::BeginChild("Log");
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        ImGuiListClipper clipper(static_cast<int>(mMessages.size()));
        while (clipper.Step())
        {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
            {
                ImGui::Text(mMessages[i].c_str());
            }
        }
        ImGui::PopStyleVar();
        ImGui::EndChild();
    }

private:
    enum PIPES
    {
        READ,
        WRITE,
    };

    static const unsigned int PIPE_SIZE = 65536;

    int32_t     mCount;
    int         mPipes[2];
    int         mStdOut;
    int         mStdErr;

    std::vector<char>           mBuffer;
    std::vector<std::string>    mMessages;
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

void LogView::update()
{
    mImpl.update();
}

void LogView::onGUI()
{
    mImpl.onGUI();
}
