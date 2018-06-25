#include "Application.h"
#include "GameboyPlugin.h"
#include "GameBrowser.h"
#include "NESPlugin.h"
#include "Sandbox.h"

int SDL_main(int argc, char* argv[])
{
    EMU_UNUSED(argc);
    EMU_UNUSED(argv);
    bool success = false;
    auto application = Application::create();
    auto gameBrowser = GameBrowser::create();
    auto sandbox = Sandbox::create();
    auto pluginGameboy = GameboyPlugin::create();
    auto pluginNES = NESPlugin::create();
    if (application && sandbox && pluginGameboy && pluginNES)
    {
        application->addPlugin(*pluginGameboy);
        application->addPlugin(*pluginNES);
        application->addPlugin(*gameBrowser);
        application->addPlugin(*sandbox);
        success = application->run();
        application->removePlugin(*sandbox);
        application->removePlugin(*gameBrowser);
        application->removePlugin(*pluginNES);
        application->removePlugin(*pluginGameboy);
    }
    if (pluginNES) NESPlugin::destroy(*pluginNES);
    if (pluginGameboy) GameboyPlugin::destroy(*pluginGameboy);
    if (sandbox) Sandbox::destroy(*sandbox);
    if (gameBrowser) GameBrowser::destroy(*gameBrowser);
    if (application) Application::destroy(*application);
    return !success;
}
