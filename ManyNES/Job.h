#ifndef __JOB_H__
#define __JOB_H__

#include "Thread.h"
#include <SDL.h>
#include <queue>
#include <set>

struct Job
{
    typedef void (*Func)(void* context);

    Func        func;
    void*       context;
    Job*        unlockedJob;
    uint32_t    waitCount;
    uint32_t    priority;

    void initialize(Func _func, void* _context, uint32_t _priority = 0);
    void unlocks(Job& _unlockedJob);
};

class JobScheduler
{
public:
    JobScheduler();
    ~JobScheduler();
    bool create(uint32_t numThreads, uint32_t numPriorities = 1);
    void destroy();
    void addJob(Job& job);
    void addJobs(uint32_t count, Job* const* jobs);

private:
    struct WorkerThread;

    typedef std::queue<Job*> JobQueue;
    typedef std::vector<JobQueue> JobQueueArray;
    typedef std::set<Job*> JobSet;
    typedef std::vector<WorkerThread*> WorkerThreadArray;

    void addReadyJob(Job& job);
    Job* pickJob(uint32_t threadIndex);
    void terminateJob(Job& job, uint32_t threadIndex);
    void workerThreadLogic(uint32_t threadIndex);

    Mutex               mMutex;
    Semaphore           mAvailableJob;
    JobQueueArray       mJobQueues;
    JobSet              mBlockedJobs;
    WorkerThreadArray   mWorkerThreads;
    uint32_t            mReadyJobs;
    uint32_t            mBlockedThreads;
    bool                mTerminate;
};

#endif
