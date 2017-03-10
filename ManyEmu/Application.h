#pragma once

#include <imgui_lib.h>

namespace emu
{
    class ISerializer;
}

class BackendRegistry;
class GameView;
class LogView;

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

    class IView
    {
    public:
        virtual ~IView() {}
        virtual const char* getName() const = 0;
        virtual void onGUI() = 0;
    };

    virtual bool run() = 0;
    virtual void addPlugin(IPlugin& plugin) = 0;
    virtual void removePlugin(IPlugin& plugin) = 0;
    virtual void addView(IView& view) = 0;
    virtual void removeView(IView& view) = 0;

    virtual GameView& getGameView() = 0;
    virtual LogView& getLogView() = 0;

    virtual BackendRegistry& getBackendRegistry() = 0;

    static Application* create();
    static void destroy(Application& instance);
};
