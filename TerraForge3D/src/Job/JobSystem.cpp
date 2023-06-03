#include "Job/JobSystem.h"
#include "Job/Job.h"

namespace JobSystem
{



	JobSystem::JobSystem(ApplicationState* appState)
	{
		this->appState = appState;
		this->jobs.clear();
		this->threadPoolSize = std::thread::hardware_concurrency();
	}

	JobSystem::~JobSystem()
	{
		WaitAll();
		for (auto& [_, value] : jobs)
		{
			if (value->onDelete)
				value->onDelete(value.get());
		}
		this->jobs.clear();
	}

	void JobSystem::Update()
	{
		// Assign Jobs for AsyncOnMainThread
		UpdateAsyncOnMainThreadJobs();

		// Assign Jobs for Async
		UpdateAsyncJobs();

		// Delete Completed Jobs execeding maxCompletedJobQueueSize
		if (completedJobs.size() == maxCompletedJobQueueSize)
		{
			auto job = jobs[completedJobs.front()];
			completedJobs.pop();
			jobs.erase(job->id);
			if (job->onDelete)
				job->onDelete(job.get());
		}
	}

	std::shared_ptr<Job> JobSystem::AddNewJob(std::shared_ptr<Job> job)
	{
		// TF3D_ASSERT(job, "job is NULL");

		// if (job->onSetup)
		//  	job->onSetup(job);

		jobs[job->id] = job;

		if (job->excutionModel == JobExecutionModel_Async)
			asyncJobsQueue.push(job->id);
		else if (job->excutionModel == JobExecutionModel_AsyncOnMainThread)
			asyncOnMTJobsQueue.push(job->id);

		job->status = JobStatus_Queued;

		return job;
	}

	std::shared_ptr<Job> JobSystem::AddFunctionWorker(std::function<void()> func, JobExecutionModel excutionModel)
	{
		auto job = std::make_shared<Job>();
		job->onRun = [func](Job*) -> bool { func(); return true; };
		return AddNewJob(job);
	}

	void JobSystem::WaitAll()
	{
		for (int i = 0; i < threadPoolSize; i++)
		{
			threadPool[i].FinishPendingJob(true);
		}
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
				auto job = jobs[asyncOnMTJobsQueue.front()];
				asyncOnMTJobsQueue.pop();
				job->status = JobStatus_OnGoing;

				if (job->onRun)
				{
					if (job->onRun(job.get()))
						job->status = JobStatus_Success;
					else
						job->status = JobStatus_Faliure;
				}
				if (job->onComplete)
					job->onComplete(job.get());
				completedJobs.push(job->id);
				auto mtJobEndTime = std::chrono::high_resolution_clock::now();
				timeTaken = std::chrono::duration<double>(mtJobEndTime - mtJobsBeginTime).count();
				if (timeTaken > maxMTJobExecutionTime)
					break;
			}
		}
	}

	void JobSystem::UpdateAsyncJobs()
	{
		// Recover the finished Jobs
		for (int i = 0; i < threadPoolSize; i++)
		{
			if (threadPool[i].hasCompletedJob)
			{
				Job* job = threadPool[i].FinishPendingJob();
				if (job->onComplete)
					job->onComplete(job);
				completedJobs.push(job->id);
			}
		}

		// Assign the Jobs
		for (int i = 0; i < threadPoolSize && !asyncJobsQueue.empty(); i++)
		{
			if (threadPool[i].IsFree())
			{
				auto job = jobs[asyncJobsQueue.front()];
				asyncJobsQueue.pop();
				threadPool[i].AssignJob(job.get());
			}
		}
	}

}

