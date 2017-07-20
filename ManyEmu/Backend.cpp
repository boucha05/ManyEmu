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
    auto item = mBackends.find(extension);
    return item == mBackends.end() ? nullptr : item->second;
}

bool BackendRegistry::add(IBackend& backend)
{
    emu::IEmulator::SystemInfo info;
    EMU_VERIFY(backend.getEmulator().getSystemInfo(info));

    const char* extensionPos = info.extensions;
    while (*extensionPos)
    {
        const char* separator = extensionPos;
        while (*separator && *separator != '|')
            ++separator;

        std::string extension(extensionPos, separator - extensionPos);
        mBackends[extension] = &backend;

        if (*separator == '|')
            ++separator;
        extensionPos = separator;
    }
    return true;
}

void BackendRegistry::remove(IBackend& backend)
{
    for (auto item = mBackends.begin(); item != mBackends.end();)
    {
        if (item->second == &backend)
            item = mBackends.erase(item);
        else
            ++item;
    }
}

void BackendRegistry::accept(BackendRegistry::Visitor& visitor)
{
    for (auto backend : mBackends)
        visitor.visit(*backend.second);
}
