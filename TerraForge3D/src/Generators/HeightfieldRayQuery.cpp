#include "Generators/HeightfieldRayQuery.h"

#include "Data/ResourceManager.h"
#include "Profiler.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace tf3d::generators
{
    HeightfieldRayQuery::HeightfieldRayQuery(data::ResourceManager *resourceManager)
    {
        if (resourceManager != nullptr)
            m_Shader = resourceManager->LoadComputeShader("heightfield/pyramid_ray_query/compute", true);
    }

    HeightfieldRayQuery::~HeightfieldRayQuery()
    {
        m_Readback.Release();
        m_ResultTexture.Release();
    }

    void HeightfieldRayQuery::Invalidate()
    {
        ++m_Generation;
        m_LastRayHit          = {};
        m_HasLastRayHit       = false;
        m_HasSubmittedRequest = false;
        m_NeedsRefresh        = true;
    }

    void HeightfieldRayQuery::PollReadbacks()
    {
        TF3D_PROFILE_SCOPE_DOMAIN("renderer/picking/heightfield-ray-query/readback/poll",
                                  PerformanceMonitor::Domain::Renderer);
        while (const auto completed = m_Readback.Poll()) {
            const auto pending = m_PendingRequests.find(completed->token);
            if (pending == m_PendingRequests.end())
                continue;

            if (completed->data.size() == sizeof(float) * 4) {
                std::array<float, 4> result{};
                std::memcpy(result.data(), completed->data.data(), sizeof(result));
                ApplyRayQueryResult(pending->second, completed->token, result);
            }
            m_PendingRequests.erase(pending);
        }
    }

    void HeightfieldRayQuery::ApplyRayQueryResult(const PendingRequest &pending, uint64_t sequence,
                                                  const std::array<float, 4> &result)
    {
        if (pending.generation != m_Generation || sequence <= m_LastCompletedSequence)
            return;

        m_LastCompletedSequence = sequence;
        if (result[3] < 0.5f || !std::isfinite(result[0]) || !std::isfinite(result[1]) ||
            !std::isfinite(result[2])) {
            m_LastRayHit    = {};
            m_HasLastRayHit = false;
            return;
        }

        const glm::vec3 worldPosition(result[0], result[1], result[2]);
        const glm::vec2 terrainMaximumXZ = pending.request.terrainMinimumXZ + pending.request.terrainWorldSize;
        HeightfieldRayHit hit;
        hit.worldPosition = worldPosition;
        hit.terrainUv     = glm::vec2(
            (worldPosition.x - pending.request.terrainMinimumXZ.x) / pending.request.terrainWorldSize.x,
            (terrainMaximumXZ.y - worldPosition.z) / pending.request.terrainWorldSize.y);
        hit.terrainHeight = worldPosition.y - pending.request.terrainHeightOffset;
        hit.distance      = glm::dot(worldPosition - pending.request.rayOrigin, pending.request.rayDirection);
        if (!std::isfinite(hit.terrainUv.x) || !std::isfinite(hit.terrainUv.y) ||
            !std::isfinite(hit.terrainHeight) || !std::isfinite(hit.distance) || hit.distance < 0.0f) {
            m_LastRayHit    = {};
            m_HasLastRayHit = false;
            return;
        }

        m_LastRayHit    = hit;
        m_HasLastRayHit = true;
    }

    bool HeightfieldRayQuery::QueueReadback(const RayQueryRequest &request)
    {
        const uint64_t sequence = m_NextSequence++;
        {
            TF3D_PROFILE_SCOPE_DOMAIN("renderer/picking/heightfield-ray-query/readback/queue",
                                      PerformanceMonitor::Domain::Renderer);
            TF3D_PROFILE_VALUE_DOMAIN("renderer/picking/heightfield-ray-query/readback-bytes",
                                      sizeof(float) * 4, 0, 0, PerformanceMonitor::Domain::Renderer);
            if (!m_Readback.QueueTexture2D(m_ResultTexture.GetRendererID(), GL_RGBA, GL_FLOAT,
                                           sizeof(float) * 4, sequence))
                return false;
        }
        m_PendingRequests.emplace(sequence, PendingRequest{request, m_Generation});
        return true;
    }

    bool HeightfieldRayQuery::IsSameRequest(const RayQueryRequest &left, const RayQueryRequest &right)
    {
        return left.rayOrigin.x == right.rayOrigin.x && left.rayOrigin.y == right.rayOrigin.y &&
               left.rayOrigin.z == right.rayOrigin.z && left.rayDirection.x == right.rayDirection.x &&
               left.rayDirection.y == right.rayDirection.y && left.rayDirection.z == right.rayDirection.z &&
               left.terrainMinimumXZ.x == right.terrainMinimumXZ.x &&
               left.terrainMinimumXZ.y == right.terrainMinimumXZ.y &&
               left.terrainWorldSize.x == right.terrainWorldSize.x &&
               left.terrainWorldSize.y == right.terrainWorldSize.y &&
               left.terrainHeightOffset == right.terrainHeightOffset && left.heightBias == right.heightBias;
    }

    bool HeightfieldRayQuery::Intersect(uint32_t heightPyramidRendererID, int32_t pyramidLevels,
                                        const glm::vec3 &rayOrigin, const glm::vec3 &rayDirection,
                                        const glm::vec2 &terrainMinimumXZ, const glm::vec2 &terrainWorldSize,
                                        float terrainHeightOffset, HeightfieldRayHit &hit, float heightBias)
    {
        hit = {};
        PollReadbacks();
        if (heightPyramidRendererID == 0 || pyramidLevels <= 0 || !m_Shader.has_value() || !m_Shader->IsValid() ||
            terrainWorldSize.x <= 0.000001f || terrainWorldSize.y <= 0.000001f)
            return false;

        const float directionLength = glm::length(rayDirection);
        if (!std::isfinite(directionLength) || directionLength <= 0.000001f)
            return false;
        if (!std::isfinite(rayOrigin.x) || !std::isfinite(rayOrigin.y) || !std::isfinite(rayOrigin.z) ||
            !std::isfinite(terrainMinimumXZ.x) || !std::isfinite(terrainMinimumXZ.y) ||
            !std::isfinite(terrainWorldSize.x) || !std::isfinite(terrainWorldSize.y) ||
            !std::isfinite(terrainHeightOffset))
            return false;

        const glm::vec3 normalizedDirection = rayDirection / directionLength;
        const RayQueryRequest request{
            rayOrigin,
            normalizedDirection,
            terrainMinimumXZ,
            terrainWorldSize,
            terrainHeightOffset,
            std::max(heightBias, 0.0f),
        };

        if (!m_ResultTexture.Allocate(1, 1, 1, GL_RGBA32F, GL_NEAREST, GL_NEAREST))
            return false;

        const bool requestChanged = m_NeedsRefresh || !m_HasSubmittedRequest ||
                                    !IsSameRequest(request, m_LastSubmittedRequest);
        if (requestChanged && m_Readback.HasAvailableSlot()) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, heightPyramidRendererID);

            m_Shader->Bind();
            m_Shader->SetUniform1i("u_HeightPyramid", 0);
            m_Shader->SetUniform1i("u_PyramidLevels", pyramidLevels);
            m_Shader->SetUniform3f("u_RayOrigin", rayOrigin);
            m_Shader->SetUniform3f("u_RayDirection", normalizedDirection);
            m_Shader->SetUniform2f("u_TerrainMinimumXZ", terrainMinimumXZ);
            m_Shader->SetUniform2f("u_TerrainWorldSize", terrainWorldSize);
            m_Shader->SetUniform1f("u_TerrainHeightOffset", terrainHeightOffset);
            m_Shader->SetUniform1f("u_HeightBias", request.heightBias);

            m_ResultTexture.BindImage(0, GL_WRITE_ONLY, GL_RGBA32F);
            {
                TF3D_PROFILE_GPU_SCOPE("renderer/picking/heightfield-ray-query");
                glDispatchCompute(1, 1, 1);
                TF3D_PROFILE_COUNTER_DOMAIN("gpu/dispatches", 1.0, PerformanceMonitor::Domain::Gpu);
            }
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT |
                            GL_TEXTURE_FETCH_BARRIER_BIT | GL_PIXEL_BUFFER_BARRIER_BIT);
            m_Shader->Unbind();

            if (QueueReadback(request)) {
                m_LastSubmittedRequest = request;
                m_HasSubmittedRequest  = true;
                m_NeedsRefresh         = false;
            }

            glBindTexture(GL_TEXTURE_2D, 0);
            glActiveTexture(GL_TEXTURE0);
            glBindImageTexture(0, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        if (!m_HasLastRayHit)
            return false;
        hit = m_LastRayHit;
        return true;
    }
} // namespace tf3d::generators
