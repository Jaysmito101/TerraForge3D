#include "Generators/HeightfieldPyramid.h"

#include "Data/ApplicationState.h"
#include "Data/ResourceManager.h"
#include "Generators/GeneratorData.h"
#include "Profiler.h"

#include <algorithm>

namespace tf3d::generators
{

    constexpr int32_t WorkgroupSize = 16;

    HeightfieldPyramid::HeightfieldPyramid(ApplicationState *appState)
        : m_AppState(appState)
    {
        if (m_AppState != nullptr && m_AppState->resourceManager != nullptr) {
            m_Shader = m_AppState->resourceManager->LoadComputeShader("heightfield/minmax_pyramid/compute", true);
            m_RayQueryShader = m_AppState->resourceManager->LoadComputeShader("heightfield/pyramid_ray_query", true);
        }
    }

    HeightfieldPyramid::~HeightfieldPyramid()
    {
        ReleaseTexture();
        ReleaseRayQueryResultTexture();
    }

    void HeightfieldPyramid::ReleaseTexture()
    {
        if (m_RendererID != 0) {
            glDeleteTextures(1, &m_RendererID);
            m_RendererID = 0;
        }
        m_Resolution = 0;
        m_MipLevels  = 0;
        m_IsReady    = false;
    }

    void HeightfieldPyramid::EnsureTexture(int32_t resolution)
    {
        if (resolution <= 0)
            return;

        int32_t mipLevels = 1;
        for (int32_t mipSize = resolution; mipSize > 1; mipSize >>= 1)
            ++mipLevels;
        if (m_RendererID != 0 && m_Resolution == resolution && m_MipLevels == mipLevels)
            return;

        ReleaseTexture();
        m_Resolution = resolution;
        m_MipLevels  = mipLevels;

        glGenTextures(1, &m_RendererID);
        glBindTexture(GL_TEXTURE_2D, m_RendererID);
        glTexStorage2D(GL_TEXTURE_2D, m_MipLevels, GL_RGBA32F, m_Resolution, m_Resolution);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, m_MipLevels - 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
        m_IsReady = false;
    }

    void HeightfieldPyramid::ReleaseRayQueryResultTexture()
    {
        if (m_RayQueryResultRendererID != 0) {
            glDeleteTextures(1, &m_RayQueryResultRendererID);
            m_RayQueryResultRendererID = 0;
        }
    }

    void HeightfieldPyramid::EnsureRayQueryResultTexture()
    {
        if (m_RayQueryResultRendererID != 0)
            return;

        glGenTextures(1, &m_RayQueryResultRendererID);
        glBindTexture(GL_TEXTURE_2D, m_RayQueryResultRendererID);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, 1, 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    bool HeightfieldPyramid::Rebuild(GeneratorData *heightmap)
    {
        TF3D_PROFILE_SCOPE_DOMAIN("generation/heightfield-pyramid", PerformanceMonitor::Domain::Generation);
        if (heightmap == nullptr || heightmap->GetResolution() <= 0 || !m_Shader)
            return false;

        const int32_t resolution = heightmap->GetResolution();
        EnsureTexture(resolution);

        heightmap->BindAsTexture(0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_RendererID);

        m_Shader->Bind();
        m_Shader->SetUniform1i("u_Heightmap", 0);
        m_Shader->SetUniform1i("u_Pyramid", 1);

        int32_t sourceWidth  = resolution;
        int32_t sourceHeight = resolution;
        TF3D_PROFILE_GPU_SCOPE("generation/heightfield-pyramid/gpu");
        for (int32_t level = 0; level < m_MipLevels; ++level) {
            const bool sourceIsHeightmap = level == 0;
            const int32_t outputWidth    = sourceIsHeightmap ? resolution : std::max(sourceWidth / 2, 1);
            const int32_t outputHeight   = sourceIsHeightmap ? resolution : std::max(sourceHeight / 2, 1);

            m_Shader->SetUniform1i("u_SourceIsHeightmap", sourceIsHeightmap ? 1 : 0);
            m_Shader->SetUniform1i("u_SourceLevel", sourceIsHeightmap ? 0 : level - 1);
            m_Shader->SetUniform2f("u_SourceSize", static_cast<float>(sourceWidth), static_cast<float>(sourceHeight));
            m_Shader->SetUniform2f("u_OutputSize", static_cast<float>(outputWidth), static_cast<float>(outputHeight));
            glBindImageTexture(0, m_RendererID, level, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
            glDispatchCompute((outputWidth + WorkgroupSize - 1) / WorkgroupSize,
                              (outputHeight + WorkgroupSize - 1) / WorkgroupSize, 1);
            TF3D_PROFILE_COUNTER_DOMAIN("gpu/dispatches", 1.0, PerformanceMonitor::Domain::Gpu);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

            sourceWidth  = outputWidth;
            sourceHeight = outputHeight;
        }

        m_Shader->Unbind();
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        m_IsReady = true;
        return true;
    }

    bool HeightfieldPyramid::IntersectWorldRay(const glm::vec3 &rayOrigin,
                                               const glm::vec3 &rayDirection,
                                               const glm::vec2 &terrainMinimumXZ,
                                               const glm::vec2 &terrainWorldSize,
                                               float terrainHeightOffset,
                                               HeightfieldRayHit &hit,
                                               float heightBias)
    {
        hit = {};
        if (!m_IsReady || m_RendererID == 0 || m_MipLevels <= 0 ||
            !m_RayQueryShader.has_value() || !m_RayQueryShader->IsValid() ||
            terrainWorldSize.x <= 0.000001f || terrainWorldSize.y <= 0.000001f) {
            return false;
        }

        const float directionLength = glm::length(rayDirection);
        if (!std::isfinite(directionLength) || directionLength <= 0.000001f)
            return false;

        if (!std::isfinite(rayOrigin.x) || !std::isfinite(rayOrigin.y) || !std::isfinite(rayOrigin.z) ||
            !std::isfinite(terrainMinimumXZ.x) || !std::isfinite(terrainMinimumXZ.y) ||
            !std::isfinite(terrainWorldSize.x) || !std::isfinite(terrainWorldSize.y) ||
            !std::isfinite(terrainHeightOffset)) {
            return false;
        }

        const glm::vec3 normalizedDirection = rayDirection / directionLength;
        EnsureRayQueryResultTexture();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_RendererID);

        m_RayQueryShader->Bind();
        m_RayQueryShader->SetUniform1i("u_HeightPyramid", 0);
        m_RayQueryShader->SetUniform1i("u_PyramidLevels", m_MipLevels);
        m_RayQueryShader->SetUniform3f("u_RayOrigin", rayOrigin);
        m_RayQueryShader->SetUniform3f("u_RayDirection", normalizedDirection);
        m_RayQueryShader->SetUniform2f("u_TerrainMinimumXZ", terrainMinimumXZ);
        m_RayQueryShader->SetUniform2f("u_TerrainWorldSize", terrainWorldSize);
        m_RayQueryShader->SetUniform1f("u_TerrainHeightOffset", terrainHeightOffset);
        m_RayQueryShader->SetUniform1f("u_HeightBias", std::max(heightBias, 0.0f));

        glBindImageTexture(0, m_RayQueryResultRendererID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
        {
            TF3D_PROFILE_GPU_SCOPE("renderer/picking/heightfield-pyramid-ray-query");
            glDispatchCompute(1, 1, 1);
            TF3D_PROFILE_COUNTER_DOMAIN("gpu/dispatches", 1.0, PerformanceMonitor::Domain::Gpu);
        }
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT |
                        GL_TEXTURE_FETCH_BARRIER_BIT);
        m_RayQueryShader->Unbind();

        float result[4]{};
        {
            TF3D_PROFILE_SCOPE_DOMAIN("renderer/picking/heightfield-pyramid-ray-query/readback",
                                      PerformanceMonitor::Domain::Wait);
            TF3D_PROFILE_VALUE_DOMAIN("renderer/picking/heightfield-pyramid-ray-query/readback-bytes",
                                      sizeof(result), 0, 0, PerformanceMonitor::Domain::Wait);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_RayQueryResultRendererID);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, result);
        }

        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindImageTexture(0, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
        glBindTexture(GL_TEXTURE_2D, 0);

        if (result[3] < 0.5f || !std::isfinite(result[0]) || !std::isfinite(result[1]) ||
            !std::isfinite(result[2])) {
            return false;
        }

        hit.worldPosition                = glm::vec3(result[0], result[1], result[2]);
        const glm::vec2 terrainMaximumXZ = terrainMinimumXZ + terrainWorldSize;
        hit.terrainUv                    = glm::vec2(
            (hit.worldPosition.x - terrainMinimumXZ.x) / terrainWorldSize.x,
            (terrainMaximumXZ.y - hit.worldPosition.z) / terrainWorldSize.y);
        hit.terrainHeight = hit.worldPosition.y - terrainHeightOffset;
        hit.distance      = glm::dot(hit.worldPosition - rayOrigin, normalizedDirection);
        return std::isfinite(hit.distance) && hit.distance >= 0.0f;
    }

} // namespace tf3d::generators
