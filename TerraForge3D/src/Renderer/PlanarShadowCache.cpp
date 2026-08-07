#include "Renderer/PlanarShadowCache.h"

#include "Data/ApplicationState.h"
#include "Data/ResourceManager.h"
#include "Generators/HeightfieldPyramid.h"
#include "Profiler.h"

#include <algorithm>

namespace tf3d::renderer
{
    namespace
    {
        constexpr int32_t WorkgroupSize          = 8;
        constexpr int32_t MaximumAtlasResolution = 1024;
    } // namespace

    PlanarShadowCache::PlanarShadowCache(ApplicationState *appState)
        : m_AppState(appState)
    {
        if (m_AppState != nullptr && m_AppState->resourceManager != nullptr) {
            m_Shader = m_AppState->resourceManager->LoadComputeShader("heightfield/planar_shadow/compute", true);
        }
    }

    PlanarShadowCache::~PlanarShadowCache()
    {
        ReleaseTexture();
    }

    void PlanarShadowCache::ReleaseTexture()
    {
        if (m_RendererID != 0) {
            glDeleteTextures(1, &m_RendererID);
            m_RendererID = 0;
        }
        m_Resolution = 0;
        m_IsReady    = false;
    }

    void PlanarShadowCache::EnsureTexture(int32_t resolution)
    {
        resolution = std::clamp(resolution, 1, MaximumAtlasResolution);
        if (m_RendererID != 0 && m_Resolution == resolution)
            return;

        ReleaseTexture();
        m_Resolution      = resolution;
        int32_t mipLevels = 1;
        for (int32_t mipSize = m_Resolution; mipSize > 1; mipSize >>= 1)
            ++mipLevels;

        glGenTextures(1, &m_RendererID);
        glBindTexture(GL_TEXTURE_2D, m_RendererID);
        glTexStorage2D(GL_TEXTURE_2D, mipLevels, GL_R8, m_Resolution, m_Resolution);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipLevels - 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
        m_IsReady = false;
    }

    bool PlanarShadowCache::Update(HeightfieldPyramid *heightPyramid, uint64_t terrainRevision,
                                   const glm::vec3 &sunDirection, const glm::vec2 &terrainMinimumXZ,
                                   float terrainWorldSize, float terrainHeightOffset, float terrainMaximumHeight,
                                   float receiverHeight)
    {
        TF3D_PROFILE_SCOPE_DOMAIN("renderer/cache/planar-shadow/update", PerformanceMonitor::Domain::Renderer);
        if (heightPyramid == nullptr || !heightPyramid->IsReady() || !m_Shader ||
            terrainWorldSize <= 0.000001f) {
            return false;
        }

        const float directionLength = glm::length(sunDirection);
        if (directionLength <= 0.000001f)
            return false;

        const glm::vec3 normalizedSunDirection = sunDirection / directionLength;
        const glm::vec3 lightDirection         = -normalizedSunDirection;
        const glm::vec2 terrainMaximumXZ       = terrainMinimumXZ + glm::vec2(terrainWorldSize);
        glm::vec2 atlasMinimumXZ               = terrainMinimumXZ;
        glm::vec2 atlasMaximumXZ               = terrainMaximumXZ;

        const float maximumHeightAboveReceiver = terrainHeightOffset + terrainMaximumHeight - receiverHeight;
        if (lightDirection.y > 0.000001f && maximumHeightAboveReceiver > 0.0f) {
            const glm::vec2 lightDirectionXZ(lightDirection.x, lightDirection.z);
            const glm::vec2 projectedOffset = -lightDirectionXZ * (maximumHeightAboveReceiver / lightDirection.y);
            atlasMinimumXZ                  = glm::min(atlasMinimumXZ, terrainMinimumXZ + projectedOffset);
            atlasMaximumXZ                  = glm::max(atlasMaximumXZ, terrainMaximumXZ + projectedOffset);
        }

        const glm::vec2 atlasWorldSize    = glm::max(atlasMaximumXZ - atlasMinimumXZ, glm::vec2(0.000001f));
        const int32_t requestedResolution = std::max(1, heightPyramid->GetResolution());
        const bool requiresRebuild        = !m_IsReady ||
                                     m_TerrainRevision != terrainRevision ||
                                     glm::length(normalizedSunDirection - m_SunDirection) > 0.00001f ||
                                     glm::length(m_AtlasMinimumXZ - atlasMinimumXZ) > 0.00001f ||
                                     glm::length(m_AtlasWorldSize - atlasWorldSize) > 0.00001f ||
                                     std::abs(m_TerrainWorldSize - terrainWorldSize) > 0.00001f ||
                                     std::abs(m_TerrainHeightOffset - terrainHeightOffset) > 0.00001f ||
                                     std::abs(m_TerrainMaximumHeight - terrainMaximumHeight) > 0.00001f ||
                                     std::abs(m_ReceiverHeight - receiverHeight) > 0.00001f ||
                                     m_Resolution != std::min(requestedResolution, MaximumAtlasResolution);
        if (!requiresRebuild)
            return false;

        EnsureTexture(requestedResolution);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, heightPyramid->GetRendererID());

        m_Shader->Bind();
        m_Shader->SetUniform1i("u_HeightPyramid", 0);
        m_Shader->SetUniform1i("u_PyramidLevels", heightPyramid->GetMipLevels());
        m_Shader->SetUniform1i("u_OutputResolution", m_Resolution);
        m_Shader->SetUniform3f("u_LightDirection", lightDirection);
        m_Shader->SetUniform2f("u_AtlasMinimumXZ", atlasMinimumXZ);
        m_Shader->SetUniform2f("u_AtlasWorldSize", atlasWorldSize);
        m_Shader->SetUniform2f("u_TerrainMinimumXZ", terrainMinimumXZ);
        m_Shader->SetUniform2f("u_TerrainWorldSize", terrainWorldSize, terrainWorldSize);
        m_Shader->SetUniform1f("u_TerrainHeightOffset", terrainHeightOffset);
        m_Shader->SetUniform1f("u_ReceiverHeight", receiverHeight);
        m_Shader->SetUniform1f("u_ReceiverBias", std::max(0.001f, terrainWorldSize / static_cast<float>(m_Resolution) * 1.5f));
        const glm::vec2 lightDirectionXZ(lightDirection.x, lightDirection.z);
        m_Shader->SetUniform1f("u_MaxDistance", glm::length(atlasWorldSize) * 2.0f / std::max(glm::length(lightDirectionXZ), 0.000001f));

        {
            TF3D_PROFILE_GPU_SCOPE("renderer/cache/planar-shadow/dispatch");
            glBindImageTexture(0, m_RendererID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8);
            glDispatchCompute((m_Resolution + WorkgroupSize - 1) / WorkgroupSize,
                              (m_Resolution + WorkgroupSize - 1) / WorkgroupSize, 1);
            TF3D_PROFILE_COUNTER_DOMAIN("gpu/dispatches", 1.0, PerformanceMonitor::Domain::Gpu);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT);
        }
        {
            TF3D_PROFILE_GPU_SCOPE("renderer/cache/planar-shadow/mipmap");
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_RendererID);
            glGenerateMipmap(GL_TEXTURE_2D);
            TF3D_PROFILE_COUNTER_DOMAIN("gpu/mipmap-generations", 1.0, PerformanceMonitor::Domain::Gpu);
            glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
        }

        m_Shader->Unbind();
        glBindTexture(GL_TEXTURE_2D, 0);

        m_TerrainRevision      = terrainRevision;
        m_SunDirection         = normalizedSunDirection;
        m_AtlasMinimumXZ       = atlasMinimumXZ;
        m_AtlasWorldSize       = atlasWorldSize;
        m_TerrainWorldSize     = terrainWorldSize;
        m_TerrainHeightOffset  = terrainHeightOffset;
        m_TerrainMaximumHeight = terrainMaximumHeight;
        m_ReceiverHeight       = receiverHeight;
        m_IsReady              = true;
        return true;
    }

    void PlanarShadowCache::Bind(uint32_t textureSlot) const
    {
        if (!m_IsReady || m_RendererID == 0)
            return;
        glActiveTexture(GL_TEXTURE0 + textureSlot);
        glBindTexture(GL_TEXTURE_2D, m_RendererID);
    }
} // namespace tf3d::renderer
