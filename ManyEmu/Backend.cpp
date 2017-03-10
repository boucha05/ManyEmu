#include "Backend.h"

BackendRegistry::BackendRegistry()
{
}

BackendRegistry::~BackendRegistry()
{
    EMU_ASSERT(mBackends.empty());
}

IBackend* BackendRegistry::getBackend(const char* extension)
{
    for (auto backend : mBackends)
    {
        if (strcmpi(backend->getExtension(), extension) == 0)
        {
            return backend;
        }
    }
    return nullptr;
}

void BackendRegistry::add(IBackend& backend)
{
    mBackends.push_back(&backend);
}

void BackendRegistry::remove(IBackend& backend)
{
    auto item = std::find(mBackends.begin(), mBackends.end(), &backend);
    if (item != mBackends.end())
        mBackends.erase(item);
}

void BackendRegistry::accept(BackendRegistry::Visitor& visitor)
{
    for (auto backend : mBackends)
        visitor.visit(*backend);
}
