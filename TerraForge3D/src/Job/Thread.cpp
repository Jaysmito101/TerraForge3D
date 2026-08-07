#include "Job/Thread.h"
#include "Job/Job.h"
#include "Profiler.h"

static uint32_t threadId = 0;

namespace tf3d::job
{

    Thread::Thread()
    {
        this->id = threadId++;
        worker   = std::thread([this]() -> void {
            TF3D_PROFILE_THREAD_NAME("Job Worker " + std::to_string(this->id));
            this->Run();
        });
        worker.detach();
    }

    Thread::~Thread()
    {
        if (isAlive)
            this->Shutdown();
    }

    void Thread::Run()
    {
        while (isAlive) {
            TF3D_PROFILE_BEGIN(queueWaitScope, "job/queue-wait");
            std::unique_lock lock(mutex);
            condVar.wait(lock, [this]() -> bool {
                return hasNewJob;
            });
            queueWaitScope.End();

            hasNewJob    = false;
            isRunningJob = true;

            if (currentJob) {
                currentJob->status = JobStatus_OnGoing;
                TF3D_PROFILE_FLOW_STEP_DOMAIN("job/request", currentJob->profileFlowId, "started",
                                              PerformanceMonitor::Domain::Job);

                TF3D_PROFILE_SCOPE_FLOW("job/execute", PerformanceMonitor::Domain::Job, currentJob->profileFlowId);
                TF3D_PROFILE_VALUE_DOMAIN_FLOW("job/execute/id", currentJob->id, 0, 0,
                                               PerformanceMonitor::Domain::Job, currentJob->profileFlowId);

                if (currentJob->onRun) {
                    if (currentJob->onRun(currentJob))
                        currentJob->status = JobStatus_Success;
                    else
                        currentJob->status = JobStatus_Faliure;
                }
                TF3D_PROFILE_FLOW_STEP_DOMAIN("job/request", currentJob->profileFlowId, "executed",
                                              PerformanceMonitor::Domain::Job);
            }

            isRunningJob    = false;
            hasCompletedJob = true;
        }
    }

    void Thread::Join()
    {
        if (worker.joinable())
            worker.join();
    }

    void Thread::AssignJob(Job *job)
    {
        // TF3D_ASSERT(currentJob == nullptr, "Current Job not yet cleared");

        {
            std::lock_guard lock(mutex);
            hasCompletedJob = false;
            currentJob      = job;
            hasNewJob       = true;
            condVar.notify_one();
        }
    }

    void Thread::Shutdown()
    {
        isAlive = false;
        this->Join();
    }

    Job *Thread::FinishPendingJob(bool wait)
    {
        // TF3D_ASSERT(currentJob, "No Job assigned");

        // TODO : fix me use something better
        if (wait) {
            TF3D_PROFILE_SCOPE_DOMAIN("job/wait", PerformanceMonitor::Domain::Wait);
            while (isRunningJob)
                ; // Wait for current job to finish
        }

        if (hasCompletedJob) {
            hasCompletedJob = false;
            Job *job        = currentJob;
            currentJob      = nullptr;
            return job;
        }

        return nullptr;
    }

    bool Thread::IsFree()
    {
        return currentJob == nullptr;
    }

} // namespace tf3d::job
