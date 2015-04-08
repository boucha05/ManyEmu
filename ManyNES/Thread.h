#pragma once
#ifndef __THREAD_H__
#define __THREAD_H__

#include <stdint.h>

class Mutex
{
public:
    Mutex();
    ~Mutex();
    bool tryLock();
    void lock();
    void unlock();

private:
    struct Impl;
    Impl*   mImpl;
};

class ScopedLock
{
public:
    ScopedLock(Mutex& mutex)
        : mMutex(mutex)
    {
        mMutex.lock();
    }

    ~ScopedLock()
    {
        mMutex.unlock();
    }

private:
    ScopedLock();

    Mutex&  mMutex;
};

class ScopedUnlock
{
public:
    ScopedUnlock(Mutex& mutex)
        : mMutex(mutex)
    {
        mMutex.unlock();
    }

    ~ScopedUnlock()
    {
        mMutex.lock();
    }

private:
    ScopedUnlock();

    Mutex&  mMutex;
};

class Event
{
public:
    Event();
    ~Event();
    bool tryWait();
    void wait();
    void signal();
private:
    struct Impl;
    Impl* mImpl;
};

class Semaphore
{
public:
    Semaphore();
    ~Semaphore();
    bool tryWait();
    void wait();
    void signal(uint32_t count = 1);
private:
    struct Impl;
    Impl* mImpl;
};

class IExecutable
{
public:
    virtual void execute() = 0;
};

class Thread
{
public:
    Thread();
    ~Thread();
    void start(IExecutable& executable, const char* name);
    void wait();
private:
    struct Impl;
    Impl* mImpl;
};

#endif