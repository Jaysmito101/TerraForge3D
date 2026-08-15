#include "Generators/DEMBaseShapeGenerator.h"

#include "Data/ApplicationState.h"
#include "Generators/DEM/DEMRenderer.h"
#include "Generators/DEM/DEMTileSource.h"
#include "Profiler.h"
#include "imgui/imgui_internal.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace tf3d::generators
{

    DEMBaseShapeGenerator::DEMBaseShapeGenerator(ApplicationState *appState)
        : m_State(State{}),
          m_Tiles(std::make_unique<dem::TileSource>(appState)),
          m_Renderer(std::make_unique<dem::Renderer>(appState))
    {
        const auto apiKey = m_Tiles->GetApiKey();
        std::snprintf(m_APIKeyInput, sizeof(m_APIKeyInput), "%s", apiKey.c_str());
    }

    DEMBaseShapeGenerator::~DEMBaseShapeGenerator() = default;

    bool DEMBaseShapeGenerator::ShowSettings()
    {
        State &settings            = m_UIState;
        const auto currentRevision = m_State.PublishedRevision();
        bool stateChanged          = false;

        if (m_ViewInteractionPending &&
            std::chrono::steady_clock::now() - m_LastViewInteractionTime >=
                std::chrono::milliseconds(kViewSettleMilliseconds)) {
            m_ViewInteractionPending   = false;
            settings.allowTileRequests = true;
            stateChanged               = true;
        }

        if (ImGui::CollapsingHeader("Statistics"))
            ImGui::Text("Tiles Using: %d", m_TilesUsingCount);

        ImGui::TextDisabled("Elevation provider: MapTiler Cloud (Terrain RGB)");
        ImGui::TextWrapped("Paste an API key created in your MapTiler Cloud account. This key is used to download DEM elevation tiles.");
        ImGui::InputText("MapTiler Cloud API Key",
                         m_APIKeyInput,
                         sizeof(m_APIKeyInput),
                         ImGuiInputTextFlags_Password);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Create or copy a key from your MapTiler Cloud account: cloud.maptiler.com/account/keys/");
        }
        ImGui::TextDisabled("Saved in the TerraForge3D user config, not in project files.");

        const auto apiKey = m_Tiles->GetApiKey();
        if (apiKey != m_APIKeyInput && m_APIKeyInput[0] != '\0' && ImGui::Button("Apply")) {
            m_Tiles->SetApiKey(m_APIKeyInput);
            stateChanged = true;
        }
        if (!m_Tiles->HasApiKey()) {
            ImGui::TextDisabled("Add a MapTiler Cloud key before downloading elevation tiles.");
        }

        if (ImGui::Checkbox("Automatic Tile Zoom", &settings.autoZoomResolution)) {
            MarkViewInteraction(settings);
            stateChanged = true;
        }
        if (!settings.autoZoomResolution &&
            ImGui::SliderInt("Manual Tile Zoom", &settings.zoomResolution, 0, dem::kMaxTileZoom)) {
            MarkViewInteraction(settings);
            stateChanged = true;
        }

        const State &previewState  = settings;
        const auto previewRevision = currentRevision + (stateChanged ? 1 : 0);
        const int previewZoom      = GetEffectiveZoomResolution(previewState);
        const int previewTileCount = GetVisibleTileCount(previewState, previewZoom);
        const auto viewTileStatus  = GetViewTileStatus();
        ImGui::Text("Effective tile zoom: %d (%s)", previewZoom,
                    previewState.autoZoomResolution ? "automatic" : "manual");
        ImGui::TextDisabled("Visible tiles: %d/%d | all DEM requests pending: %d/%d",
                            previewTileCount,
                            kMaxVisibleTiles,
                            static_cast<int>(m_Tiles->PendingCount()),
                            m_Tiles->MaxPendingRequests());
        ImGui::TextDisabled("Rendered tiles: %d high-res | %d fallback-covered",
                            m_TilesUsingCount,
                            m_TilesFallbackCount);
        if (!m_Tiles->HasApiKey()) {
            ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.25f, 1.0f),
                               "Tile loading unavailable: add a MapTiler Cloud key.");
        } else if (m_ViewInteractionPending || viewTileStatus.revision != previewRevision) {
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.25f, 1.0f),
                               m_ViewInteractionPending
                                   ? "Waiting for the current zoom/pan to settle before loading tiles..."
                                   : "Preparing tiles for the current zoom/pan...");
        } else if (viewTileStatus.visible == 0) {
            ImGui::TextDisabled("No DEM tiles intersect the current view.");
        } else if (viewTileStatus.ready >= viewTileStatus.visible) {
            ImGui::TextColored(ImVec4(0.35f, 1.0f, 0.5f, 1.0f),
                               "Tiles ready for the current zoom/pan: %d/%d high-res.",
                               viewTileStatus.ready,
                               viewTileStatus.visible);
        } else {
            const int waiting = std::max(0, viewTileStatus.visible - viewTileStatus.ready - viewTileStatus.pending - viewTileStatus.blocked);
            if (viewTileStatus.blocked > 0 && viewTileStatus.pending == 0 && waiting == 0) {
                ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.25f, 1.0f),
                                   "Tile loading paused: %d tile(s) blocked by the download circuit.",
                                   viewTileStatus.blocked);
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.25f, 1.0f),
                                   "Loading tiles for the current zoom/pan: %d/%d ready (%d downloading, %d waiting).",
                                   viewTileStatus.ready,
                                   viewTileStatus.visible,
                                   viewTileStatus.pending,
                                   waiting + viewTileStatus.blocked);
            }
        }
        ImGui::TextDisabled("At most %d new requests per update; view changes cancel pending downloads; requests are at least %d ms apart.",
                            m_Tiles->MaxRequestsPerUpdate(),
                            m_Tiles->RequestIntervalMilliseconds());

        settings.mapStrength = std::clamp(settings.mapStrength, 0.0f, kMaxMapStrength);
        if (ImGui::DragFloat("Strength", &settings.mapStrength, 0.01f, 0.0f, kMaxMapStrength))
            stateChanged = true;

        ImGui::ImageButton(m_Renderer->Texture().GetTextureID(), ImVec2(400, 400));
        ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);

        const bool mapHovered = ImGui::IsItemHovered();
        const bool mapActive  = ImGui::IsItemActive();
        const bool zooming    = mapHovered && std::abs(ImGui::GetIO().MouseWheel) > 0.05f;
        const bool panning    = (mapHovered || mapActive) &&
                             (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f) ||
                              ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f)) &&
                             (std::abs(ImGui::GetIO().MouseDelta.x) > 0.001f ||
                              std::abs(ImGui::GetIO().MouseDelta.y) > 0.01f);

        if (zooming || panning) {
            const ImVec2 imageMin      = ImGui::GetItemRectMin();
            const ImVec2 imageMax      = ImGui::GetItemRectMax();
            const ImVec2 mousePosition = ImGui::GetIO().MousePos;
            const glm::vec2 imageSize(
                std::max(imageMax.x - imageMin.x, 1.0f),
                std::max(imageMax.y - imageMin.y, 1.0f));
            const glm::vec2 cursorUV = glm::clamp(
                glm::vec2(
                    (mousePosition.x - imageMin.x) / imageSize.x,
                    (mousePosition.y - imageMin.y) / imageSize.y),
                glm::vec2(0.0f), glm::vec2(1.0f));

            if (zooming) {
                const float oldZoom         = settings.zoomOnMap;
                const glm::vec2 cursorWorld = cursorUV / oldZoom - settings.mapCenter;
                settings.zoomOnMap *= std::pow(1.18f, ImGui::GetIO().MouseWheel);
                settings.zoomOnMap = std::clamp(settings.zoomOnMap, 1.0f, 4096.0f);
                settings.mapCenter = cursorUV / settings.zoomOnMap - cursorWorld;
            }

            settings.mapCenter.x += ImGui::GetIO().MouseDelta.x * 0.006f / settings.zoomOnMap;
            settings.mapCenter.y += ImGui::GetIO().MouseDelta.y * 0.006f / settings.zoomOnMap;
            settings.zoomOnMap   = std::clamp(settings.zoomOnMap, 1.0f, 4096.0f);
            settings.mapCenter.x = std::clamp(settings.mapCenter.x, -4.0f, 4.0f);
            settings.mapCenter.y = std::clamp(settings.mapCenter.y, -4.0f, 4.0f);
            MarkViewInteraction(settings);
            stateChanged = true;
        }

        if (ImGui::DragFloat("View Zoom", &settings.zoomOnMap, 0.05f, 1.0f, 4096.0f)) {
            MarkViewInteraction(settings);
            stateChanged = true;
        }
        if (ImGui::DragFloat2("Center Position", glm::value_ptr(settings.mapCenter), 0.01f)) {
            MarkViewInteraction(settings);
            stateChanged = true;
        }

        settings.zoomOnMap   = std::clamp(settings.zoomOnMap, 1.0f, 4096.0f);
        settings.mapCenter.x = std::clamp(settings.mapCenter.x, -4.0f, 4.0f);
        settings.mapCenter.y = std::clamp(settings.mapCenter.y, -4.0f, 4.0f);

        if (stateChanged) {
            m_State.Replace(settings);
        }

        return RequireUpdation();
    }

    void DEMBaseShapeGenerator::MarkViewInteraction(State &settings)
    {
        m_ViewInteractionPending   = true;
        m_LastViewInteractionTime  = std::chrono::steady_clock::now();
        settings.allowTileRequests = false;
        m_Tiles->CancelPendingRequests();
    }

    DEMBaseShapeGenerator::ViewTileStatus DEMBaseShapeGenerator::GetViewTileStatus() const
    {
        std::lock_guard lock(m_ViewTileStatusMutex);
        return m_ViewTileStatus;
    }

    void DEMBaseShapeGenerator::GetVisibleTileRange(const State &state,
                                                    int32_t zoomResolution,
                                                    int32_t &minTileX,
                                                    int32_t &maxTileX,
                                                    int32_t &minTileY,
                                                    int32_t &maxTileY) const
    {
        zoomResolution          = std::clamp(zoomResolution, 0, static_cast<int32_t>(dem::kMaxTileZoom));
        const int32_t tileCount = 1 << zoomResolution;
        const float tileSize    = 1.0f / static_cast<float>(tileCount);
        const float viewZoom    = std::clamp(state.zoomOnMap, 1.0f, 4096.0f);
        const float viewEnd     = 1.0f / viewZoom;
        const float edgeEpsilon = 0.00001f;

        auto calculateRange = [&](float center, int32_t &minTile, int32_t &maxTile) {
            const int32_t rawMin = static_cast<int32_t>(std::ceil((-center / tileSize) - 1.0f - edgeEpsilon));
            const int32_t rawMax = static_cast<int32_t>(std::ceil(((viewEnd - center) / tileSize) - edgeEpsilon)) - 1;
            minTile              = std::max(0, rawMin);
            maxTile              = std::min(tileCount - 1, rawMax);
        };

        calculateRange(state.mapCenter.x, minTileX, maxTileX);
        calculateRange(state.mapCenter.y, minTileY, maxTileY);
    }

    int32_t DEMBaseShapeGenerator::GetVisibleTileCount(const State &state, int32_t zoomResolution) const
    {
        int32_t minTileX = 0, maxTileX = -1, minTileY = 0, maxTileY = -1;
        GetVisibleTileRange(state, zoomResolution, minTileX, maxTileX, minTileY, maxTileY);
        if (maxTileX < minTileX || maxTileY < minTileY)
            return 0;
        return (maxTileX - minTileX + 1) * (maxTileY - minTileY + 1);
    }

    int32_t DEMBaseShapeGenerator::GetEffectiveZoomResolution(const State &state) const
    {
        const float viewZoom  = std::clamp(state.zoomOnMap, 1.0f, 4096.0f);
        int32_t requestedZoom = state.autoZoomResolution
                                    ? static_cast<int32_t>(std::floor(std::log2(viewZoom))) + 3
                                    : state.zoomResolution;
        requestedZoom         = std::clamp(requestedZoom, 0, static_cast<int32_t>(dem::kMaxTileZoom));

        while (requestedZoom > 0 && GetVisibleTileCount(state, requestedZoom) > kMaxVisibleTiles)
            --requestedZoom;
        return requestedZoom;
    }

    void DEMBaseShapeGenerator::Update(const Snapshot *state,
                                       const GenerationContext *context,
                                       GeneratorData *buffer)
    {
        if (state == nullptr || context == nullptr || buffer == nullptr || m_Tiles == nullptr ||
            m_Renderer == nullptr || context->tileResolution <= 0) {
            return;
        }

        TF3D_PROFILE_SCOPE_CHILD("dem-base-shape");

        State settings          = state->value;
        settings.zoomOnMap      = std::clamp(settings.zoomOnMap, 1.0f, 4096.0f);
        settings.mapCenter.x    = std::clamp(settings.mapCenter.x, -4.0f, 4.0f);
        settings.mapCenter.y    = std::clamp(settings.mapCenter.y, -4.0f, 4.0f);
        settings.mapStrength    = std::clamp(settings.mapStrength, 0.0f, kMaxMapStrength);
        settings.zoomResolution = std::clamp(settings.zoomResolution, 0, static_cast<int32_t>(dem::kMaxTileZoom));

        const int32_t workgroupSize = context->gpuWorkgroupSize > 0 ? context->gpuWorkgroupSize : 1;
        m_Tiles->BeginUpdate(settings.allowTileRequests);

        m_EffectiveZoomResolution = GetEffectiveZoomResolution(settings);
        m_VisibleTileCount        = GetVisibleTileCount(settings, m_EffectiveZoomResolution);

        int32_t minTileX = 0, maxTileX = -1, minTileY = 0, maxTileY = -1;
        GetVisibleTileRange(settings, m_EffectiveZoomResolution, minTileX, maxTileX, minTileY, maxTileY);
        const int32_t requestedZoom = settings.zoomResolution;
        m_TilesSkippedCount         = std::max(0, GetVisibleTileCount(settings, requestedZoom) - m_VisibleTileCount);

        std::vector<dem::RenderTile> fallbackTiles;
        std::vector<dem::RenderTile> highResolutionTiles;
        int viewReadyCount       = 0;
        int viewPendingCount     = 0;
        int viewBlockedCount     = 0;
        const auto effectiveZoom = static_cast<uint32_t>(m_EffectiveZoomResolution);
        for (int yi = minTileY; yi <= maxTileY; ++yi) {
            for (int xi = minTileX; xi <= maxTileX; ++xi) {
                const dem::TileKey exactKey{static_cast<uint32_t>(xi),
                                            static_cast<uint32_t>(yi),
                                            effectiveZoom};
                if (const auto exactTile = m_Tiles->FindLoaded(exactKey); exactTile != nullptr) {
                    highResolutionTiles.push_back({exactKey, exactTile, false});
                    ++viewReadyCount;
                    continue;
                }

                dem::TileKey fallbackKey;
                if (const auto fallback = m_Tiles->FindBestAvailable(exactKey, fallbackKey); fallback != nullptr) {
                    const auto duplicate = std::find_if(
                        fallbackTiles.begin(), fallbackTiles.end(),
                        [&fallbackKey](const dem::RenderTile &candidate) { return candidate.key == fallbackKey; });
                    if (duplicate == fallbackTiles.end())
                        fallbackTiles.push_back({fallbackKey, fallback, true});
                }

                m_Tiles->Request(exactKey);
                if (const auto loaded = m_Tiles->FindLoaded(exactKey); loaded != nullptr) {
                    highResolutionTiles.push_back({exactKey, loaded, false});
                    ++viewReadyCount;
                } else if (m_Tiles->IsPending(exactKey) || m_Tiles->IsTexturePending(exactKey)) {
                    ++viewPendingCount;
                } else if (m_Tiles->IsCircuitOpen()) {
                    ++viewBlockedCount;
                }
            }
        }

        std::vector<dem::RenderTile> renderTiles;
        renderTiles.reserve(fallbackTiles.size() + highResolutionTiles.size());
        renderTiles.insert(renderTiles.end(), fallbackTiles.begin(), fallbackTiles.end());
        renderTiles.insert(renderTiles.end(), highResolutionTiles.begin(), highResolutionTiles.end());

        const dem::RenderSettings renderSettings{
            context->tileResolution,
            workgroupSize,
            settings.zoomOnMap,
            settings.mapStrength,
            settings.mapCenter};
        const auto renderStats = m_Renderer->Render(renderSettings,
                                                    buffer,
                                                    renderTiles);
        m_TilesUsingCount      = renderStats.tilesUsing;
        m_TilesFallbackCount   = renderStats.tilesFallback;

        {
            std::lock_guard lock(m_ViewTileStatusMutex);
            m_ViewTileStatus = {state->revision,
                                m_VisibleTileCount,
                                viewReadyCount,
                                viewPendingCount,
                                viewBlockedCount};
        }

        TF3D_PROFILE_VALUE_DOMAIN("generation/dem/visible-tiles",
                                  static_cast<uint64_t>(m_VisibleTileCount),
                                  static_cast<uint64_t>(m_TilesFallbackCount),
                                  static_cast<uint64_t>(m_TilesUsingCount),
                                  PerformanceMonitor::Domain::Generation);
        m_State.MarkProcessed(state->revision);
    }

    void DEMBaseShapeGenerator::Load(SerializerNode data)
    {
        if (data == nullptr)
            return;

        m_UIState.zoomOnMap          = data->Get<float>("ZoomOnMap", 1.0f);
        m_UIState.zoomResolution     = data->Get<int>("ZoomResolution", 0);
        m_UIState.autoZoomResolution = data->Get<bool>("AutoZoomResolution", true);
        m_UIState.mapStrength        = data->Get<float>("MapStrength", 1.0f);
        m_UIState.mapCenter          = data->Get<glm::vec2>("MapCenter", glm::vec2(0.0f));
        m_State.Replace(m_UIState);
    }

    SerializerNode DEMBaseShapeGenerator::Save()
    {
        auto node          = CreateSerializerNode();
        const State &state = m_UIState;
        node->Set("ZoomOnMap", state.zoomOnMap);
        node->Set("ZoomResolution", state.zoomResolution);
        node->Set("AutoZoomResolution", state.autoZoomResolution);
        node->Set("MapStrength", state.mapStrength);
        node->Set("MapCenter", state.mapCenter);
        return node;
    }

} // namespace tf3d::generators
