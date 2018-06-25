#pragma once

#include "Application.h"

class GameBrowser : public Application::IPlugin
{
public:
    static GameBrowser* create();
    static void destroy(GameBrowser& instance);
};
