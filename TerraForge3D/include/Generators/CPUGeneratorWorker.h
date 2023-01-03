#pragma once

class ApplicationState;
class Model;
class ClearMeshGenerator;
class CPUNoiseLayersGenerator;
class GPUNoiseLayerGenerator;

class CPUNodeEditor;



#include "Base/OpenCL/ComputeKernel.h"
#include "json/json.hpp"


#include <vector>
#include <queue>
#include <atomic>

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

	void AddJob(std::vector<CPUNoiseLayersGenerator*>& cpuNoiseLayers, std::vector<CPUNodeEditor*> cpuNodeEditors, int start, int size);

	void Start();
	void Quit();

	void WaitForFinish();

	inline int GetJobCount() { return this->jobs.size(); }
	inline float GetProgress() { return this->progress; }
	inline float GetJobTime() { return this->jobTime; }
	inline bool IsIdle() { return this->current_state == CPUGeneratorWorkerState_Waiting; }

private:
	void Worker();

private:
	ApplicationState* appState;
	std::atomic<bool> request_quit = false;
	std::atomic<CPUGeneratorWorkerState> current_state = CPUGeneratorWorkerState_Waiting;
	std::queue< std::pair< std::vector<CPUNoiseLayersGenerator*>, std::vector<CPUNodeEditor*>>> jobs;
	std::queue< std::pair< int, int>> jobs_sizes;
	std::thread worker_th;
	float progress = 0.0f;
	float jobTime = 0.0f;
	int id = 0;
};
