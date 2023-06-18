#pragma once
#include "Base/Base.h"
#include "Job/Thread.h"
#include "Job/Job.h"

#define TERR3D_THREAD_POOL_MAX_SIZE 64


class ApplicationState;

namespace JobSystem
{
	class Job;

	class JobSystem
	{
		friend class JobManager;

	public:
		JobSystem(ApplicationState* appState);
		~JobSystem();

		void Update();

		std::shared_ptr<Job> AddNewJob(std::shared_ptr<Job> job);
		std::shared_ptr<Job> AddFunctionWorker(std::function<void()> func, JobExecutionModel excutionModel = JobExecutionModel_Async);



		void WaitAll();


	private:
		void UpdateAsyncOnMainThreadJobs();
		void UpdateAsyncJobs();

	private:
		std::unordered_map < uint32_t, std::shared_ptr<Job>> jobs;
		std::queue<uint32_t> asyncJobsQueue;
		std::queue<uint32_t> asyncOnMTJobsQueue;
		std::queue<uint32_t> completedJobs;
		std::vector<uint32_t> onGoingJobs;
		int32_t threadPoolSize = 4;

		const uint32_t maxCompletedJobQueueSize = 64;
		const double flushMTJobsEvery = 5.0f;
		const double maxMTJobExecutionTime = 0.5f;

		ApplicationState* appState;

		// AsyncOnMainThreadJobs data
		std::chrono::time_point<std::chrono::high_resolution_clock> prevTime = std::chrono::high_resolution_clock::now();
		double deltaTime = 0.0;
		double totalTime = 0.0;

		// AsyncJobs data
		Thread threadPool[TERR3D_THREAD_POOL_MAX_SIZE];
		
	};

}

