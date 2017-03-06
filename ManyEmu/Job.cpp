#include "Job.h"
#include <Core/Core.h>

void Job::initialize(Func _func, void* _context, uint32_t _priority)
{
    func = _func;
    context = _context;
    unlockedJob = nullptr;
    waitCount = 0;
    priority = _priority;
}

void Job::unlocks(Job& _unlockedJob)
{
    unlockedJob = &_unlockedJob;
    _unlockedJob.waitCount = 0;
}

////////////////////////////////////////////////////////////////////////////////

struct JobScheduler::WorkerThread : public IExecutable
{
    WorkerThread(JobScheduler* _scheduler, uint32_t threadIndex)
        : scheduler(_scheduler)
    {
        EMU_UNUSED(threadIndex);
    }

    virtual void execute() override
    {
        scheduler->workerThreadLogic(threadIndex);
    }

    Thread          thread;
    JobScheduler*   scheduler;
    uint32_t        threadIndex;
};

JobScheduler::JobScheduler()
    : mReadyJobs(0)
    , mBlockedThreads(0)
    , mTerminate(false)
{
}

JobScheduler::~JobScheduler()
{
}

bool JobScheduler::create(uint32_t numThreads, uint32_t numPriorities)
{
    ScopedLock lock(mMutex);
    mJobQueues.resize(numPriorities);
    mWorkerThreads.resize(numThreads, nullptr);
    for (uint32_t index = 0; index < numThreads; ++index)
    {
        auto workerThread = new WorkerThread(this, index);
        mWorkerThreads[index] = workerThread;
        workerThread->thread.start(*workerThread, "Worker thread");
    }
    return true;
}

void JobScheduler::destroy()
{
    ScopedLock lock(mMutex);
    mJobQueues.clear();
    mTerminate = true;

    if (mReadyJobs < mBlockedThreads)
        mAvailableJob.signal(mBlockedThreads - mReadyJobs);
    mReadyJobs = 0;

    uint32_t numThreads = static_cast<uint32_t>(mWorkerThreads.size());
    {
        ScopedUnlock unlock(mMutex);
        for (uint32_t index = 0; index < numThreads; ++index)
            mWorkerThreads[index]->thread.wait();
    }

    for (uint32_t index = 0; index < numThreads; ++index)
        delete mWorkerThreads[index];
    mWorkerThreads.clear();
    mTerminate = false;
}

void JobScheduler::addJob(Job& job)
{
    Job* jobs[] = { &job };
    addJobs(1, jobs);
}

void JobScheduler::addJobs(uint32_t count, Job* const* jobs)
{
    ScopedLock lock(mMutex);
    for (uint32_t index = 0; index < count; ++index)
    {
        Job* job = jobs[index];
        if (job->waitCount)
            mBlockedJobs.insert(job);
        else
            addReadyJob(*job);
    }
}

void JobScheduler::addReadyJob(Job& job)
{
    mJobQueues[job.priority].push(&job);
    if (++mReadyJobs <= mBlockedThreads)
        mAvailableJob.signal();
}

Job* JobScheduler::pickJob(uint32_t threadIndex)
{
    EMU_UNUSED(threadIndex);
    while (!mReadyJobs)
    {
        if (mTerminate)
            return nullptr;

        ++mBlockedThreads;
        {
            ScopedUnlock unlock(mMutex);
            mAvailableJob.wait();
        }
        --mBlockedThreads;
    }

    for (auto queue = mJobQueues.rbegin(), end = mJobQueues.rend(); queue != end; ++queue)
    {
        if (queue->empty())
            continue;

        --mReadyJobs;
        auto job = queue->front();
        queue->pop();
        return job;
    }
    return nullptr;
}

void JobScheduler::terminateJob(Job& job, uint32_t threadIndex)
{
    EMU_UNUSED(threadIndex);
    auto unlockedJob = job.unlockedJob;
    if (unlockedJob && (--unlockedJob->waitCount == 0))
    {
        mBlockedJobs.erase(unlockedJob);
        addReadyJob(*unlockedJob);
    }
}

void JobScheduler::workerThreadLogic(uint32_t threadIndex)
{
    ScopedLock lock(mMutex);
    while (!mTerminate)
    {
        Job* job = pickJob(threadIndex);
        if (job)
        {
            ScopedUnlock unlock(mMutex);
            job->func(job->context);
        }
    }
}