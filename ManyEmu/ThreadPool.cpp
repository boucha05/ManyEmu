#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t threadCount, size_t priorityCount)
    : mTerminate(false)
    , mTasks(priorityCount)
    , mTaskCount(0)
{
    mWorkers.reserve(threadCount);
    for (size_t n = 0; n < threadCount; ++n)
    {
        mWorkers.emplace_back([this]()
        {
            for (;;)
            {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(mMutex);

                    mCondition.wait(lock, [&]()
                    {
                        return (mTaskCount > 0) || mTerminate;
                    });

                    if (mTerminate)
                        return;

                    for (auto& taskPriority : mTasks)
                    {
                        if (!taskPriority.empty())
                        {
                            task = taskPriority.front();
                            taskPriority.pop();
                            --mTaskCount;
                            break;
                        }
                    }
                }
                if (task)
                    task();
            }
        });
    }
}

ThreadPool::~ThreadPool()
{
    mTerminate = true;
    mCondition.notify_all();
    for (auto& worker : mWorkers)
    {
        worker.join();
    }
}
