#include "Renderer/HeightfieldGICache.h"

#include "Data/ApplicationState.h"
#include "Data/ResourceManager.h"
#include "Generators/HeightfieldPyramid.h"
#include "Profiler.h"
#include "Utils/Utils.h"

#include <algorithm>

namespace tf3d::renderer
{

    namespace
    {
        constexpr int32_t WorkgroupSize     = 8;
        constexpr int32_t MinimumResolution = 32;
        constexpr int32_t MaximumResolution = 256;
        constexpr int32_t MaximumSamples    = 256;
    } // namespace

    uint64_t HeightfieldGICache::WorkParameters::GetInputHash() const
    {
        uint64_t hash = 14695981039346656037ull;
        auto addFloat = [&hash](float value) {
            HashCombine64(hash, QuantizeFloatForHash(value));
        };
        auto addVector = [&addFloat](const glm::vec3 &value) {
            addFloat(value.x);
            addFloat(value.y);
            addFloat(value.z);
        };

        HashCombine64(hash, reinterpret_cast<uintptr_t>(heightPyramid));
        HashCombine64(hash, terrainRevision);
        addFloat(terrainWorldSize);
        addVector(sunDirection);
        addVector(sunColor);
        addFloat(sunIntensity);
        HashCombine64(hash, hasSkyLight ? 1ull : 0ull);
        addFloat(skyLightIntensity);
        HashCombine64(hash, static_cast<uint64_t>(static_cast<int64_t>(skyRadianceRendererID)));
        HashCombine64(hash, static_cast<uint64_t>(static_cast<int64_t>(skyIrradianceRendererID)));
        HashCombine64(hash, hasTerrainSelfShadow ? 1ull : 0ull);
        HashCombine64(hash, static_cast<uint64_t>(static_cast<int64_t>(terrainSelfShadowRendererID)));
        HashCombine64(hash, static_cast<uint64_t>(outputResolution));
        HashCombine64(hash, static_cast<uint64_t>(targetSamples));
        return hash;
    }

    HeightfieldGICache::HeightfieldGICache(ApplicationState *appState)
        : m_AppState(appState)
    {
        if (m_AppState != nullptr && m_AppState->resourceManager != nullptr) {
            m_Shader       = m_AppState->resourceManager->LoadComputeShader("heightfield/gi/compute", true);
            m_FilterShader = m_AppState->resourceManager->LoadComputeShader("heightfield/gi/filter", true);
        }
    }

    HeightfieldGICache::~HeightfieldGICache()
    {
        m_Worker.reset();
        ReleaseTextures();
    }

    void HeightfieldGICache::ReleaseTextures()
    {
        if (m_AccumulationRendererID != 0)
            glDeleteTextures(1, &m_AccumulationRendererID);
        if (m_RawRendererID != 0)
            glDeleteTextures(1, &m_RawRendererID);
        if (m_RendererID != 0)
            glDeleteTextures(1, &m_RendererID);
        if (m_WorkingRendererID != 0)
            glDeleteTextures(1, &m_WorkingRendererID);
        m_AccumulationRendererID = 0;
        m_RawRendererID          = 0;
        m_RendererID             = 0;
        m_WorkingRendererID      = 0;
        m_Resolution             = 0;
        m_AccumulatedSamples     = 0;
        m_IsReady                = false;
        m_HasPublishedOutput     = false;
    }

    void HeightfieldGICache::EnsureTextures(int32_t resolution)
    {
        resolution = std::clamp(resolution, MinimumResolution, MaximumResolution);
        if (m_AccumulationRendererID != 0 && m_RawRendererID != 0 && m_RendererID != 0 &&
            m_WorkingRendererID != 0 && m_Resolution == resolution) {
            return;
        }

        ReleaseTextures();
        m_Resolution = resolution;

        glGenTextures(1, &m_AccumulationRendererID);
        glBindTexture(GL_TEXTURE_2D, m_AccumulationRendererID);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, resolution, resolution);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        for (uint32_t *texture : {&m_RawRendererID, &m_RendererID, &m_WorkingRendererID}) {
            glGenTextures(1, texture);
            glBindTexture(GL_TEXTURE_2D, *texture);
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, resolution, resolution);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
        m_IsReady            = false;
        m_HasPublishedOutput = false;
    }

    bool HeightfieldGICache::InputsMatch(const WorkParameters &parameters) const
    {
        return m_HasInputKey && m_CurrentInputHash == parameters.GetInputHash();
    }

    bool HeightfieldGICache::Update(HeightfieldPyramid *heightPyramid, uint64_t terrainRevision,
                                    float terrainWorldSize, const glm::vec3 &sunDirection,
                                    const glm::vec3 &sunColor, float sunIntensity,
                                    bool hasSkyLight, float skyLightIntensity,
                                    int32_t skyRadianceRendererID, int32_t skyIrradianceRendererID,
                                    bool hasTerrainSelfShadow, int32_t terrainSelfShadowRendererID,
                                    int32_t resolution, int32_t targetSamples, int32_t samplesPerDispatch)
    {
        TF3D_PROFILE_SCOPE_DOMAIN("renderer/cache/heightfield-gi/check", PerformanceMonitor::Domain::Renderer);
        PollWorkerCompletion();
        if (!m_Enabled)
            return false;
        if (heightPyramid == nullptr || !heightPyramid->IsReady() ||
            !m_Shader || !m_FilterShader || terrainWorldSize <= 0.000001f) {
            return false;
        }

        resolution         = std::clamp(resolution, MinimumResolution, MaximumResolution);
        targetSamples      = std::clamp(targetSamples, 1, MaximumSamples);
        samplesPerDispatch = std::clamp(samplesPerDispatch, 1, 8);

        WorkParameters inputs;
        inputs.heightPyramid               = heightPyramid;
        inputs.terrainRevision             = terrainRevision;
        inputs.terrainWorldSize            = terrainWorldSize;
        inputs.sunDirection                = sunDirection;
        inputs.sunColor                    = sunColor;
        inputs.sunIntensity                = sunIntensity;
        inputs.hasSkyLight                 = hasSkyLight && skyRadianceRendererID >= 0 && skyIrradianceRendererID >= 0;
        inputs.skyLightIntensity           = skyLightIntensity;
        inputs.skyRadianceRendererID       = skyRadianceRendererID;
        inputs.skyIrradianceRendererID     = skyIrradianceRendererID;
        inputs.hasTerrainSelfShadow        = hasTerrainSelfShadow && terrainSelfShadowRendererID >= 0;
        inputs.terrainSelfShadowRendererID = terrainSelfShadowRendererID;
        inputs.outputResolution            = resolution;
        inputs.targetSamples               = targetSamples;

        if (!InputsMatch(inputs)) {
            m_CurrentInputHash   = inputs.GetInputHash();
            m_HasInputKey        = true;
            m_ResetPending       = true;
            m_AccumulatedSamples = 0;
            m_TargetSamples      = targetSamples;
            m_IsReady            = false;
            m_HasPublishedOutput = false;
        }

        {
            std::lock_guard lock(m_WorkMutex);
            if (m_WorkPending || m_WorkComplete)
                return false;
        }
        if (m_Worker == nullptr || !m_Worker->HasContext())
            return false;
        EnsureTextures(resolution);
        if (!m_ResetPending && m_AccumulatedSamples >= targetSamples)
            return false;

        WorkParameters work      = inputs;
        work.sampleStart         = m_ResetPending ? 0 : m_AccumulatedSamples;
        work.samplesThisDispatch = std::min(samplesPerDispatch, targetSamples - work.sampleStart);
        work.resetAccumulation   = m_ResetPending || work.sampleStart == 0;
        work.outputRendererID    = m_WorkingRendererID;
        {
            std::lock_guard lock(m_WorkMutex);
            m_PendingWork = work;
            m_WorkPending = true;
        }
        TF3D_PROFILE_VALUE_DOMAIN("renderer/cache/heightfield-gi/request", resolution,
                                  static_cast<uint64_t>(work.sampleStart), static_cast<uint64_t>(work.samplesThisDispatch),
                                  PerformanceMonitor::Domain::Renderer);
        if (!m_Worker->Request(false)) {
            std::lock_guard lock(m_WorkMutex);
            m_WorkPending = false;
            m_PendingWork = {};
            return false;
        }
        return true;
    }

    void HeightfieldGICache::SetEnabled(bool enabled)
    {
        if (m_Enabled == enabled)
            return;
        m_Enabled = enabled;
        if (!enabled) {
            m_IsReady = false;
            return;
        }

        if (m_Worker == nullptr) {
            m_Worker = std::make_unique<GenerationWorker>("Heightfield GI Worker", [this](bool) { RunWorkerBuild(); }, "renderer/cache/heightfield-gi");
        }
        m_IsReady = m_HasPublishedOutput;
    }

    void HeightfieldGICache::RunWorkerBuild()
    {
        TF3D_PROFILE_SCOPE_DOMAIN("renderer/cache/heightfield-gi/worker", PerformanceMonitor::Domain::Worker);
        WorkParameters work;
        {
            std::lock_guard lock(m_WorkMutex);
            if (!m_WorkPending)
                return;
            work = m_PendingWork;
        }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, work.heightPyramid->GetRendererID());
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, work.skyRadianceRendererID >= 0
                                               ? static_cast<uint32_t>(work.skyRadianceRendererID)
                                               : 0);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_CUBE_MAP, work.skyIrradianceRendererID >= 0
                                               ? static_cast<uint32_t>(work.skyIrradianceRendererID)
                                               : 0);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, work.terrainSelfShadowRendererID >= 0
                                         ? static_cast<uint32_t>(work.terrainSelfShadowRendererID)
                                         : 0);

        m_Shader->Bind();
        m_Shader->SetUniform1i("u_HeightPyramid", 0);
        m_Shader->SetUniform1i("u_SkyRadiance", 1);
        m_Shader->SetUniform1i("u_SkyIrradiance", 2);
        m_Shader->SetUniform1i("u_TerrainSelfShadow", 3);
        m_Shader->SetUniform1i("u_HasSkyLight", work.hasSkyLight ? 1 : 0);
        m_Shader->SetUniform1i("u_HasTerrainSelfShadow", work.hasTerrainSelfShadow ? 1 : 0);
        m_Shader->SetUniform3f("u_SunDirection", work.sunDirection);
        m_Shader->SetUniform3f("u_SunColor", work.sunColor);
        m_Shader->SetUniform1f("u_SunIntensity", work.sunIntensity);
        m_Shader->SetUniform1f("u_SkyLightIntensity", work.skyLightIntensity);
        m_Shader->SetUniform2f("u_TerrainWorldSize", work.terrainWorldSize, work.terrainWorldSize);
        m_Shader->SetUniform1i("u_PyramidLevels", work.heightPyramid->GetMipLevels());
        m_Shader->SetUniform1i("u_OutputResolution", work.outputResolution);
        m_Shader->SetUniform1i("u_SampleStart", work.sampleStart);
        m_Shader->SetUniform1i("u_SamplesThisDispatch", work.samplesThisDispatch);
        m_Shader->SetUniform1i("u_TargetSamples", work.targetSamples);
        m_Shader->SetUniform1i("u_ResetAccumulation", work.resetAccumulation ? 1 : 0);
        m_Shader->SetUniform1f("u_HeightBias", std::max(0.001f,
                                                        work.terrainWorldSize / static_cast<float>(std::max(work.heightPyramid->GetResolution(), 1)) * 1.5f));

        {
            TF3D_PROFILE_GPU_SCOPE("renderer/cache/heightfield-gi/sample-gpu");
            glBindImageTexture(0, m_AccumulationRendererID, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
            glBindImageTexture(1, m_RawRendererID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
            glDispatchCompute((work.outputResolution + WorkgroupSize - 1) / WorkgroupSize,
                              (work.outputResolution + WorkgroupSize - 1) / WorkgroupSize, 1);
            TF3D_PROFILE_COUNTER_DOMAIN("gpu/dispatches", 1.0, PerformanceMonitor::Domain::Gpu);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
        }
        m_Shader->Unbind();
        glBindImageTexture(0, 0, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
        glBindImageTexture(1, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, work.heightPyramid->GetRendererID());
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, m_RawRendererID);
        m_FilterShader->Bind();
        m_FilterShader->SetUniform1i("u_HeightPyramid", 0);
        m_FilterShader->SetUniform1i("u_RawGI", 4);
        m_FilterShader->SetUniform1i("u_OutputResolution", work.outputResolution);
        m_FilterShader->SetUniform2f("u_TerrainWorldSize", work.terrainWorldSize, work.terrainWorldSize);
        m_FilterShader->SetUniform1f("u_HeightSigma", std::max(
                                                          work.terrainWorldSize / static_cast<float>(std::max(work.outputResolution, 1)) * 6.0f,
                                                          0.0001f));
        m_FilterShader->SetUniform1f("u_NormalPower", 4.0f);
        {
            TF3D_PROFILE_GPU_SCOPE("renderer/cache/heightfield-gi/filter-gpu");
            glBindImageTexture(0, work.outputRendererID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
            glDispatchCompute((work.outputResolution + WorkgroupSize - 1) / WorkgroupSize,
                              (work.outputResolution + WorkgroupSize - 1) / WorkgroupSize, 1);
            TF3D_PROFILE_COUNTER_DOMAIN("gpu/dispatches", 1.0, PerformanceMonitor::Domain::Gpu);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
        }
        m_FilterShader->Unbind();
        glBindImageTexture(0, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
        for (int32_t textureSlot = 3; textureSlot >= 0; --textureSlot) {
            glActiveTexture(GL_TEXTURE0 + textureSlot);
            glBindTexture(textureSlot == 1 || textureSlot == 2 ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, 0);
        }
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0);

        {
            std::lock_guard lock(m_WorkMutex);
            m_CompletedWork = work;
            m_WorkComplete  = true;
            m_WorkPending   = false;
        }
    }

    void HeightfieldGICache::PollWorkerCompletion()
    {
        TF3D_PROFILE_SCOPE_DOMAIN("renderer/cache/heightfield-gi/poll", PerformanceMonitor::Domain::Renderer);
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

        if (!m_Enabled || !InputsMatch(completed)) {
            TF3D_PROFILE_COUNTER_DOMAIN_FLOW("renderer/cache/heightfield-gi/stale-result", 1.0,
                                             PerformanceMonitor::Domain::Renderer, requestId);
            TF3D_PROFILE_FLOW_STEP_DOMAIN("renderer/cache/heightfield-gi/request", requestId, "stale",
                                          PerformanceMonitor::Domain::Renderer);
            TF3D_PROFILE_FLOW_END_DOMAIN("renderer/cache/heightfield-gi/request", requestId,
                                         PerformanceMonitor::Domain::Generation);
            m_IsReady            = false;
            m_HasPublishedOutput = false;
            m_AccumulatedSamples = 0;
            m_ResetPending       = true;
            return;
        }

        std::swap(m_RendererID, m_WorkingRendererID);
        m_AccumulatedSamples = completed.sampleStart + completed.samplesThisDispatch;
        m_TargetSamples      = completed.targetSamples;
        m_ResetPending       = false;
        m_HasPublishedOutput = true;
        m_IsReady            = true;
        TF3D_PROFILE_SCOPE_FLOW("renderer/cache/heightfield-gi/publish", PerformanceMonitor::Domain::Renderer, requestId);
        TF3D_PROFILE_FLOW_STEP_DOMAIN("renderer/cache/heightfield-gi/request", requestId, "published",
                                      PerformanceMonitor::Domain::Renderer);
        TF3D_PROFILE_FLOW_END_DOMAIN("renderer/cache/heightfield-gi/request", requestId,
                                     PerformanceMonitor::Domain::Generation);
        TF3D_PROFILE_VALUE_DOMAIN("renderer/cache/heightfield-gi/publish", m_AccumulatedSamples,
                                  m_TargetSamples, 1, PerformanceMonitor::Domain::Renderer);
    }

    void HeightfieldGICache::Bind(uint32_t textureSlot) const
    {
        if (!m_IsReady || m_RendererID == 0)
            return;
        glActiveTexture(GL_TEXTURE0 + textureSlot);
        glBindTexture(GL_TEXTURE_2D, m_RendererID);
    }
} // namespace tf3d::renderer
