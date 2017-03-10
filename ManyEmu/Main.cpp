#define SDL_MAIN_HANDLED
#include "Application.h"
#include "GameboyPlugin.h"
#include "NESPlugin.h"
#include "Sandbox.h"

int main()
{
    bool success = false;
    auto application = Application::create();
    auto sandbox = Sandbox::create();
    auto pluginGameboy = GameboyPlugin::create();
    auto pluginNES = NESPlugin::create();
    if (application && sandbox && pluginGameboy && pluginNES)
    {
        application->addPlugin(*pluginGameboy);
        application->addPlugin(*pluginNES);
        application->addPlugin(*sandbox);
        success = application->run();
        application->removePlugin(*sandbox);
        application->removePlugin(*pluginNES);
        application->removePlugin(*pluginGameboy);
    }
    if (pluginNES) NESPlugin::destroy(*pluginNES);
    if (pluginGameboy) GameboyPlugin::destroy(*pluginGameboy);
    if (sandbox) Sandbox::destroy(*sandbox);
    if (application) Application::destroy(*application);
    return !success;
}
