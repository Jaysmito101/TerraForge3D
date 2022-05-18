#pragma once
#include "Base/Core/Core.hpp"
#include "Job/Thread.hpp"

#define TF3D_THREAD_POOL_SIZE 8

namespace TerraForge3D
{
	class ApplicationState;

	namespace JobSystem
	{
		class Job;

		class JobSystem
		{
		public:
			JobSystem(ApplicationState* appState);
			~JobSystem();

			void Update();

			uint32_t AddJob(Job* job);

		private:
			void UpdateAsyncOnMainThreadJobs();
			void UpdateAsyncJobs();

		public:
			std::unordered_map<uint32_t, Job*> jobs;
			std::queue<uint32_t> asyncJobsQueue;
			std::queue<uint32_t> asyncOnMTJobsQueue;
			std::queue<uint32_t> completedJobs;
			std::vector<uint32_t> onGoingJobs;

			const uint32_t maxCompletedJobQueueSize = 64;
			const double flushMTJobsEvery = 5.0f;
			const double maxMTJobExecutionTime = 0.5f;

		// private:
			ApplicationState* appState;

			// AsyncOnMainThreadJobs data
			std::chrono::steady_clock::time_point prevTime = std::chrono::high_resolution_clock::now();
			double deltaTime = 0.0;
			double totalTime = 0.0;

			// AsyncJobs data
			Thread threadPool[TF3D_THREAD_POOL_SIZE];
		};

	}

}
