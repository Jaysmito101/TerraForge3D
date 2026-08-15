#include "Generators/DEM/DEMRenderer.h"

#include "Data/ApplicationState.h"
#include "Generators/GeneratorData.h"
#include "Profiler.h"

#include <algorithm>
#include <cmath>

namespace tf3d::generators::dem
{

    constexpr float kZoomStrengthExponent = 0.5f;

    struct DispatchRegion {
        glm::ivec2 offset = glm::ivec2(0);
        glm::ivec2 size   = glm::ivec2(0);
    };

    DispatchRegion GetTileDispatchRegion(const glm::vec2 &start,
                                         const glm::vec2 &end,
                                         int32_t resolution)
    {
        const glm::vec2 clippedStart = glm::clamp(start, glm::vec2(0.0f), glm::vec2(1.0f));
        const glm::vec2 clippedEnd   = glm::clamp(end, glm::vec2(0.0f), glm::vec2(1.0f));
        if (clippedStart.x >= clippedEnd.x || clippedStart.y >= clippedEnd.y) {
            return {};
        }

        const glm::vec2 pixelStartUV(clippedStart.x, 1.0f - clippedEnd.y);
        const glm::vec2 pixelEndUV(clippedEnd.x, 1.0f - clippedStart.y);
        const glm::ivec2 offset(static_cast<int32_t>(std::floor(pixelStartUV.x * resolution)),
                                static_cast<int32_t>(std::floor(pixelStartUV.y * resolution)));
        const glm::ivec2 endPixel(static_cast<int32_t>(std::ceil(pixelEndUV.x * resolution)),
                                  static_cast<int32_t>(std::ceil(pixelEndUV.y * resolution)));
        const glm::ivec2 clippedOffset   = glm::clamp(offset, glm::ivec2(0), glm::ivec2(resolution));
        const glm::ivec2 clippedEndPixel = glm::clamp(endPixel, glm::ivec2(0), glm::ivec2(resolution));
        if (clippedOffset.x >= clippedEndPixel.x || clippedOffset.y >= clippedEndPixel.y) {
            return {};
        }

        return {clippedOffset, clippedEndPixel - clippedOffset};
    }

    Renderer::Renderer(tf3d::data::ApplicationState *appState)
        : m_Texture(kPreviewWidth, kPreviewHeight)
    {
        if (appState != nullptr && appState->resourceManager != nullptr) {
            m_Shader = appState->resourceManager->LoadComputeShader("generation/dem/map_gen");
        }
    }

    RenderStats Renderer::Render(const RenderSettings &settings,
                                 GeneratorData *buffer,
                                 std::span<const RenderTile> tiles)
    {
        RenderStats stats{};
        if (!m_Shader || buffer == nullptr || settings.tileResolution <= 0) {
            return stats;
        }

        const int32_t workgroupSize = std::max(settings.workgroupSize, 1);
        const int32_t dispatchSize  = (settings.tileResolution + workgroupSize - 1) / workgroupSize;

        buffer->Bind(0);
        m_Shader->Bind();
        m_Shader->SetUniform1i("u_Resolution", settings.tileResolution);
        m_Shader->SetUniform1i("u_Mode", 0);
        {
            TF3D_PROFILE_GPU_SCOPE_CHILD("clear/gpu");
            m_Shader->Dispatch(dispatchSize, dispatchSize, 1);
        }
        m_Shader->SetMemoryBarrier();

        m_Shader->SetUniform1i("u_Mode", 1);
        m_Shader->SetUniform1f("u_ZoomOnMap", settings.zoomOnMap);
        const float zoomStrengthMultiplier =
            std::pow(std::max(settings.zoomOnMap, 1.0f), kZoomStrengthExponent);
        const float effectiveMapStrength =
            std::clamp(settings.mapStrength * zoomStrengthMultiplier, 0.0f, 1000.0f);
        m_Shader->SetUniform1f("u_MapStrength", effectiveMapStrength);
        {
            TF3D_PROFILE_GPU_SCOPE_CHILD("tiles/gpu");
            for (const auto &tile : tiles) {
                if (tile.texture == nullptr) {
                    continue;
                }

                const float tileSize  = 1.0f / static_cast<float>(1u << tile.key.zoom);
                const glm::vec2 start = (settings.mapCenter +
                                         glm::vec2(tileSize * static_cast<float>(tile.key.x),
                                                   tileSize * static_cast<float>(tile.key.y))) *
                                        settings.zoomOnMap;
                const glm::vec2 end                 = start + glm::vec2(tileSize * settings.zoomOnMap);
                const DispatchRegion dispatchRegion = GetTileDispatchRegion(start, end, settings.tileResolution);
                if (dispatchRegion.size.x <= 0 || dispatchRegion.size.y <= 0) {
                    continue;
                }

                m_Shader->SetUniform1f("u_RegionTileSize", tileSize * settings.zoomOnMap);
                m_Shader->SetUniform2i("u_DispatchOffset", dispatchRegion.offset.x, dispatchRegion.offset.y);
                m_Shader->SetUniform1i("u_DEMTexture", tile.texture->Bind(0));
                m_Shader->SetUniform4f("u_RegionToUpdate", glm::vec4(start, end));
                const int32_t tileDispatchX = (dispatchRegion.size.x + workgroupSize - 1) / workgroupSize;
                const int32_t tileDispatchY = (dispatchRegion.size.y + workgroupSize - 1) / workgroupSize;
                m_Shader->Dispatch(tileDispatchX, tileDispatchY, 1);

                if (tile.fallback) {
                    ++stats.tilesFallback;
                } else {
                    ++stats.tilesUsing;
                }
            }
        }

        m_Shader->SetMemoryBarrier();
        m_Shader->SetUniform1i("u_Mode", 2);
        {
            TF3D_PROFILE_GPU_SCOPE_CHILD("visualizer/gpu");
            m_Texture.BindForCompute(1);
            m_Shader->Dispatch((m_Texture.GetWidth() + workgroupSize - 1) / workgroupSize,
                               (m_Texture.GetHeight() + workgroupSize - 1) / workgroupSize,
                               1);
        }
        m_Shader->SetMemoryBarrier();
        return stats;
    }

} // namespace tf3d::generators::dem
