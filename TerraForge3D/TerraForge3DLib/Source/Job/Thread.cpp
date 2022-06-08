#include "Job/Job.hpp"
#include "Job/Thread.hpp"
#include "TerraForge3D.hpp"

static uint32_t threadId = 0;

namespace TerraForge3D
{

	namespace JobSystem
	{

		

		Thread::Thread()
		{
			this->id = threadId++;
			worker = std::thread([this]()->void {
				this->Run();
				});
			worker.detach();
		}

		Thread::~Thread()
		{
			if(isAlive)
				this->Shutdown();
		}

		void Thread::Run()
		{
			while (isAlive)
			{
				std::unique_lock lock(mutex);
				condVar.wait(lock, [this]()->bool {
					return hasNewJob;
				});

				hasNewJob = false;
				isRunningJob = true;

				if (currentJob)
				{
					currentJob->status = JobStatus_OnGoing;

					if (currentJob->onRun)
					{
						if (currentJob->onRun(currentJob))
							currentJob->status = JobStatus_Success;
						else
							currentJob->status = JobStatus_Faliure;
					}
				}

				isRunningJob = false;
				hasCompletedJob = true;
			}
		}

		void Thread::Join()
		{
			if (worker.joinable())
				worker.join();
		}

		void Thread::AssignJob(Job* job)
		{
			TF3D_ASSERT(currentJob == nullptr, "Current Job not yet cleared");
			{
				std::lock_guard lock(mutex);
				hasCompletedJob = false;
				currentJob = job;
				hasNewJob = true;
				condVar.notify_one();
			}
		}

		void Thread::Shutdown()
		{
			isAlive = false;
			this->Join();
		}

		Job* Thread::FinishPendingJob(bool wait)
		{
			TF3D_ASSERT(currentJob, "No Job assigned");

			// TODO : fix me use something better
			if (wait)
				while (isRunningJob); // Wait for current job to finish

			if (hasCompletedJob)
			{
				hasCompletedJob = false;
				Job* job = currentJob;
				currentJob = nullptr;
				return job;
			}

			return nullptr;
		}

		bool Thread::IsFree()
		{
			return currentJob == nullptr;
		}

	}

}