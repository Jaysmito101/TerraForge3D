#include "Generators/GenerationManager.h"
#include "Data/ApplicationState.h"
#include "Base/ComputeShader.h"
#include "Utils/Utils.h"
#include "Profiler.h"

GenerationManager::GenerationManager(ApplicationState* appState)
{
	// if (!BiomeManager::LoadBaseShapeGenerators(appState)) Log("Failed to load Base Shape Generators!");
	m_AppState = appState;
	m_AppState->eventManager->Subscribe("TileResolutionChanged", BIND_EVENT_FN(OnTileResolutionChange));
	m_AppState->eventManager->Subscribe("ForceUpdate", BIND_EVENT_FN(UpdateInternal));
	m_HeightmapData = std::make_shared<GeneratorData>();
	m_SwapBuffer = std::make_shared<GeneratorData>();
	m_BiomeManagers.push_back(std::make_shared<BiomeManager>(m_AppState));
	m_BiomeManagers.back()->SetName("Default Global");

	//m_BlurrShader = new ComputeShader(source);
}

GenerationManager::~GenerationManager()
{
}

void GenerationManager::Update()
{
	if (m_UpdationPaused) return;

	//if (m_RequireUpdation)
	{
		UpdateInternal();
	}
}

bool GenerationManager::UpdateInternal(const std::string& params, void* paramsPtr)
{
	auto forceUpdate = (params == "ForceUpdate") || m_RequireUpdation;
	for (auto biome : m_BiomeManagers)
	{
		if (biome->IsUpdationRequired() || forceUpdate)
		{
			biome->Update(m_SwapBuffer.get(), m_SeedTexture.get());
		}
	}
	if(m_BiomeManagers.size() > 0) m_BiomeManagers[0]->GetBiomeData()->CopyTo(m_HeightmapData.get());

	m_RequireUpdation = false;
	return false;
}

void GenerationManager::PullSeedTextureFromActiveMesh()
{
	if (!m_UseSeedFromActiveMesh) return;
	m_SeedTexture->MakeCPUCopy();
	m_SeedTexture->ZeroCPUCopy();
	auto mesh = m_AppState->mainModel->mesh;
	for (auto i = 0; i < mesh->GetVertexCount(); i++)
	{
		const auto& vertex = mesh->GetVertex(i);
		m_SeedTexture->SetPixel(vertex.texCoord.x, vertex.texCoord.y, vertex.position.x, vertex.position.z, vertex.position.y);
	}
	m_SeedTexture->UploadCPUCopy();
	m_SeedTexture->FreeCPUCopy();
}


void GenerationManager::ShowSettings()
{
	ShowSettingsInspector();
	ShowSettingsDetailed();
}

void GenerationManager::ShowSettingsInspector()
{
	static bool s_TempBoolean = false;
	ImGui::Begin("Generator Inspector", &m_IsWindowVisible);

	if (ImGui::Selectable("Options", m_SelectedNodeUI.m_ID == "GlobalOptions"))
	{
		m_SelectedNodeUI.m_ID = "GlobalOptions";
		m_SelectedNodeUI.m_ObjectName = SelectedUINodeObjectType_GlobalOptions;
	}

	s_TempBoolean = ImGui::TreeNodeEx("Biomes", ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_AllowItemOverlap);
	ImGui::SameLine();
	if (ImGui::Button("Add##BiomeAdd"))
	{
		m_BiomeManagers.push_back(std::make_shared<BiomeManager>(m_AppState));
		m_BiomeManagers.back()->SetName("Biome " + std::to_string(m_BiomeManagers.size()));
	}
	if (s_TempBoolean)
	{
		if (m_BiomeManagers.size() == 0) ImGui::Text("No Biomes Added!");
		for (int i = 0; i < m_BiomeManagers.size(); i++)
		{
			auto biome = m_BiomeManagers[i];
			ImGui::PushID(biome->GetBiomeID().c_str());
			s_TempBoolean = ImGui::TreeNodeEx(biome->GetBiomeName(), ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_AllowItemOverlap);
			ImGui::SameLine();
			if (ImGui::Button("Delete"))
			{
				m_BiomeManagers.erase(m_BiomeManagers.begin() + i);
				SetUINodeData(-1, None);
			}
			if (s_TempBoolean)
			{
				if (ImGui::Selectable("General", m_SelectedNodeUI.m_ID == MakeUINodeID(i, General)))
				{
					SetUINodeData(i, General);
				}
				if (ImGui::Selectable("Base Shape", m_SelectedNodeUI.m_ID == MakeUINodeID(i, BaseShape)))
				{
					SetUINodeData(i, BaseShape);
				}
				if (ImGui::Selectable("Custom Base Shape", m_SelectedNodeUI.m_ID == MakeUINodeID(i, CustomBaseShape)))
				{
					SetUINodeData(i, CustomBaseShape);
				}
				if (ImGui::Selectable("Base Noise", m_SelectedNodeUI.m_ID == MakeUINodeID(i, BaseNoise)))
				{
					SetUINodeData(i, BaseNoise);
				}
				s_TempBoolean = ImGui::TreeNodeEx("Filters", ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_AllowItemOverlap);
				ImGui::SameLine();
				if (ImGui::Button("Add##BiomeFilterAdd"))
				{
					// TODO
				}
				if (s_TempBoolean)
				{
					const auto& filters = biome->GetFilters();
					if (filters.size() == 0) ImGui::Text("No Filters Added!");
					ImGui::TreePop();
				}
				ImGui::TreePop();
			}
			ImGui::PopID();
		}
		ImGui::TreePop();
	}

	ImGui::End();
}

void GenerationManager::ShowSettingsDetailed()
{
	ImGui::Begin("Generation Settings", &m_IsWindowVisible);

	if (m_SelectedNodeUI.m_ObjectName == SelectedUINodeObjectType_GlobalOptions) ShowSettingsGlobalOptions();
	else if (m_SelectedNodeUI.m_ObjectName == SelectedUINodeObjectType_General) m_BiomeManagers[m_SelectedNodeUI.m_BiomeIndex]->ShowGeneralSettings();
	else if (m_SelectedNodeUI.m_ObjectName == SelectedUINodeObjectType_BaseShape) m_BiomeManagers[m_SelectedNodeUI.m_BiomeIndex]->ShowBaseShapeSettings();
	else if (m_SelectedNodeUI.m_ObjectName == SelectedUINodeObjectType_CustomBaseShape) m_BiomeManagers[m_SelectedNodeUI.m_BiomeIndex]->ShowCustomBaseShapeSettings();
	else if (m_SelectedNodeUI.m_ObjectName == SelectedUINodeObjectType_BaseNoise) m_BiomeManagers[m_SelectedNodeUI.m_BiomeIndex]->ShowBaseNoiseSettings();

	ImGui::End();
}

void GenerationManager::ShowSettingsGlobalOptions()
{
	ImGui::Checkbox("Use Seed Texture", &m_UseSeedFromActiveMesh);
	ImGui::Checkbox("Auto Updation Paused", &m_UpdationPaused);

	if (m_UseSeedFromActiveMesh && m_SeedTexture == nullptr)
	{
		m_SeedTexture = std::make_shared<GeneratorTexture>(m_SeedTextureResolution, m_SeedTextureResolution);
		m_RequireUpdation = true;
	}
	else if (!m_UseSeedFromActiveMesh && m_SeedTexture != nullptr)
	{
		m_SeedTexture = nullptr;
		m_RequireUpdation = true;
	}

	if (m_UseSeedFromActiveMesh)
	{
		if (ImGui::CollapsingHeader("Seed Texture Settings"))
		{
			ImGui::BeginChild("Seed Texture Settings", ImVec2(0, 250), true, ImGuiWindowFlags_AlwaysUseWindowPadding);
			ImGui::PushID("Seed Texture Settings");
			if (ImGui::Button("Pull From Active Mesh"))
			{
				PullSeedTextureFromActiveMesh();
				m_RequireUpdation = true;
			}
			if (PowerOfTwoDropDown("Resolution", &m_SeedTextureResolution, 2, 20))
			{
				m_SeedTexture->Resize(m_SeedTextureResolution, m_SeedTextureResolution);
				m_RequireUpdation = true;
			}
			ImGui::Image(m_SeedTexture->GetTextureID(), ImVec2(200, 200));
			ImGui::PopID();
			ImGui::EndChild();
		}
	}
}


bool GenerationManager::OnTileResolutionChange(const std::string params, void* paramsPtr)
{
	auto size = m_AppState->mainMap.tileResolution * m_AppState->mainMap.tileResolution * sizeof(float);
	m_HeightmapData->Resize(size);
	m_SwapBuffer->Resize(size);
	m_RequireUpdation = true;
	for (auto biome : m_BiomeManagers)
	{
		biome->Resize();
	}
	return false;
}

