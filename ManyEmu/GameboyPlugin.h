#include "Application.h"

class GameboyPlugin : public Application::IPlugin
{
public:
    static GameboyPlugin* create();
    static void destroy(GameboyPlugin& instance);
};
