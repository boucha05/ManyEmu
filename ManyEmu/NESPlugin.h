#include "Application.h"

class NESPlugin : public Application::IPlugin
{
public:
    static NESPlugin* create();
    static void destroy(NESPlugin& instance);
};
