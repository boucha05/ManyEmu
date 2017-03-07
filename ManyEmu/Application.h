#pragma once

#include <imgui_lib.h>

namespace emu
{
    class ISerializer;
}

class Application
{
public:
    class IPlugin
    {
    public:
        virtual ~IPlugin() {}
        virtual bool create(Application& application) = 0;
        virtual void destroy(Application& application) = 0;
        virtual const char* getName() const = 0;
        virtual void onGUI() {}
        virtual void update() {}
        virtual void serialize(emu::ISerializer& serializer) { (void)serializer; }
    };

    virtual bool run() = 0;
    virtual void addPlugin(IPlugin& plugin) = 0;
    virtual void removePlugin(IPlugin& plugin) = 0;

    static Application* create();
    static void destroy(Application& instance);
};
