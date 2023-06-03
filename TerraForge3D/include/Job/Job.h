#pragma once

#include "Base/Base.h"


namespace JobSystem
{

	enum JobExecutionModel
	{
		JobExecutionModel_Async = 0,
		JobExecutionModel_AsyncOnMainThread,
		JobExecutionModel_Count
	};

	enum JobStatus
	{
		JobStatus_None = 0,
		JobStatus_Success,
		JobStatus_Faliure,
		JobStatus_OnGoing,
		JobStatus_Queued,
		JobStatus_Paused, // For future
		JobStatus_Count
	};

	class Job
	{
	public:
		Job(std::string name = "Job");
		virtual ~Job();


	public:
		std::string name = "Job";
		std::string description = "No Description";
		float progress = 0.0f;
		JobExecutionModel excutionModel = JobExecutionModel_Async;
		JobStatus status = JobStatus_None;
		void* userData = nullptr;
		uint32_t id;
		// std::vector<uint32_t> dependsOn; For future

		// Job Functions
		// std::function<void(Job*)> onSetup = nullptr;
		std::function<bool(Job*)> onRun = nullptr;
		std::function<void(Job*)> onDelete = nullptr;
		std::function<void(Job*)> onComplete = nullptr;

	private:
	};

}

