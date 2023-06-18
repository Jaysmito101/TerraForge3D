#pragma once

#include "Base/Base.h"
#include <condition_variable>


namespace JobSystem
{
	class Job;

	class Thread
	{
	public:
		Thread();
		~Thread();

		void Run();
		void Join();

		void AssignJob(Job* job);

		void Shutdown();

		Job* FinishPendingJob(bool wait = false);

		bool IsFree();

	public:
		bool isAlive = true;
		bool isRunningJob = false;
		bool hasNewJob = false;
		bool hasCompletedJob = false;
		Job* currentJob = nullptr;
		uint32_t id = 0;

	private:
		std::mutex mutex;
		std::thread worker;
		std::condition_variable condVar;
	};

}

