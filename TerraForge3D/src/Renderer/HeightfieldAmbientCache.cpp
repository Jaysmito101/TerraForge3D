#include "Renderer/HeightfieldAmbientCache.h"

#include "Data/ApplicationState.h"
#include "Data/ResourceManager.h"
#include "Generators/HeightfieldPyramid.h"
#include "Profiler.h"

#include <algorithm>

namespace tf3d::renderer
{

    namespace
    {
        constexpr int32_t WorkgroupSize     = 16;
        constexpr int32_t MaximumResolution = 512;
    } // namespace

    HeightfieldAmbientCache::HeightfieldAmbientCache(ApplicationState *appState)
        : m_AppState(appState)
    {
        if (m_AppState != nullptr && m_AppState->resourceManager != nullptr) {
            m_GenerateShader = m_AppState->resourceManager->LoadComputeShader("heightfield/ambient/compute", true);
        }
    }

    HeightfieldAmbientCache::~HeightfieldAmbientCache()
    {
        m_Worker.reset();
        ReleaseTextures();
    }

    void HeightfieldAmbientCache::ReleaseTextures()
    {
        if (m_RendererID != 0)
            glDeleteTextures(1, &m_RendererID);
        if (m_WorkingRendererID != 0)
            glDeleteTextures(1, &m_WorkingRendererID);
        m_RendererID        = 0;
        m_WorkingRendererID = 0;
        m_Resolution        = 0;
        m_IsReady           = false;
    }

    void HeightfieldAmbientCache::EnsureTextures(int32_t resolution)
    {
        resolution = std::clamp(resolution, 1, MaximumResolution);
        if (m_RendererID != 0 && m_WorkingRendererID != 0 && m_Resolution == resolution)
            return;

        ReleaseTextures();
        m_Resolution = resolution;

        glGenTextures(1, &m_RendererID);
        glBindTexture(GL_TEXTURE_2D, m_RendererID);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, m_Resolution, m_Resolution);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glGenTextures(1, &m_WorkingRendererID);
        glBindTexture(GL_TEXTURE_2D, m_WorkingRendererID);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, m_Resolution, m_Resolution);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
        m_IsReady = false;
    }

    bool HeightfieldAmbientCache::Update(HeightfieldPyramid *heightPyramid, uint64_t terrainRevision,
                                         float terrainWorldSize, float aoRadius)
    {
        TF3D_PROFILE_SCOPE_DOMAIN("renderer/cache/heightfield-ambient/check", PerformanceMonitor::Domain::Renderer);
        PollWorkerCompletion();
        if (!m_Enabled)
            return false;
        if (heightPyramid == nullptr || !heightPyramid->IsReady() ||
            !m_GenerateShader || terrainWorldSize <= 0.000001f) {
            return false;
        }

        const int32_t requestedResolution = std::clamp(heightPyramid->GetResolution(), 1, MaximumResolution);
        aoRadius                          = std::max(aoRadius, 0.0001f);
        {
            std::lock_guard lock(m_WorkMutex);
            if (m_WorkPending || m_WorkComplete)
                return false;
        }
        if (m_Worker == nullptr || !m_Worker->HasContext())
            return false;

        const bool requiresRebuild = !m_IsReady ||
                                     m_TerrainRevision != terrainRevision ||
                                     std::abs(m_TerrainWorldSize - terrainWorldSize) > 0.00001f ||
                                     std::abs(m_AoRadius - aoRadius) > 0.00001f ||
                                     m_Resolution != requestedResolution;
        if (!requiresRebuild)
            return false;

        TF3D_PROFILE_VALUE_DOMAIN("renderer/cache/heightfield-ambient/request", requestedResolution,
                                  terrainRevision, static_cast<uint64_t>(m_IsReady ? 1 : 0),
                                  PerformanceMonitor::Domain::Renderer);
        EnsureTextures(requestedResolution);

        WorkParameters work;
        work.heightPyramid    = heightPyramid;
        work.terrainRevision  = terrainRevision;
        work.terrainWorldSize = terrainWorldSize;
        work.aoRadius         = aoRadius;
        work.outputRendererID = m_WorkingRendererID;
        work.outputResolution = m_Resolution;
        {
            std::lock_guard lock(m_WorkMutex);
            m_PendingWork = work;
            m_WorkPending = true;
        }
        if (!m_Worker->Request(false)) {
            std::lock_guard lock(m_WorkMutex);
            m_WorkPending = false;
            m_PendingWork = {};
            return false;
        }
        return false;
    }

    void HeightfieldAmbientCache::SetEnabled(bool enabled)
    {
        m_Enabled = enabled;
        if (!enabled)
            m_IsReady = false;
        else if (m_Worker == nullptr) {
            m_Worker = std::make_unique<GenerationWorker>("Heightfield Ambient Worker", [this](bool) { RunWorkerBuild(); }, "renderer/cache/heightfield-ambient");
        }
    }

    void HeightfieldAmbientCache::RunWorkerBuild()
    {
        TF3D_PROFILE_SCOPE_DOMAIN("renderer/cache/heightfield-ambient/worker", PerformanceMonitor::Domain::Worker);
        WorkParameters work;
        {
            std::lock_guard lock(m_WorkMutex);
            if (!m_WorkPending)
                return;
            work = m_PendingWork;
        }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, work.heightPyramid->GetRendererID());

        m_GenerateShader->Bind();
        m_GenerateShader->SetUniform1i("u_HeightPyramid", 0);
        m_GenerateShader->SetUniform1i("u_PyramidLevels", work.heightPyramid->GetMipLevels());
        m_GenerateShader->SetUniform1i("u_OutputResolution", work.outputResolution);
        m_GenerateShader->SetUniform2f("u_TerrainWorldSize", work.terrainWorldSize, work.terrainWorldSize);
        m_GenerateShader->SetUniform1f("u_AoRadius", work.aoRadius);
        m_GenerateShader->SetUniform1f("u_HeightBias", std::max(0.001f,
                                                                work.terrainWorldSize / static_cast<float>(work.outputResolution) * 1.5f));
        {
            TF3D_PROFILE_GPU_SCOPE("renderer/cache/heightfield-ambient/gpu");
            glBindImageTexture(0, work.outputRendererID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
            glDispatchCompute((work.outputResolution + WorkgroupSize - 1) / WorkgroupSize,
                              (work.outputResolution + WorkgroupSize - 1) / WorkgroupSize, 1);
            TF3D_PROFILE_COUNTER_DOMAIN("gpu/dispatches", 1.0, PerformanceMonitor::Domain::Gpu);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
        }
        m_GenerateShader->Unbind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        {
            TF3D_PROFILE_SCOPE_DOMAIN("renderer/cache/heightfield-ambient/finish", PerformanceMonitor::Domain::Wait);
            glFinish();
        }

        {
            std::lock_guard lock(m_WorkMutex);
            m_CompletedWork = work;
            m_WorkComplete  = true;
            m_WorkPending   = false;
        }
    }

    void HeightfieldAmbientCache::PollWorkerCompletion()
    {
        TF3D_PROFILE_SCOPE_DOMAIN("renderer/cache/heightfield-ambient/poll", PerformanceMonitor::Domain::Renderer);
        if (m_Worker == nullptr || m_Worker->IsRunning() || m_Worker->IsRequestPending())
            return;

        WorkParameters completed;
        {
            std::lock_guard lock(m_WorkMutex);
            if (!m_WorkComplete)
                return;
            completed      = m_CompletedWork;
            m_WorkComplete = false;
        }
        const uint64_t requestId = m_Worker->GetCompletedRequestId();
        m_Worker->ConsumeCompleted();
        std::swap(m_RendererID, m_WorkingRendererID);
        m_TerrainRevision  = completed.terrainRevision;
        m_TerrainWorldSize = completed.terrainWorldSize;
        m_AoRadius         = completed.aoRadius;
        m_IsReady          = m_Enabled;
        TF3D_PROFILE_SCOPE_FLOW("renderer/cache/heightfield-ambient/publish", PerformanceMonitor::Domain::Renderer, requestId);
        TF3D_PROFILE_FLOW_STEP_DOMAIN("renderer/cache/heightfield-ambient/request", requestId, "published",
                                      PerformanceMonitor::Domain::Renderer);
        TF3D_PROFILE_FLOW_END_DOMAIN("renderer/cache/heightfield-ambient/request", requestId,
                                     PerformanceMonitor::Domain::Generation);
        TF3D_PROFILE_INSTANT_DOMAIN("renderer/cache/heightfield-ambient/publish", PerformanceMonitor::Domain::Renderer);
    }

    void HeightfieldAmbientCache::Bind(uint32_t textureSlot) const
    {
        if (!m_IsReady || m_RendererID == 0)
            return;
        glActiveTexture(GL_TEXTURE0 + textureSlot);
        glBindTexture(GL_TEXTURE_2D, m_RendererID);
    }

} // namespace tf3d::renderer
