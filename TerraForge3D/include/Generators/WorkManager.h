#pragma once

#define WORK_MANAGER_MAX_CPU_WORKERS 32
#define WORK_MANAGER_MAX_GPU_WORKERS 16

#include "Base/OpenCL/OpenCLPlatform.h"
#include "Generators/CPUGeneratorWorker.h"
#include "Generators/GPUGeneratorWorker.h"
#include "Base/Base.h"
#include <atomic>
#include <thread>

class ApplicationState;

enum WorkManagerWorkStatus
{
	WorkManagerWorkStatus_None = 0,
	WorkManagerWorkStatus_GPUGeneratorsAssigned,
	WorkManagerWorkStatus_GPUGeneratorsFinished,
	WorkManagerWorkStatus_CPUGeneratorsAssigned,
	WorkManagerWorkStatus_CPUGeneratorsFinished,
	WorkManagerWorkStatus_Uploaded,
	WorkManagerWorkStatus_Count
};

static const ImColor s_WorkManagerWorkStatusColors[WorkManagerWorkStatus_Count] =
{
	ImVec4(1, 0, 0, 1),
	ImVec4(0.5, 0.5, 0, 1),
	ImVec4(0.5, 0, 0.5, 1),
	ImVec4(0, 0.5, 0.5, 1),
	ImVec4(0, 0.0, 1.0, 1),
	ImVec4(0, 1.0, 0.0, 1)
};

class WorkManager
{
public:
	WorkManager(ApplicationState* appState);
	~WorkManager();

	void Update();
	void ShowSettings();
	void WaitForFinish();
	bool IsWorking();
	void Resize();
	void StartWork();
	void StartThread();

	inline bool RequireRemesh() const { return m_RequireRemesh; }
	inline void SetRequireRemesh(bool remesh) { m_RequireRemesh = m_RequireRemesh || remesh; }
	inline bool IsUpdationPaused() const { return m_UpdationPaused; }
	inline void SetUpdationPaused(bool paused) { m_UpdationPaused = paused; }

	inline bool IsWindowVisible() const { return m_IsWindowVisible; }
	inline bool* IsWindowVisiblePtr() { return &m_IsWindowVisible; }
	inline void SetWindowVisible(bool visible) { m_IsWindowVisible = visible; }

	inline int32_t GetCPUGeneratorWorkerCount() const { return m_CPUGeneratorWorkerCount; }
	inline int32_t GetGPUGeneratorWorkerCount() const { return m_GPUGeneratorWorkerCount; }
	inline int32_t GetWorkSize() const { return m_WorkSize; }
	inline int32_t GetWorkResolution() const { return m_WorkResolution; }
	
private:
	void ResetWorkStatus();
	void LoadGPUInfo();
	void UpdateWork();
	int FindFreeCPUWorker();
	int FindFreeGPUWorker();

private:
	ApplicationState* m_AppState = nullptr;
	CPUGeneratorWorker* m_CPUGeneratorWorkers[WORK_MANAGER_MAX_CPU_WORKERS] = { nullptr };
	GPUGeneratorWorker* m_GPUGeneratorWorkers[WORK_MANAGER_MAX_GPU_WORKERS] = { nullptr };
	int32_t m_CPUGeneratorWorkerCount = 0;
	int32_t m_GPUGeneratorWorkerCount = 0;
	int32_t m_WorkResolution = 0;
	int32_t m_WorkSize = 0;
	WorkManagerWorkStatus* m_WorkStatusMatrix = nullptr;
	bool m_IsWindowVisible = false;
	std::vector<OpenCLDevice> m_OpenCLDevices;
	std::atomic<bool> m_RequireRemesh = false;
	std::atomic<bool> m_UpdationPaused = false;
	std::atomic<int> m_WorkCompleted = 0;
	std::atomic<int> m_WorkUploaded = 0;
	std::atomic<bool> m_Alive;
	std::atomic<bool> m_WorkerThreadActive;
	std::thread m_WorkThread;
};