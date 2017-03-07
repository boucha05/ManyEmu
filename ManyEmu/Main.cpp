#include "Application.h"
#include "Sandbox.h"

int main()
{
    bool success = false;
    auto application = Application::create();
    auto sandbox = Sandbox::create();
    if (application && sandbox)
    {
        application->addPlugin(*sandbox);
        success = application->run();
        application->removePlugin(*sandbox);
    }
    if (sandbox) Sandbox::destroy(*sandbox);
    if (application) Application::destroy(*application);
    return !success;
}
