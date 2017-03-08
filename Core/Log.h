#pragma once

#include <Core/Core.h>
#include <array>
#include <cstdarg>
#include <vector>

namespace emu
{
    namespace Log
    {
        enum class Type
        {
            Debug,
            Info,
            Warning,
            Error,
            COUNT
        };

        class IListener
        {
        public:
            virtual ~IListener() {}
            virtual void message(Log::Type, const char* text) = 0;
        };

        void vprintf(Type type, const char* format, va_list args);
        void printf(Type type, const char* format, ...);
        void addListener(IListener& listener);
        void removeListener(IListener& listener);
    };
}