#include "Generators/GenerationManager.h"
#include "Data/ApplicationState.h"
#include "Data/ConfigManager.h"
#include "Base/ComputeShader.h"
#include "UI/ImGuiComponents.h"
#include "Utils/Utils.h"
#include "Profiler.h"
#include <map>

GenerationManager::GenerationManager(ApplicationState* appState)
{
	// if (!BiomeManager::LoadBaseShapeGenerators(appState)) Log("Failed to load Base Shape Generators!");
	m_AppState = appState;
	std::string configuredStorage;
	if (m_AppState->configManager != nullptr && m_AppState->configManager->GetString("generation", "field_storage", configuredStorage))
	{
		GeneratorData::SetDefaultStorage(configuredStorage == "R16F" ? GeneratorDataStorage::R16F : GeneratorDataStorage::R32F);
	}
	m_Ui.fieldStorageUiMode = GeneratorData::GetDefaultStorage() == GeneratorDataStorage::R16F ? 1 : 0;
	m_Field.statistics = std::make_shared<GeneratorDataStatistics>(m_AppState);
	m_Field.heightPyramid = std::make_shared<HeightfieldPyramid>(m_AppState);
	m_AppState->eventManager->Subscribe("TileResolutionChanged", BIND_EVENT_FN(OnTileResolutionChange));
	m_AppState->eventManager->Subscribe("ForceUpdate", BIND_EVENT_FN(UpdateInternal));
	m_Field.heightmapData = std::make_shared<GeneratorData>();
	m_Field.workingHeightmapData = std::make_shared<GeneratorData>();
	m_Field.swapBuffer = std::make_shared<GeneratorData>();
	m_Field.slopeGenerator = std::make_shared<SlopeGenerator>(m_AppState, m_AppState->mainMap.tileResolution);
	m_Field.biomeMixer = std::make_shared<BiomeMixer>(m_AppState);
	m_Field.biomeManagers.push_back(std::make_shared<BiomeManager>(m_AppState));
	m_Field.biomeManagers.back()->SetName("Default Global");
	m_Worker = std::make_unique<GenerationWorker>([this](bool force) { ExecuteGeneration(force); });
}

GenerationManager::~GenerationManager() = default;

void GenerationManager::Update()
{
	if (m_Ui.updationPaused) return;
	if (!m_Worker->HasContext())
	{
		// TF3D_LOG_DEBUG("GenerationManager::Update() - Running generation on render thread as no shared OpenGL context is available");
		if (m_Ui.requireUpdation)
		{
			m_Ui.requireUpdation = false;
			ExecuteGeneration(true);
			CommitHeightfield();
		}
		return;
	}

	if (m_Worker->IsCompleted() && !m_Worker->IsRunning())
	{
		if (m_Worker->ConsumeCompleted())
		{
			CommitHeightfield();
		}
	}

	if (m_Ui.requireUpdation.load(std::memory_order_acquire) &&
		!m_Worker->IsRunning() && !m_Worker->IsRequestPending())
	{
		RequestGeneration(true);
	}
}

bool GenerationManager::UpdateInternal(const std::string& params, void* paramsPtr)
{
	RequestGeneration(params == "ForceUpdate");
	return false;
}

void GenerationManager::RequestGeneration(bool force)
{
	if (!m_Worker->Request(force))
	{
		m_Ui.requireUpdation = false;
		ExecuteGeneration(force);
		return;
	}
	m_Ui.requireUpdation = false;
}

void GenerationManager::WaitForGenerationWorker()
{
	m_Worker->WaitForIdle();
}

void GenerationManager::ExecuteGeneration(bool forceUpdate)
{
	auto hasAnythingUpdated = false;
	for (auto biome : m_Field.biomeManagers)
	{
		if (biome->IsUpdationRequired() || forceUpdate)
		{
			biome->Update(m_Field.swapBuffer.get(), m_Field.seedTexture.get());
			hasAnythingUpdated = true;
		}
	}
	if (hasAnythingUpdated || m_Field.biomeMixer->IsUpdationRequired() || forceUpdate)
	{
		m_Field.biomeMixer->Update(m_Field.workingHeightmapData.get(), m_Field.swapBuffer.get());
		m_Field.slopeGenerator->Compute(m_Field.workingHeightmapData.get(), m_Field.workingHeightmapData->GetResolution());
	}
}

void GenerationManager::PullSeedTextureFromActiveMesh()
{
	if (!m_Ui.useSeedFromActiveMesh) return;
	m_Field.seedTexture->MakeCPUCopy();
	m_Field.seedTexture->ZeroCPUCopy();
	auto mesh = m_AppState->mainModel->mesh;
	for (auto i = 0; i < mesh->GetVertexCount(); i++)
	{
		const auto& vertex = mesh->GetVertex(i);
		m_Field.seedTexture->SetPixel(vertex.texCoord.x, vertex.texCoord.y, vertex.position.x, vertex.position.z, vertex.position.y);
	}
	m_Field.seedTexture->UploadCPUCopy();
	m_Field.seedTexture->FreeCPUCopy();
}

void GenerationManager::ShowSettings()
{
	ShowSettingsInspector();
	ShowSettingsDetailed();
}

void GenerationManager::ShowSettingsInspector()
{
	static bool s_TempBoolean = false;
	ImGui::Begin("Generator Inspector", &m_Ui.windowVisible);

	if (ImGui::Selectable("Options", m_Ui.selectedNode.m_ID == "GlobalOptions"))
	{
		m_Ui.selectedNode.m_ID = "GlobalOptions";
		m_Ui.selectedNode.m_ObjectName = SelectedUINodeObjectType_GlobalOptions;
	}

	if (ImGui::Selectable("Biome Mixer", m_Ui.selectedNode.m_ID == "GlobalBiomeMixer"))
	{
		m_Ui.selectedNode.m_ID = "GlobalBiomeMixer";
		m_Ui.selectedNode.m_ObjectName = SelectedUINodeObjectType_GlobalBiomeMixer;
	}

	s_TempBoolean = ImGui::TreeNodeEx("Biomes", ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_AllowItemOverlap);
	ImGui::SameLine();
	if (ImGui::Button("Add##BiomeAdd"))
	{
		m_Field.biomeManagers.push_back(std::make_shared<BiomeManager>(m_AppState));
		m_Field.biomeManagers.back()->SetName("Biome " + std::to_string(m_Field.biomeManagers.size()));
	}
	if (s_TempBoolean)
	{
		if (m_Field.biomeManagers.size() == 0) ImGui::Text("No Biomes Added!");
		for (int i = 0; i < m_Field.biomeManagers.size(); i++)
		{
			auto biome = m_Field.biomeManagers[i];
			ImGui::PushID(biome->GetBiomeID().c_str());
			s_TempBoolean = ImGui::TreeNodeEx(biome->GetBiomeName(), ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_AllowItemOverlap);
			ImGui::SameLine();
			if (ImGui::Button("Delete"))
			{
				TF3D_LOG_DEBUG("Loaded {} biome managers", m_Field.biomeManagers.size());
				m_Field.biomeManagers.erase(m_Field.biomeManagers.begin() + i);
				TF3D_LOG_DEBUG("Active biome managers: {}", m_Field.biomeManagers.size());
				m_Ui.requireUpdation = true;
				SetUINodeData(-1, None);
			}
			if (s_TempBoolean)
			{
				if (ImGui::Selectable("General", m_Ui.selectedNode.m_ID == MakeUINodeID(i, General)))
				{
					SetUINodeData(i, General);
				}
				if (ImGui::Selectable("Mask", m_Ui.selectedNode.m_ID == MakeUINodeID(i, MaskTool)))
				{
					SetUINodeData(i, MaskTool);
				}
				if (ImGui::Selectable("Base Shape", m_Ui.selectedNode.m_ID == MakeUINodeID(i, BaseShape)))
				{
					SetUINodeData(i, BaseShape);
				}
				if (ImGui::Selectable("Custom Base Shape", m_Ui.selectedNode.m_ID == MakeUINodeID(i, CustomBaseShape)))
				{
					SetUINodeData(i, CustomBaseShape);
				}
				if (ImGui::Selectable("Base Noise", m_Ui.selectedNode.m_ID == MakeUINodeID(i, BaseNoise)))
				{
					SetUINodeData(i, BaseNoise);
				}
				s_TempBoolean = ImGui::TreeNodeEx("Filters", ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_AllowItemOverlap);
				ImGui::SameLine();
				const std::string addFilterPopupID = "Add Filter##" + biome->GetBiomeID();
				static char filterSearch[128] = {};
				if (ImGui::Button("Add##BiomeFilterAdd"))
				{
					filterSearch[0] = '\0';
					ImGui::OpenPopup(addFilterPopupID.c_str());
				}
				if (ImGui::BeginPopup(addFilterPopupID.c_str()))
				{
					ImGui::SetNextItemWidth(240.0f);
					ImGui::InputText("Search filters", filterSearch, IM_ARRAYSIZE(filterSearch));
					ImGui::Separator();

					std::map<std::string, std::vector<int>> visibleFiltersByCategory;
					const auto& definitions = biome->GetFilterDefinitions();
					for (int definitionIndex = 0; definitionIndex < static_cast<int>(definitions.size()); definitionIndex++)
					{
						if (FuzzyFilterMatch(filterSearch, definitions[definitionIndex]->GetSearchText()))
							visibleFiltersByCategory[definitions[definitionIndex]->GetCategory()].push_back(definitionIndex);
					}

					if (visibleFiltersByCategory.empty()) ImGui::TextDisabled("No filters match the search.");
					for (const auto& [category, categoryDefinitions] : visibleFiltersByCategory)
					{
						const std::string categoryLabel = category + "##FilterCategory";
						const bool categoryOpen = ImGui::TreeNodeEx(categoryLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth);
						if (categoryOpen)
						{
							for (const int definitionIndex : categoryDefinitions)
							{
								const auto& definition = definitions[definitionIndex];
								const std::string filterLabel = definition->GetName() + "##" + definition->GetID();
								if (ImGui::Selectable(filterLabel.c_str()))
								{
									const int filterIndex = biome->AddFilter(definition);
									if (filterIndex >= 0)
									{
										m_Ui.selectedNode.m_BiomeIndex = i;
										m_Ui.selectedNode.m_FilterIndex = filterIndex;
										m_Ui.selectedNode.m_BiomeID = biome->GetBiomeID();
										m_Ui.selectedNode.m_ID = biome->GetFilters()[filterIndex]->GetID();
										m_Ui.selectedNode.m_ObjectName = SelectedUINodeObjectType_Filter;
										m_Ui.requireUpdation = true;
								}
									ImGui::CloseCurrentPopup();
								}
							}
							ImGui::TreePop();
						}
					}
					ImGui::EndPopup();
				}
				if (s_TempBoolean)
				{
					const auto& filters = biome->GetFilters();
					if (filters.size() == 0) ImGui::Text("No Filters Added!");
					bool filterWasRemoved = false;
					if (filters.size() > 0 && ImGui::BeginTable("##FilterList", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings))
					{
						ImGui::TableSetupColumn("Filter", ImGuiTableColumnFlags_WidthStretch);
						ImGui::TableSetupColumn("##Delete", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFrameHeight());

						for (int filterIndex = 0; filterIndex < static_cast<int>(filters.size()); filterIndex++)
						{
							const auto& filter = filters[filterIndex];
							ImGui::PushID(filter->GetID().c_str());
							const bool selected = m_Ui.selectedNode.m_ObjectName == SelectedUINodeObjectType_Filter &&
								m_Ui.selectedNode.m_BiomeIndex == i && m_Ui.selectedNode.m_FilterIndex == filterIndex;

							ImGui::TableNextRow();
							ImGui::TableSetColumnIndex(0);
							if (ImGui::Selectable(filter->GetName().c_str(), selected))
							{
								m_Ui.selectedNode.m_BiomeIndex = i;
								m_Ui.selectedNode.m_FilterIndex = filterIndex;
								m_Ui.selectedNode.m_BiomeID = biome->GetBiomeID();
								m_Ui.selectedNode.m_ID = filter->GetID();
								m_Ui.selectedNode.m_ObjectName = SelectedUINodeObjectType_Filter;
							}

							ImGui::TableSetColumnIndex(1);
							const bool deleteFilter = ImGui::Button("X", ImVec2(-1.0f, 0.0f));
							if (ImGui::IsItemHovered()) ImGui::SetTooltip("Delete filter");
							if (deleteFilter)
							{
								if (biome->RemoveFilter(filterIndex))
								{
									if (m_Ui.selectedNode.m_ObjectName == SelectedUINodeObjectType_Filter &&
										m_Ui.selectedNode.m_BiomeIndex == i &&
										m_Ui.selectedNode.m_FilterIndex == filterIndex)
									{
										SetUINodeData(i, General);
										m_Ui.selectedNode.m_BiomeID = biome->GetBiomeID();
									}
									else if (m_Ui.selectedNode.m_ObjectName == SelectedUINodeObjectType_Filter &&
										m_Ui.selectedNode.m_BiomeIndex == i &&
										m_Ui.selectedNode.m_FilterIndex > filterIndex)
									{
										m_Ui.selectedNode.m_FilterIndex--;
									}
									m_Ui.requireUpdation = true;
									filterWasRemoved = true;
								}
							}
							ImGui::PopID();
							if (filterWasRemoved) break;
						}
						ImGui::EndTable();
					}
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
	ImGui::Begin("Generation Settings", &m_Ui.windowVisible);

	if (m_Ui.selectedNode.m_ObjectName == SelectedUINodeObjectType_GlobalOptions) ShowSettingsGlobalOptions();
	else if (m_Ui.selectedNode.m_ObjectName == SelectedUINodeObjectType_GlobalBiomeMixer) m_Ui.requireUpdation = m_Field.biomeMixer->ShowSettings() || m_Ui.requireUpdation;
	else if (m_Ui.selectedNode.m_ObjectName == SelectedUINodeObjectType_General) m_Ui.requireUpdation = m_Field.biomeManagers[m_Ui.selectedNode.m_BiomeIndex]->ShowGeneralSettings() || m_Ui.requireUpdation;
	else if (m_Ui.selectedNode.m_ObjectName == SelectedUINodeObjectType_BaseShape) m_Ui.requireUpdation = m_Field.biomeManagers[m_Ui.selectedNode.m_BiomeIndex]->ShowBaseShapeSettings() || m_Ui.requireUpdation;
	else if (m_Ui.selectedNode.m_ObjectName == SelectedUINodeObjectType_CustomBaseShape) m_Ui.requireUpdation = m_Field.biomeManagers[m_Ui.selectedNode.m_BiomeIndex]->ShowCustomBaseShapeSettings() || m_Ui.requireUpdation;
	else if (m_Ui.selectedNode.m_ObjectName == SelectedUINodeObjectType_BaseNoise) m_Ui.requireUpdation = m_Field.biomeManagers[m_Ui.selectedNode.m_BiomeIndex]->ShowBaseNoiseSettings() || m_Ui.requireUpdation;
	else if (m_Ui.selectedNode.m_ObjectName == SelectedUINodeObjectType_MaskTool) m_Ui.requireUpdation = m_Field.biomeManagers[m_Ui.selectedNode.m_BiomeIndex]->ShowMaskToolSettings() || m_Ui.requireUpdation;
	else if (m_Ui.selectedNode.m_ObjectName == SelectedUINodeObjectType_Filter &&
		m_Ui.selectedNode.m_BiomeIndex >= 0 && m_Ui.selectedNode.m_BiomeIndex < static_cast<int>(m_Field.biomeManagers.size()))
		m_Ui.requireUpdation = m_Field.biomeManagers[m_Ui.selectedNode.m_BiomeIndex]->ShowFilterSettings(m_Ui.selectedNode.m_FilterIndex) || m_Ui.requireUpdation;

	ImGui::End();
}

void GenerationManager::ShowSettingsGlobalOptions()
{
	int storageMode = m_Ui.fieldStorageUiMode;
	const char* storageLabels[] = { "R32F (32-bit float)", "R16F (16-bit float)" };
	if (ImGui::Combo("Field Storage", &storageMode, storageLabels, IM_ARRAYSIZE(storageLabels)))
	{
		m_Ui.fieldStorageUiMode = storageMode;
		m_Ui.fieldStorageRestartPending = true;
		if (m_AppState->configManager != nullptr)
			m_AppState->configManager->SetString("generation", "field_storage", storageMode == 1 ? "R16F" : "R32F");
	}
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Controls the precision of generated field textures.");
	if (m_Ui.fieldStorageRestartPending) {
		ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f), "Restart required to apply storage change.");
	}
	ShowFieldStatistics();

	ImGui::Checkbox("Use Seed Texture", &m_Ui.useSeedFromActiveMesh);
	ImGui::Checkbox("Auto Updation Paused", &m_Ui.updationPaused);

	if (m_Ui.useSeedFromActiveMesh && m_Field.seedTexture == nullptr)
	{
		m_Field.seedTexture = std::make_shared<GeneratorTexture>(m_Ui.seedTextureResolution, m_Ui.seedTextureResolution);
		m_Ui.requireUpdation = true;
	}
	else if (!m_Ui.useSeedFromActiveMesh && m_Field.seedTexture != nullptr)
	{
		m_Field.seedTexture = nullptr;
		m_Ui.requireUpdation = true;
	}

	if (m_Ui.useSeedFromActiveMesh)
	{
		if (ImGui::CollapsingHeader("Seed Texture Settings"))
		{
			ImGui::BeginChild("Seed Texture Settings", ImVec2(0, 250), true, ImGuiWindowFlags_AlwaysUseWindowPadding);
			ImGui::PushID("Seed Texture Settings");
			if (ImGui::Button("Pull From Active Mesh"))
			{
				PullSeedTextureFromActiveMesh();
				m_Ui.requireUpdation = true;
			}
			if (PowerOfTwoDropDown("Resolution", &m_Ui.seedTextureResolution, 2, 20))
			{
				m_Field.seedTexture->Resize(m_Ui.seedTextureResolution, m_Ui.seedTextureResolution);
				m_Ui.requireUpdation = true;
			}
			ImGui::Image(m_Field.seedTexture->GetTextureID(), ImVec2(200, 200));
			ImGui::PopID();
			ImGui::EndChild();
		}
	}
}

void GenerationManager::UpdateFieldStatistics()
{
	if (m_Field.statistics == nullptr || m_Field.heightmapData == nullptr) return;
	m_Field.statistics->Compute(m_Field.heightmapData.get(), m_AppState->mainMap.tileResolution, m_Field.statisticsSampleStride);
	glFinish();
	m_Field.statisticsResult = m_Field.statistics->Read();
}

void GenerationManager::CommitHeightfield()
{
	m_Field.heightmapData.swap(m_Field.workingHeightmapData);
	m_TerrainRevision.fetch_add(1, std::memory_order_release);
	GenerateHeightmapMipmaps();
	if (m_Field.heightPyramid != nullptr)
	{
		m_Field.heightPyramid->Rebuild(m_Field.heightmapData.get());
	}
	UpdateFieldStatistics();
}

void GenerationManager::GenerateHeightmapMipmaps()
{
	if (m_Field.heightmapData == nullptr || m_Field.heightmapData->GetResolution() <= 0) return;

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
	m_Field.heightmapData->BindAsTexture(0);
	int32_t mipLevels = 1;
	for (int32_t mipSize = m_Field.heightmapData->GetResolution(); mipSize > 1; mipSize >>= 1) ++mipLevels;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipLevels - 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void GenerationManager::ShowFieldStatistics()
{
	ImGui::Separator();
	ImGui::TextUnformatted("Final Field Statistics");
	if (!m_Field.statisticsResult.valid)
	{
		ImGui::TextDisabled("No completed statistics yet.");
		return;
	}

	ImGui::Text("Minimum: %.6f", m_Field.statisticsResult.minimum);
	ImGui::Text("Maximum: %.6f", m_Field.statisticsResult.maximum);
	ImGui::TextDisabled("Histogram sampled every %d pixels", m_Field.statisticsSampleStride);
	ImGui::PlotLines("##FinalFieldHistogram", m_Field.statisticsResult.histogram.data(),
		static_cast<int>(m_Field.statisticsResult.histogram.size()), 0, "Height distribution", 0.0f, 1.0f,
		ImVec2(-1.0f, 120.0f));
}


bool GenerationManager::OnTileResolutionChange(const std::string params, void* paramsPtr)
{
	WaitForGenerationWorker();
	m_Worker->ConsumeCompleted();
	auto size = m_AppState->mainMap.tileResolution * m_AppState->mainMap.tileResolution * sizeof(float);
	m_Field.heightmapData->Resize(size);
	m_Field.workingHeightmapData->Resize(size);
	m_Field.swapBuffer->Resize(size);
	m_Field.slopeGenerator->Resize(m_AppState->mainMap.tileResolution);
	m_Ui.requireUpdation = true;
	for (auto biome : m_Field.biomeManagers)
	{
		biome->Resize();
	}
	return false;
}

