#include "Generators/WorkManager.h"
#include "Data/ApplicationState.h"
#include "Utils/Utils.h"
#include "Profiler.h"

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

WorkManager::WorkManager(ApplicationState* appState)
{
	m_AppState = appState;
	m_RequireRemesh = true;
	m_CPUGeneratorWorkerCount = max(1u, std::thread::hardware_concurrency() - 1u);
	for (int i = 0; i < WORK_MANAGER_MAX_CPU_WORKERS; i++)
		m_CPUGeneratorWorkers[i] = new CPUGeneratorWorker(m_AppState, i);
	for (int i = 0; i < WORK_MANAGER_MAX_GPU_WORKERS; i++)
		m_GPUGeneratorWorkers[i] = new GPUGeneratorWorker(m_AppState, i);
	m_Alive = true;
	this->Resize();
	this->LoadGPUInfo();
	// m_GPUGeneratorWorkerCount = (int)m_OpenCLDevices.size(); // For future maybe
	m_GPUGeneratorWorkerCount = std::min((int)m_OpenCLDevices.size(), 1);
}

WorkManager::~WorkManager()
{
	m_Alive = false;
	while (m_WorkerThreadActive) std::this_thread::sleep_for(std::chrono::milliseconds(10));
	if (m_WorkThread.joinable()) m_WorkThread.join();
	for (int i = 0; i < WORK_MANAGER_MAX_CPU_WORKERS; i++)
		delete m_CPUGeneratorWorkers[i];
	for (int i = 0; i < WORK_MANAGER_MAX_GPU_WORKERS; i++)
		delete m_GPUGeneratorWorkers[i];
}

void WorkManager::StartThread()
{
	m_WorkThread = std::thread(&WorkManager::UpdateWork, this);
	m_WorkThread.detach();
	for (int i = 0; i < WORK_MANAGER_MAX_CPU_WORKERS; i++)
		m_CPUGeneratorWorkers[i]->Start();
	for (int i = 0; i < WORK_MANAGER_MAX_GPU_WORKERS; i++)
		m_GPUGeneratorWorkers[i]->Start();
}


void WorkManager::ShowSettings()
{
	if (!m_IsWindowVisible) return;
	ImGui::Begin("Work Manager", &m_IsWindowVisible);

	ImGui::PushFont(GetUIFont("OpenSans-Semi-Bold"));
	auto infoPanel = ImGui::CollapsingHeader("Info");;
	ImGui::PopFont();
	if (infoPanel)
	{
		ImGui::Text("CPU Cores: %d", std::thread::hardware_concurrency());
		ImGui::Text("Max CPU Workers: %d", WORK_MANAGER_MAX_CPU_WORKERS);
		ImGui::Text("Max GPU Workers: %d", WORK_MANAGER_MAX_GPU_WORKERS);
		ImGui::PushFont(GetUIFont("OpenSans-Semi-Bold"));
		ImGui::Text("GPU Info:");
		ImGui::PopFont();
		if (m_OpenCLDevices.size() <= 0) { if (ImGui::Button("Load GPU Info")) LoadGPUInfo(); }
		else
		{
			ImGui::Text("Number of GPU : %d", m_OpenCLDevices.size());
			for (auto& device : m_OpenCLDevices)
			{
				ImGui::Text("Name: %s", device.GetName().data());
				ImGui::Text("Vendor: %s", device.GetVendor().data());
				ImGui::Text("Version: %s", device.GetVersion().data());
				ImGui::Text("Global Memory: %s", FormatMemoryToString(device.GetGlobalMemorySize()).data());
				ImGui::Text("Local Memory: %s", FormatMemoryToString(device.GetGlobalMemorySize()).data());
				ImGui::Separator();
			}
		}
	}
	ImGui::Separator();

	ImGui::PushFont(GetUIFont("OpenSans-Semi-Bold"));
	auto workerStatusPanel = ImGui::CollapsingHeader("Work Status");;
	ImGui::PopFont();
	if (workerStatusPanel)
	{
		static char tp_Buffer[256];
		if (PowerOfTwoDropDown("Subtile Size", &m_SubTileSize, 4, 32))
		{
			this->WaitForFinish();
			this->Resize();
		}
		ImGui::Text("Grid Size         : %d x %d", m_WorkSize, m_WorkSize);
		ImGui::Text("Work Completed    : %d / %d", (int)m_WorkCompleted, (m_WorkSize * m_WorkSize));
		ImGui::Text("Total GPU Time    : %f", m_GPUGeneratorsTime);
		ImGui::Text("Total CPU Time    : %f", m_CPUGeneratorsTime);
		ImGui::Text("Total Upload Time : %f", m_UploadTime);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.0f, 0.0f));
		for (int i = 0; i < m_WorkSize; i++)
		{
			for (int j = 0; j < m_WorkSize; j++)
			{
				sprintf(tp_Buffer, "##WorkerStatusGrid-%d-%d", i, j);
				ImGui::ColorButton(tp_Buffer, s_WorkManagerWorkStatusColors[m_WorkStatusMatrix[i * m_WorkSize + j]], 0, ImVec2(50, 50));
				ImGui::SameLine();
			}
			ImGui::NewLine();
		}
		ImGui::PopStyleVar();
		ImGui::PopStyleVar();
		ImGui::PopStyleVar();
	}
	ImGui::Separator();

	ImGui::PushFont(GetUIFont("OpenSans-Semi-Bold"));
	auto cpuWorkerSettingsPanel = ImGui::CollapsingHeader("CPU Worker Settings");;
	ImGui::PopFont();
	if (cpuWorkerSettingsPanel)
	{
		ImGui::DragInt("CPU Worker Count", &m_CPUGeneratorWorkerCount, 0.1f, 1, WORK_MANAGER_MAX_CPU_WORKERS);
		m_CPUGeneratorWorkerCount = std::clamp(m_CPUGeneratorWorkerCount, 1, WORK_MANAGER_MAX_CPU_WORKERS);
		if (ImGui::CollapsingHeader("CPU Workers"))
		{
			for (int i = 0; i < m_CPUGeneratorWorkerCount; i++)
			{
				ImGui::PushID(i);
				ImGui::Text("CPU Worker : %d", i);
				ImGui::Text("Status     : %s", m_CPUGeneratorWorkers[i]->IsIdle() ? "Idle" : (m_CPUGeneratorWorkers[i]->HasFinishedJob() ? "Finished Job" : "Working"));
				ImGui::Text("Job Time   : %f ms", m_CPUGeneratorWorkers[i]->GetJobTime());
				ImGui::ProgressBar(m_CPUGeneratorWorkers[i]->GetProgress());
				ImGui::Separator();
				ImGui::PopID();
			}
		}
	}

	ImGui::PushFont(GetUIFont("OpenSans-Semi-Bold"));
	auto gpuWorkerSettingsPanel = ImGui::CollapsingHeader("GPU Worker Settings");;
	ImGui::PopFont();
	if (gpuWorkerSettingsPanel)
	{
		ImGui::DragInt("GPU Worker Count", &m_GPUGeneratorWorkerCount, 0.1f, 1, WORK_MANAGER_MAX_GPU_WORKERS);
		m_GPUGeneratorWorkerCount = std::clamp(m_GPUGeneratorWorkerCount, 1, WORK_MANAGER_MAX_GPU_WORKERS);
		if (ImGui::CollapsingHeader("GPU Workers"))
		{
			for (int i = 0; i < m_GPUGeneratorWorkerCount; i++)
			{
				ImGui::PushID(i);
				ImGui::Text("GPU Worker : %d", i);
				ImGui::Text("Device     : %s", m_GPUGeneratorWorkers[i]->GetContextInUse()->GetDevice().GetName().data());
				ImGui::Text("Status     : %s", m_GPUGeneratorWorkers[i]->IsIdle() ? "Idle" : (m_GPUGeneratorWorkers[i]->HasFinishedJob() ? "Finished Job" : "Working"));
				ImGui::Text("Job Time   : %f ms", m_GPUGeneratorWorkers[i]->GetJobTime());
				ImGui::ProgressBar(m_GPUGeneratorWorkers[i]->GetProgress());
				ImGui::Selectable("Change Device");
				if (ImGui::BeginPopupContextItem())
				{
					for (int j = 0; j < m_GPUGeneratorWorkers[i]->GetContexts().size(); j++)
					{
						if (ImGui::Button(m_GPUGeneratorWorkers[i]->GetContexts()[j]->GetDevice().GetName().data()))
							m_GPUGeneratorWorkers[i]->SetContextInUse(j);
					}
					ImGui::EndPopup();
				}
				ImGui::Separator();
				ImGui::PopID();
			}
		}
	}
	ImGui::End();
}

void WorkManager::Update()
{
	
	{
		for (int i = 0; i < m_WorkSize; i++) for (int j = 0; j < m_WorkSize; j++)
		{
			if (m_WorkStatusMatrix[i * m_WorkSize + j] == WorkManagerWorkStatus_CPUGeneratorsFinished)
			{
				START_PROFILER();
				for (int k = 0; k < 3; k++) m_AppState->mainMap.currentTileDataLayers[k]->UploadTileToGPU(j, i);
				END_PROFILER(m_TempTime);
				m_UploadTime += m_TempTime;
				m_WorkStatusMatrix[i * m_WorkSize + j] = WorkManagerWorkStatus_Uploaded;
				m_WorkUploaded += 1;
				break;
			}
		}
	}

	if (IsWorking()) return;
	if (m_UpdationPaused) return;
	if (!m_RequireRemesh) return; m_RequireRemesh = false;
	this->StartWork();
}

void WorkManager::WaitForFinish()
{
	using namespace std::chrono_literals;
	while (this->IsWorking())
		std::this_thread::sleep_for(20ms);
}

bool WorkManager::IsWorking()
{
	return m_WorkCompleted < m_WorkSize * m_WorkSize;
}

void WorkManager::Resize()
{
	//this->WaitForFinish();
	delete m_WorkStatusMatrix;
	m_WorkResolution = std::min(m_SubTileSize, m_AppState->mainMap.tileResolution);
	m_WorkSize = m_AppState->mainMap.tileResolution / m_WorkResolution;
	m_WorkStatusMatrix = new WorkManagerWorkStatus[(uint64_t)m_WorkSize * m_WorkSize];
	for (int i = 0; i < 6; i++) m_AppState->mainMap.currentTileDataLayers[i]->SetTileSize(m_WorkResolution);
	this->ResetWorkStatus();
}

void WorkManager::StartWork()
{
	this->ResetWorkStatus();
}

void WorkManager::LoadGPUInfo()
{
	auto platforms = OpenCLPlatform::GetPlatforms();
	m_OpenCLDevices.clear();
	for (auto& platform : platforms)
	{
		auto devices = platform.GetDevices();
		for (auto& device : devices)
			m_OpenCLDevices.push_back(device);
	}
}

void WorkManager::UpdateWork()
{
	m_WorkerThreadActive = true;
	while (m_Alive)
	{
		
		if (!IsWorking()) { std::this_thread::sleep_for(std::chrono::milliseconds(10)); continue; }

		for (int i = 0; i < m_GPUGeneratorWorkerCount; i++)
		{
			if (m_GPUGeneratorWorkers[i]->HasFinishedJob())
			{
				auto [tx, ty] = m_GPUGeneratorWorkers[i]->GetFinishedJob();
				m_GPUGeneratorsTime += m_GPUGeneratorWorkers[i]->GetJobTime();
				m_WorkStatusMatrix[ty * m_WorkSize + tx] = WorkManagerWorkStatus_GPUGeneratorsFinished;
			}
		}

		for (int i = 0; i < m_CPUGeneratorWorkerCount; i++)
		{ 
			if (m_CPUGeneratorWorkers[i]->HasFinishedJob())
			{
				auto [tx, ty] = m_CPUGeneratorWorkers[i]->GetFinishedJob();
				m_CPUGeneratorsTime += m_CPUGeneratorWorkers[i]->GetJobTime();
				m_WorkStatusMatrix[ty * m_WorkSize + tx] = WorkManagerWorkStatus_CPUGeneratorsFinished;
		 		m_WorkCompleted += 1;
			}
		}

		for (int i = 0; i < m_WorkSize; i++)
		{
			for (int j = 0; j < m_WorkSize; j++)
			{
				if (m_WorkStatusMatrix[i * m_WorkSize + j] == WorkManagerWorkStatus_None)
				{
					auto freeGPUGenerator = FindFreeGPUWorker();
					if (freeGPUGenerator == -1) break;// no free worker, try next frame
					m_WorkStatusMatrix[i * m_WorkSize + j] = WorkManagerWorkStatus_GPUGeneratorsAssigned;
					m_GPUGeneratorWorkers[freeGPUGenerator]->AddJob(j, i);
				}
				else if (m_WorkStatusMatrix[i * m_WorkSize + j] == WorkManagerWorkStatus_GPUGeneratorsFinished)
				{
					auto freeCPUGenerator = FindFreeCPUWorker();
					if (freeCPUGenerator == -1) break;// no free worker, try next frame
					m_WorkStatusMatrix[i * m_WorkSize + j] = WorkManagerWorkStatus_CPUGeneratorsAssigned;
					m_CPUGeneratorWorkers[freeCPUGenerator]->AddJob(m_AppState->meshGenerator->GetCPUNoiseLayers(), m_AppState->meshGenerator->GetCPUNodeEditors(), j, i);
				}
			}
		}
	}
	m_WorkerThreadActive = false;
}

int WorkManager::FindFreeGPUWorker()
{
	for (int i = 0; i < m_GPUGeneratorWorkerCount; i++) if (m_GPUGeneratorWorkers[i]->IsIdle()) return i;
	return -1;
}


int WorkManager::FindFreeCPUWorker()
{
	for (int i = 0; i < m_CPUGeneratorWorkerCount; i++) if (m_CPUGeneratorWorkers[i]->IsIdle()) return i;
	return -1;
}

void WorkManager::ResetWorkStatus()
 {
	m_WorkCompleted = 0; m_WorkUploaded = 0;
	m_GPUGeneratorsTime = 0.0f; m_CPUGeneratorsTime = 0.0f; m_TempTime = 0.0f; m_UploadTime = 0.0f;
	for (auto i = 0; i < m_WorkSize * m_WorkSize; i++) m_WorkStatusMatrix[i] = WorkManagerWorkStatus_None;
}
