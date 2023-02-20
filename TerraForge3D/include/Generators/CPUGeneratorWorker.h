#pragma once

class ApplicationState;
class Model;
class ClearMeshGenerator;
class CPUNoiseLayersGenerator;
class GPUNoiseLayerGenerator;

class CPUNodeEditor;



#include "json/json.hpp"


#include <vector>
#include <queue>
#include <atomic>
#include <thread>

enum CPUGeneratorWorkerState
{
	CPUGeneratorWorkerState_Working,
	CPUGeneratorWorkerState_Waiting,
	CPUGeneratorWorkerState_Dead
};

class CPUGeneratorWorker
{
public:

	CPUGeneratorWorker(ApplicationState* appState, int id);
	~CPUGeneratorWorker();

	void AddJob(std::vector<CPUNoiseLayersGenerator*>& cpuNoiseLayers, std::vector<CPUNodeEditor*> cpuNodeEditors, int tx, int ty);

	void Start();
	void Quit();
	bool HasFinishedJob();
	std::pair<int, int> GetFinishedJob() { auto& job = jobs_sizes_finished.back(); jobs_sizes_finished.clear(); return job; }

	void WaitForFinish();

	inline int GetJobCount() { return (int)this->jobs.size(); }
	inline float GetProgress() { return this->progress; }
	inline float GetJobTime() { return this->jobTime; }
	inline std::pair<int, int> GetCurrentWork() { return { currentX, currentY }; }
	inline bool IsIdle() { return this->current_state == CPUGeneratorWorkerState_Waiting && this->jobs.size() == 0 && this->jobs_sizes_finished.size() == 0; }

private:
	void Worker();

private:
	ApplicationState* appState;
	std::atomic<bool> request_quit = false;
	std::atomic<CPUGeneratorWorkerState> current_state = CPUGeneratorWorkerState_Waiting;
	std::queue< std::pair< std::vector<CPUNoiseLayersGenerator*>, std::vector<CPUNodeEditor*>>> jobs;
	std::queue< std::pair< int, int>> jobs_sizes;
	std::vector< std::pair< int, int>> jobs_sizes_finished;
	std::thread worker_th;
	float progress = 0.0f;
	float jobTime = 0.0f;
	int id = 0;
	int currentX, currentY;
};
