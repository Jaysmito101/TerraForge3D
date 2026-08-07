#include "Renderer/RendererManager.h"
#include "Data/ApplicationState.h"
#include "Profiler.h"

namespace tf3d::renderer
{
    RendererManager::RendererManager(ApplicationState *appState)
    {
        m_AppState = appState;
        m_TerrainInspector.LoadConfig(appState, "Terrain");
        m_ObjectRenderer      = std::make_shared<ObjectRenderer>(appState);
        m_HeightmapRenderer   = std::make_shared<HeightmapRenderer>(appState);
        m_TextureSlotRenderer = std::make_shared<TextureSlotRenderer>(appState);
        m_WireframeRenderer   = std::make_shared<WireframeRenderer>(appState);

        m_RendererLights          = std::make_shared<RendererLights>(appState);
        m_RendererSky             = std::make_shared<RendererSky>(appState);
        m_TerrainSelfShadow       = std::make_shared<TerrainSelfShadow>(appState);
        m_PlanarShadowCache       = std::make_shared<PlanarShadowCache>(appState);
        m_HeightfieldAmbientCache = std::make_shared<HeightfieldAmbientCache>(appState);
        m_HeightfieldGICache      = std::make_shared<HeightfieldGICache>(appState);
        m_SeaRenderer             = std::make_shared<SeaRenderer>(appState);
    }

    RendererManager::~RendererManager()
    {
    }

    bool RendererManager::IsWindowVisible() const
    {
        return m_AppState != nullptr && m_AppState->windows.rendererSettings;
    }

    bool *RendererManager::IsWindowVisiblePtr()
    {
        return m_AppState != nullptr ? &m_AppState->windows.rendererSettings : nullptr;
    }

    void RendererManager::Update()
    {
        TF3D_PROFILE_SCOPE_DOMAIN("renderer/lighting-caches", PerformanceMonitor::Domain::Renderer);
        UpdateTerrainSelfShadowCache();
        UpdatePlanarShadowCache();
        UpdateHeightfieldAmbientCache();
        UpdateHeightfieldGICache();
    }

    void RendererManager::UpdateTerrainSelfShadowCache()
    {
        TF3D_PROFILE_SCOPE_DOMAIN("renderer/cache/terrain-self-shadow", PerformanceMonitor::Domain::Renderer);
        if (m_TerrainSelfShadow == nullptr || m_AppState == nullptr ||
            m_AppState->generationManager == nullptr || m_RendererLights == nullptr) {
            return;
        }

        m_TerrainSelfShadow->Update(
            m_AppState->generationManager->GetHeightmapData(),
            m_AppState->generationManager->GetHeightPyramid(),
            m_AppState->generationManager->GetTerrainRevision(),
            m_RendererLights->GetSunDirection(),
            std::max(std::abs(m_AppState->mainMap.tileSize) * 2.0f, 0.0001f));
    }

    void RendererManager::UpdatePlanarShadowCache()
    {
        TF3D_PROFILE_SCOPE_DOMAIN("renderer/cache/planar-shadow", PerformanceMonitor::Domain::Renderer);
        if (m_PlanarShadowCache == nullptr || m_AppState == nullptr ||
            m_AppState->generationManager == nullptr || m_RendererLights == nullptr) {
            return;
        }

        const bool isPlane              = m_AppState->mainModel != nullptr && m_AppState->mainModel->isGeneratedPlane;
        const auto &fieldStatistics     = m_AppState->generationManager->GetFieldStatisticsResult();
        const float fieldMinimum        = fieldStatistics.valid ? fieldStatistics.minimum : 0.0f;
        const float fieldMaximum        = fieldStatistics.valid ? fieldStatistics.maximum : 0.0f;
        const float solidDepth          = isPlane ? std::max(m_AppState->mainModel->planeSolidDepth, 0.0001f) : 0.0f;
        const float terrainWorldSize    = std::max(std::abs(m_AppState->mainMap.tileSize) * 2.0f, 0.0001f);
        const float terrainHeightOffset = isPlane ? -fieldMinimum + solidDepth : 0.0f;
        m_PlanarShadowCache->Update(
            isPlane ? m_AppState->generationManager->GetHeightPyramid() : nullptr,
            m_AppState->generationManager->GetTerrainRevision(),
            m_RendererLights->GetSunDirection(),
            glm::vec2(-terrainWorldSize * 0.5f),
            terrainWorldSize,
            terrainHeightOffset,
            fieldMaximum,
            0.0f);
    }

    void RendererManager::UpdateHeightfieldAmbientCache()
    {
        TF3D_PROFILE_SCOPE_DOMAIN("renderer/cache/heightfield-ambient", PerformanceMonitor::Domain::Renderer);
        if (m_HeightfieldAmbientCache == nullptr || m_AppState == nullptr ||
            m_AppState->generationManager == nullptr) {
            return;
        }

        const bool enableAmbientAo        = m_TerrainInspector.Get<bool>("TerrainAOEnabled", true);
        const float ambientAoRadiusFactor = m_TerrainInspector.Get<float>("TerrainAORadiusFactor", 0.12f);
        m_HeightfieldAmbientCache->SetEnabled(enableAmbientAo);
        const float terrainWorldSize = std::max(std::abs(m_AppState->mainMap.tileSize) * 2.0f, 0.0001f);
        if (!enableAmbientAo) {
            return;
        }

        m_HeightfieldAmbientCache->Update(
            m_AppState->generationManager->GetHeightPyramid(),
            m_AppState->generationManager->GetTerrainRevision(),
            terrainWorldSize,
            terrainWorldSize * ambientAoRadiusFactor);
    }

    void RendererManager::UpdateHeightfieldGICache()
    {
        TF3D_PROFILE_SCOPE_DOMAIN("renderer/cache/heightfield-gi", PerformanceMonitor::Domain::Renderer);
        if (m_HeightfieldGICache == nullptr || m_AppState == nullptr ||
            m_AppState->generationManager == nullptr || m_RendererLights == nullptr ||
            m_RendererSky == nullptr) {
            return;
        }

        const bool enableTerrainGI                = m_TerrainInspector.Get<bool>("TerrainGIEnabled", false);
        const int32_t terrainGIResolution         = m_TerrainInspector.Get<int32_t>("TerrainGIResolution", 128);
        const int32_t terrainGITargetSamples      = m_TerrainInspector.Get<int32_t>("TerrainGITargetSamples", 32);
        const int32_t terrainGISamplesPerDispatch = m_TerrainInspector.Get<int32_t>("TerrainGISamplesPerDispatch", 1);
        m_HeightfieldGICache->SetEnabled(enableTerrainGI);
        if (!enableTerrainGI) {
            return;
        }

        const bool hasSkyLight          = m_RendererLights->IsSkyLightEnabled() && m_RendererSky->IsSkyReady();
        const bool hasTerrainSelfShadow = m_TerrainSelfShadow != nullptr && m_TerrainSelfShadow->IsReady();
        m_HeightfieldGICache->Update(
            m_AppState->generationManager->GetHeightPyramid(),
            m_AppState->generationManager->GetTerrainRevision(),
            std::max(std::abs(m_AppState->mainMap.tileSize) * 2.0f, 0.0001f),
            m_RendererLights->GetSunDirection(),
            m_RendererLights->GetSunColor(),
            m_RendererLights->GetSunIntensity(),
            hasSkyLight,
            m_RendererLights->GetSkyLightIntensity(),
            hasSkyLight ? m_RendererSky->GetSkyboxMap() : -1,
            hasSkyLight ? m_RendererSky->GetIrradianceMap() : -1,
            hasTerrainSelfShadow,
            hasTerrainSelfShadow ? static_cast<int32_t>(m_TerrainSelfShadow->GetRendererID()) : -1,
            terrainGIResolution,
            terrainGITargetSamples,
            terrainGISamplesPerDispatch);
    }

    void RendererManager::Render(RendererViewport *viewport)
    {
        TF3D_PROFILE_SCOPE_DOMAIN("renderer/viewport", PerformanceMonitor::Domain::Renderer);
        TF3D_PROFILE_VALUE_DOMAIN("renderer/viewport/size", viewport->GetFrameBuffer()->GetWidth(),
                                  viewport->GetFrameBuffer()->GetHeight(), static_cast<uint64_t>(viewport->GetMode()),
                                  PerformanceMonitor::Domain::Renderer);
        {
            TF3D_PROFILE_SCOPE("renderer/setup");
            glBindFramebuffer(GL_FRAMEBUFFER, viewport->GetFrameBuffer()->GetRendererID());
            glViewport(0, 0, viewport->GetFrameBuffer()->GetWidth(), viewport->GetFrameBuffer()->GetHeight());
            glEnable(GL_MULTISAMPLE);
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            viewport->GetCamera().UpdateCamera();
        }

        {
            TF3D_PROFILE_SCOPE_DOMAIN("renderer/sky", PerformanceMonitor::Domain::Renderer);
            TF3D_PROFILE_GPU_SCOPE("renderer/sky/gpu");
            m_RendererSky->Render(viewport);
        }

        {
            TF3D_PROFILE_SCOPE("renderer/scene");
            auto &positionOnTerrain = viewport->GetPositionOnTerrain();
            positionOnTerrain[0] = positionOnTerrain[1] = positionOnTerrain[2] = -1.0f;
            switch (viewport->GetMode()) {
                case RendererViewportMode::Object: {
                    TF3D_PROFILE_SCOPE_DOMAIN("renderer/scene/object", PerformanceMonitor::Domain::Renderer);
                    TF3D_PROFILE_GPU_SCOPE("renderer/scene/object/gpu");
                    m_ObjectRenderer->Render(viewport);
                    if (m_SeaRenderer != nullptr && m_SeaRenderer->IsEnabled()) {
                        viewport->GetFrameBuffer()->ResolveColor();
                        viewport->GetFrameBuffer()->ResolveDepth();
                        m_SeaRenderer->Render(viewport);
                    }
                } break;
                case RendererViewportMode::Wireframe: {
                    TF3D_PROFILE_SCOPE_DOMAIN("renderer/scene/wireframe", PerformanceMonitor::Domain::Renderer);
                    TF3D_PROFILE_GPU_SCOPE("renderer/scene/wireframe/gpu");
                    m_WireframeRenderer->Render(viewport);
                } break;
                case RendererViewportMode::Heightmap: {
                    TF3D_PROFILE_SCOPE_DOMAIN("renderer/scene/heightmap", PerformanceMonitor::Domain::Renderer);
                    TF3D_PROFILE_GPU_SCOPE("renderer/scene/heightmap/gpu");
                    m_HeightmapRenderer->Render(viewport);
                } break;
                case RendererViewportMode::TextureSlot: {
                    TF3D_PROFILE_SCOPE_DOMAIN("renderer/scene/texture-slot", PerformanceMonitor::Domain::Renderer);
                    TF3D_PROFILE_GPU_SCOPE("renderer/scene/texture-slot/gpu");
                    m_TextureSlotRenderer->Render(viewport);
                } break;
                default:
                    break;
            }
        }

        {
            TF3D_PROFILE_SCOPE_DOMAIN("renderer/resolve", PerformanceMonitor::Domain::Renderer);
            TF3D_PROFILE_GPU_SCOPE("renderer/resolve/gpu");
            TF3D_PROFILE_COUNTER_DOMAIN("gpu/framebuffer-resolves", 1.0, PerformanceMonitor::Domain::Gpu);
            viewport->GetFrameBuffer()->Resolve();
        }
    }

    void RendererManager::ShowSettings()
    {
        if (m_AppState != nullptr && m_AppState->windows.rendererSettings) {
            ImGui::Begin("Renderer Settings", &m_AppState->windows.rendererSettings);
            ImGui::PushID("Renderer Settings");
            if (ImGui::CollapsingHeader("Core Settings")) {
                if (ImGui::BeginTabBar("Core Settings Type")) {
                    if (ImGui::BeginTabItem("Object")) {
                        ImGui::PushID("Core Settings Type->Object");
                        m_ObjectRenderer->ShowSettings();
                        ImGui::PopID();
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("Wireframe")) {
                        ImGui::PushID("Core Settings Type->Wireframe");
                        m_WireframeRenderer->ShowSettings();
                        ImGui::PopID();
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("Heightmap")) {
                        ImGui::PushID("Core Settings Type->Heightmap");
                        m_HeightmapRenderer->ShowSettings();
                        ImGui::PopID();
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("Texture Slot")) {
                        ImGui::PushID("Core Settings Type->Texture Slot");
                        m_TextureSlotRenderer->ShowSettings();
                        ImGui::PopID();
                        ImGui::EndTabItem();
                    }
                    ImGui::EndTabBar();
                }
            }
            ImGui::PopID();
            ImGui::Separator();
            ImGui::PushID("Items Settings");
            if (ImGui::CollapsingHeader("Items")) {
                if (ImGui::BeginTabBar("Items Settings")) {
                    if (ImGui::BeginTabItem("Terrain")) {
                        ImGui::PushID("Terrain");
                        m_TerrainInspector.Render();
                        if (m_HeightfieldGICache != nullptr) {
                            ImGui::Text("GI Progress: %.1f%% (%d / %d)",
                                        m_HeightfieldGICache->GetProgress() * 100.0f,
                                        m_HeightfieldGICache->GetAccumulatedSamples(),
                                        m_HeightfieldGICache->GetTargetSamples());
                        }
                        ImGui::PopID();
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("Lights")) {
                        ImGui::PushID("Lights");
                        m_RendererLights->ShowSettings();
                        ImGui::PopID();
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("Sky")) {
                        ImGui::PushID("Sky");
                        m_RendererSky->ShowSettings();
                        ImGui::PopID();
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("Sea")) {
                        ImGui::PushID("Sea");
                        if (m_SeaRenderer != nullptr)
                            m_SeaRenderer->ShowSettings();
                        ImGui::PopID();
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("Objects")) {
                        ImGui::PushID("Objects");
                        ImGui::PopID();
                        ImGui::EndTabItem();
                    }
                    ImGui::EndTabBar();
                }
            }
            ImGui::PopID();
            ImGui::Separator();
            ImGui::End();
        }
    }

    exporters::SerializerNode RendererManager::SaveTerrainSettings() const
    {
        auto data      = m_TerrainInspector.SaveState();
        auto terrainGI = data->Get<exporters::SerializerNode>("TerrainGI");
        if (!terrainGI) {
            terrainGI = exporters::CreateSerializerNode();
            data->Set("TerrainGI", terrainGI);
        }
        if (m_HeightfieldGICache != nullptr) {
            terrainGI->Set("Progress", m_HeightfieldGICache->GetProgress());
            terrainGI->Set("AccumulatedSamples", m_HeightfieldGICache->GetAccumulatedSamples());
            terrainGI->Set("Ready", m_HeightfieldGICache->IsReady());
        }
        data->Set("TerrainGI", terrainGI);
        return data;
    }

    bool RendererManager::LoadTerrainSettings(exporters::SerializerNode data)
    {
        if (!data) {
            TF3D_LOG_ERROR("Cannot load terrain renderer settings from an empty serializer node");
            return false;
        }

        nlohmann::json stateJson = data->ToJson();
        if (stateJson.contains("TerrainGI") && stateJson["TerrainGI"].is_object()) {
            stateJson["TerrainGI"].erase("Progress");
            stateJson["TerrainGI"].erase("AccumulatedSamples");
            stateJson["TerrainGI"].erase("Ready");
        }

        const auto previousState = m_TerrainInspector.SaveState();
        if (!m_TerrainInspector.LoadState(exporters::CreateSerializerNodeFromJson(stateJson))) {
            m_TerrainInspector.LoadState(previousState);
            return false;
        }

        return true;
    }
} // namespace tf3d::renderer
