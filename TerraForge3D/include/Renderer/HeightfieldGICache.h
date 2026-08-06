#pragma once

#include "Base/Base.h"
#include "Generators/GenerationWorker.h"

#include <cstdint>
#include <mutex>

TF3D_FWD_DEC_CLASS(ApplicationState, tf3d::data)
TF3D_FWD_DEC_CLASS(ComputeShader, tf3d::base)
TF3D_FWD_DEC_CLASS(HeightfieldPyramid, tf3d::generators)

namespace tf3d::renderer
{
    class HeightfieldGICache
    {
    public:
        explicit HeightfieldGICache(ApplicationState *appState);
        ~HeightfieldGICache();

        bool Update(HeightfieldPyramid *heightPyramid, uint64_t terrainRevision,
                    float terrainWorldSize, const glm::vec3 &sunDirection,
                    const glm::vec3 &sunColor, float sunIntensity,
                    bool hasSkyLight, float skyLightIntensity,
                    int32_t skyRadianceRendererID, int32_t skyIrradianceRendererID,
                    bool hasTerrainSelfShadow, int32_t terrainSelfShadowRendererID,
                    int32_t resolution, int32_t targetSamples, int32_t samplesPerDispatch);
        void Bind(uint32_t textureSlot) const;
        void SetEnabled(bool enabled);

        inline uint32_t GetRendererID() const
        {
            return m_RendererID;
        }
        inline int32_t GetResolution() const
        {
            return m_Resolution;
        }
        inline int32_t GetAccumulatedSamples() const
        {
            return m_AccumulatedSamples;
        }
        inline int32_t GetTargetSamples() const
        {
            return m_TargetSamples;
        }
        inline float GetProgress() const
        {
            return m_TargetSamples > 0
                       ? std::clamp(static_cast<float>(m_AccumulatedSamples) / static_cast<float>(m_TargetSamples), 0.0f, 1.0f)
                       : 0.0f;
        }
        inline bool IsReady() const
        {
            return m_IsReady;
        }
        inline bool IsEnabled() const
        {
            return m_Enabled;
        }

    private:
        struct WorkParameters {
            uint64_t GetInputHash() const;

            HeightfieldPyramid *heightPyramid   = nullptr;
            uint64_t terrainRevision            = 0;
            float terrainWorldSize              = 0.0f;
            glm::vec3 sunDirection              = glm::vec3(0.0f);
            glm::vec3 sunColor                  = glm::vec3(0.0f);
            float sunIntensity                  = 0.0f;
            bool hasSkyLight                    = false;
            float skyLightIntensity             = 0.0f;
            int32_t skyRadianceRendererID       = -1;
            int32_t skyIrradianceRendererID     = -1;
            bool hasTerrainSelfShadow           = false;
            int32_t terrainSelfShadowRendererID = -1;
            int32_t sampleStart                 = 0;
            int32_t samplesThisDispatch         = 0;
            int32_t targetSamples               = 0;
            bool resetAccumulation              = false;
            uint32_t outputRendererID           = 0;
            int32_t outputResolution            = 0;
        };

        void EnsureTextures(int32_t resolution);
        void ReleaseTextures();
        void RunWorkerBuild();
        void PollWorkerCompletion();
        bool InputsMatch(const WorkParameters &parameters) const;

        ApplicationState *m_AppState = nullptr;
        std::shared_ptr<ComputeShader> m_Shader;
        std::shared_ptr<ComputeShader> m_FilterShader;
        std::unique_ptr<GenerationWorker> m_Worker;
        uint32_t m_AccumulationRendererID = 0;
        uint32_t m_RawRendererID          = 0;
        uint32_t m_RendererID             = 0;
        uint32_t m_WorkingRendererID      = 0;
        int32_t m_Resolution              = 0;
        int32_t m_AccumulatedSamples      = 0;
        int32_t m_TargetSamples           = 0;
        bool m_IsReady                    = false;
        bool m_Enabled                    = false;
        bool m_HasInputKey                = false;
        bool m_ResetPending               = true;
        bool m_HasPublishedOutput         = false;
        uint64_t m_CurrentInputHash       = 0;
        std::mutex m_WorkMutex;
        WorkParameters m_PendingWork;
        WorkParameters m_CompletedWork;
        bool m_WorkPending  = false;
        bool m_WorkComplete = false;
    };
} // namespace tf3d::renderer