#include "Generators/GenerationManager.h"
#include "Data/ApplicationState.h"
#include "Base/ComputeShader.h"
#include "Utils/Utils.h"
#include "Profiler.h"

static float a, b, c, d, e;
static bool ch = false;

GenerationManager::GenerationManager(ApplicationState* appState)
{
	m_AppState = appState;
	a = 1.0f;
	b = 1.0f;
	c = 1.0f;
	m_AppState->eventManager->Subscribe("TileResolutionChanged", BIND_EVENT_FN(OnTileResolutionChange));
	m_AppState->eventManager->Subscribe("ForceUpdate", BIND_EVENT_FN(UpdateInternal));
	m_HeightmapData = new GeneratorData();
	m_SwapBuffer = new GeneratorData();
	m_BiomeManagers.push_back(new BiomeManager(m_AppState));
	
	auto source = ReadShaderSourceFile(m_AppState->constants.shadersDir + PATH_SEPARATOR "texture" PATH_SEPARATOR "blurr.glsl", &s_TempBool);
	if (!s_TempBool)
	{
		Log("Failed to load shader source file: %s" + m_AppState->constants.shadersDir + PATH_SEPARATOR "texture" PATH_SEPARATOR "blurr.glsl");
		return;
	}
	//m_BlurrShader = new ComputeShader(source);
}

GenerationManager::~GenerationManager()
{
	delete m_HeightmapData;
	delete m_SwapBuffer;
	//delete m_BlurrShader;
	if (m_SeedTexture) delete m_SeedTexture;
	for (auto biome : m_BiomeManagers)
	{
		delete biome;
	}
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
			biome->Update(m_SwapBuffer, m_SeedTexture);
		}
	}
	if (m_SelectedBiome != -1) m_BiomeManagers[m_SelectedBiome]->GetBiomeData()->CopyTo(m_HeightmapData);
	m_RequireUpdation = false;
	return false;
}

void GenerationManager::PullSeedTextureFromActiveMesh()
{
	if(!m_UseSeedFromActiveMesh) return;
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
	ImGui::Begin("Generation Manager", &m_IsWindowVisible);
	ImGui::Checkbox("Use Seed Texture", &m_UseSeedFromActiveMesh);
	ImGui::Checkbox("Auto Updation Paused", &m_UpdationPaused);
	
	if (m_UseSeedFromActiveMesh && m_SeedTexture == nullptr)
	{
		m_SeedTexture = new GeneratorTexture(m_SeedTextureResolution, m_SeedTextureResolution);
		m_RequireUpdation = true;
	}
	else if (!m_UseSeedFromActiveMesh && m_SeedTexture != nullptr)
	{
		delete m_SeedTexture;
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
	ImGui::Separator();
	ImGui::PushFont(GetUIFont("OpenSans-Bold"));
	ImGui::Text("Biomes");
	ImGui::PopFont();
	for (int i = 0; i < m_BiomeManagers.size(); i++)
	{
		auto biome = m_BiomeManagers[i];
		if (ImGui::Selectable(biome->GetBiomeName(), m_SelectedBiome == i))
		{
			m_SelectedBiome = i;
			m_AppState->eventManager->RaiseEvent("ForceUpdate", "ForceUpdate");
		}
	}

	ImGui::NewLine();
	if (m_SelectedBiome != -1)
	{
		ImGui::PushFont(GetUIFont("OpenSans-Bold"));
		ImGui::Text(m_BiomeManagers[m_SelectedBiome]->GetBiomeName());
		ImGui::PopFont();
		m_RequireUpdation = m_BiomeManagers[m_SelectedBiome]->ShowSettings() || m_RequireUpdation;
	}

	ImGui::End();
}

void GenerationManager::InvalidateWorkInProgress()
{
}

bool GenerationManager::IsWorking()
{
	return false;
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

