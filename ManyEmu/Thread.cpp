#include "Thread.h"
#include <SDL.h>
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <Windows.h>

struct Mutex::Impl: CRITICAL_SECTION
{
};

Mutex::Mutex()
{
    mImpl = new Impl;
    InitializeCriticalSectionAndSpinCount(mImpl, 2000);
}

Mutex::~Mutex()
{
    DeleteCriticalSection(mImpl);
    delete mImpl;
}

bool Mutex::tryLock()
{
    return TryEnterCriticalSection(mImpl) != FALSE;
}

void Mutex::lock()
{
    EnterCriticalSection(mImpl);
}

void Mutex::unlock()
{
    LeaveCriticalSection(mImpl);
}

///////////////////////////////////////////////////////////////////////////////

Event::Event()
{
    mImpl = static_cast<Impl*>(CreateEvent(NULL, FALSE, FALSE, NULL));
}

Event::~Event()
{
    CloseHandle(static_cast<HANDLE>(mImpl));
}

bool Event::tryWait()
{
    return WaitForSingleObject(static_cast<HANDLE>(mImpl), 0) != WAIT_TIMEOUT;
}

void Event::wait()
{
    WaitForSingleObject(static_cast<HANDLE>(mImpl), INFINITE);
}

void Event::signal()
{
    SetEvent(static_cast<HANDLE>(mImpl));
}

///////////////////////////////////////////////////////////////////////////////

Semaphore::Semaphore()
{
    mImpl = static_cast<Impl*>(CreateSemaphore(NULL, 0, LONG_MAX, NULL));
}

Semaphore::~Semaphore()
{
    CloseHandle(static_cast<HANDLE>(mImpl));
}

bool Semaphore::tryWait()
{
    return WaitForSingleObject(static_cast<HANDLE>(mImpl), 0) != WAIT_TIMEOUT;
}

void Semaphore::wait()
{
    WaitForSingleObject(static_cast<HANDLE>(mImpl), INFINITE);
}

void Semaphore::signal(uint32_t count)
{
    ReleaseSemaphore(static_cast<HANDLE>(mImpl), count, NULL);
}

///////////////////////////////////////////////////////////////////////////////

namespace
{
    int threadFunc(void* context)
    {
        auto executable = static_cast<IExecutable*>(context);
        if (executable)
            executable->execute();
        return 0;
    }
}

Thread::Thread()
{
    mImpl = nullptr;
}

Thread::~Thread()
{
    wait();
}

void Thread::start(IExecutable& executable, const char* name)
{
    mImpl = reinterpret_cast<Impl*>(SDL_CreateThread(threadFunc, name, &executable));
}

void Thread::wait()
{
    if (mImpl)
    {
        int status = 0;
        SDL_WaitThread(reinterpret_cast<SDL_Thread*>(mImpl), &status);
        mImpl = nullptr;
    }
}
