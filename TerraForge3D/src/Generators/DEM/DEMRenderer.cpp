#include "Generators/DEM/DEMRenderer.h"

#include "Data/ApplicationState.h"
#include "Generators/GeneratorData.h"
#include "Profiler.h"

#include <algorithm>

namespace tf3d::generators::dem
{

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
        RenderStats stats;
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
        m_Shader->SetUniform1f("u_MapStrength", settings.mapStrength);
        m_Shader->SetUniform1i("u_AdaptiveBaseMultiplier", settings.adaptiveBaseMultiplier ? 1 : 0);
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
                const glm::vec2 end = start + glm::vec2(tileSize * settings.zoomOnMap);
                m_Shader->SetUniform1f("u_RegionTileSize", tileSize * settings.zoomOnMap);
                m_Shader->SetUniform1i("u_DEMTexture", tile.texture->Bind(0));
                m_Shader->SetUniform4f("u_RegionToUpdate", glm::vec4(start, end));
                m_Shader->Dispatch(dispatchSize, dispatchSize, 1);

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
