#include "Job/JobSystem.hpp"
#include "Job/Job.hpp"

#include "Data/ApplicationState.hpp"


namespace TerraForge3D
{

	namespace JobSystem
	{

		

		JobSystem::JobSystem(ApplicationState* appState)
		{
			this->appState = appState;
			this->jobs.clear();
		}

		JobSystem::~JobSystem()
		{
			// TODO: Force finish all jobs on different threads
			for (auto& [_, value] : jobs)
			{
				if (value->onDelete)
					value->onDelete(value);
				delete value;
			}
			this->jobs.clear();
		}

		void JobSystem::Update()
		{
			UpdateAsyncOnMainThreadJobs();

			// Delete Completed Jobs execeding maxCompletedJobQueueSize
			if (completedJobs.size() == maxCompletedJobQueueSize)
			{
				Job* job = jobs[completedJobs.front()];
				completedJobs.pop();
				jobs.erase(job->id);
				if (job->onDelete)
					job->onDelete(job);
				delete job;
			}
		}

		uint32_t  JobSystem::AddJob(Job* job)
		{
			TF3D_ASSERT(job, "job is NULL");

			if (job->onSetup)
				job->onSetup(job);

			jobs[job->id] = job;

			if (job->excutionModel == JobExecutionModel_Async)
				asyncJobsQueue.push(job->id);
			else if (job->excutionModel == JobExecutionModel_AsyncOnMainThread)
				asyncOnMTJobsQueue.push(job->id);

			job->status = JobStatus_Queued;

			return job->id;
		}

		void JobSystem::UpdateAsyncOnMainThreadJobs()
		{
			auto currTime = std::chrono::high_resolution_clock::now();
			deltaTime = std::chrono::duration<double>(currTime - prevTime).count();
			prevTime = currTime;
			totalTime += deltaTime;

			if (totalTime >= flushMTJobsEvery)
			{
				totalTime = 0.0f;

				// Flush the jobs
				double timeTaken = 0.0f;
				auto mtJobsBeginTime = std::chrono::high_resolution_clock::now();
				while (asyncOnMTJobsQueue.size() > 0)
				{
					Job* job = jobs[asyncOnMTJobsQueue.front()];
					asyncOnMTJobsQueue.pop();
					job->status = JobStatus_OnGoing;
					
					if (job->onRun)
					{
						if (job->onRun(job))
							job->status = JobStatus_Success;
						else
							job->status = JobStatus_Faliure;
					}
					completedJobs.push(job->id);
					auto mtJobEndTime = std::chrono::high_resolution_clock::now();
					timeTaken = std::chrono::duration<double>(mtJobEndTime - mtJobsBeginTime).count();
					if (timeTaken > maxMTJobExecutionTime)
						break;
				}
			}
		}

	}

}