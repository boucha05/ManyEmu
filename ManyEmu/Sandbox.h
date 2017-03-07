#pragma once

#include "Application.h"

class Sandbox : public Application::IPlugin
{
public:
    static Sandbox* create();
    static void destroy(Sandbox& instance);
};
