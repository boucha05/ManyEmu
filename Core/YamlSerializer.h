#pragma once

#include "Core.h"
#include "Serializer.h"

namespace emu
{
    class YamlReader : public ISerializer
    {
    public:
        static YamlReader* create();
        static void destroy(YamlReader& instance);

        virtual bool read(const std::string& data) = 0;
    };

    class YamlWriter : public ISerializer
    {
    public:
        static YamlWriter* create();
        static void destroy(YamlWriter& instance);

        virtual bool write(std::string& data) = 0;
    };
}