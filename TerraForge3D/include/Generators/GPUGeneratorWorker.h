#pragma once

class ApplicationState;
class Model;
class ClearMeshGenerator;
class GPUNoiseLayerGenerator;


#include "Base/OpenCL/OpenCLContext.h"
#include "json/json.hpp"


#include <vector>
#include <queue>
#include <atomic>

enum GPUGeneratorWorkerState
{
	GPUGeneratorWorkerState_Working,
	GPUGeneratorWorkerState_Waiting,
	GPUGeneratorWorkerState_Dead
};

class GPUGeneratorWorker
{
public:

	GPUGeneratorWorker(ApplicationState* appState, int id);
	~GPUGeneratorWorker();

	void AddJob(int tx, int ty);

	void Start();
	void Quit();
	bool HasFinishedJob();
	std::pair<int, int> GetFinishedJob() { auto& job = jobs_sizes_finished.back(); jobs_sizes_finished.clear(); return job; }

	void WaitForFinish();

	inline int GetJobCount() { return (int)this->jobs_sizes.size(); }
	inline float GetProgress() { return this->progress; }
	inline float GetJobTime() { return this->jobTime; }
	inline std::pair<int, int> GetCurrentWork() { return { currentX, currentY }; }
	inline bool IsIdle() { return this->current_state == GPUGeneratorWorkerState_Waiting && this->jobs_sizes.size() == 0 && this->jobs_sizes_finished.size() == 0; }
	inline const OpenCLContext* GetContextInUse() const { return m_OpenCLContexts[m_OpenCLContextToUse]; }
	inline const std::vector<OpenCLContext*>& GetContexts() const { return m_OpenCLContexts; }
	inline std::vector<OpenCLContext*>& GetContexts() { return m_OpenCLContexts; }
	inline void SetContextInUse(int index) { m_OpenCLContextToUse = index; }

private:
	void Worker();
	void SetupOpenCL();
	void PrepareOpenCLContext(OpenCLContext* context);

private:
	ApplicationState* appState;
	std::atomic<bool> request_quit = false;
	std::atomic<GPUGeneratorWorkerState> current_state = GPUGeneratorWorkerState_Waiting;
	std::queue<std::pair<int, int>> jobs_sizes;
	std::vector<std::pair<int, int>> jobs_sizes_finished;
	std::vector<OpenCLContext*> m_OpenCLContexts;
	std::thread worker_th;
	int m_OpenCLContextToUse = 0;
	float progress = 0.0f;
	float jobTime = 0.0f;
	int id = 0;
	int currentX, currentY;
};
