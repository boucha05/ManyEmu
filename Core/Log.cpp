#include "Log.h"
#include <algorithm>

namespace
{
    thread_local std::vector<emu::Log::IListener*>     mListeners;
}

namespace emu
{
    namespace Log
    {
        void Log::vprintf(Type type, const char* format, va_list args)
        {
            char text[4096];
            vsprintf(text, format, args);

            for (auto listener : mListeners)
            {
                listener->message(type, text);
            }
        }

        void Log::printf(Type type, const char* format, ...)
        {
            va_list args;
            va_start(args, format);
            vprintf(type, format, args);
            va_end(args);
        }

        void Log::addListener(IListener& listener)
        {
            mListeners.push_back(&listener);
        }

        void Log::removeListener(IListener& listener)
        {
            mListeners.erase(std::remove(mListeners.begin(), mListeners.end(), &listener), mListeners.end());
        }
    }
}