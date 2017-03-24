#pragma once

// Inspired from the following implementations:
//      https://github.com/progschj/ThreadPool/blob/master/ThreadPool.h
//      http://codereview.stackexchange.com/questions/126026/threadpool-implementation-in-c11

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>
#include <Core/Core.h>

class ThreadPool
{
public:
    ThreadPool(size_t threadCount, size_t priorityCount = 1);
    ~ThreadPool();

    template <typename TFunction>
    void enqueue(size_t priority, TFunction&& function)
    {
        {
            EMU_ASSERT(priority < mTasks.size());
            std::lock_guard<std::mutex> lock(mMutex);
            mTasks[priority].emplace(std::forward<TFunction>(function));
            ++mTaskCount;
        }
        mCondition.notify_one();
    }

    template <typename TFunction>
    void enqueue(TFunction&& function)
    {
        enqueue(0, std::forward<TFunction>(function));
    }

private:
    std::mutex                                      mMutex;
    std::condition_variable                         mCondition;
    std::vector<std::queue<std::function<void()>>>  mTasks;
    std::vector<std::thread>                        mWorkers;
    std::atomic<bool>                               mTerminate;
    size_t                                          mTaskCount;
};
