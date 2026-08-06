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
    class HeightfieldAmbientCache
    {
    public:
        explicit HeightfieldAmbientCache(ApplicationState *appState);
        ~HeightfieldAmbientCache();

        bool Update(HeightfieldPyramid *heightPyramid, uint64_t terrainRevision,
                    float terrainWorldSize, float aoRadius);
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
            HeightfieldPyramid *heightPyramid = nullptr;
            uint64_t terrainRevision          = 0;
            float terrainWorldSize            = 0.0f;
            float aoRadius                    = 0.0f;
            uint32_t outputRendererID         = 0;
            int32_t outputResolution          = 0;
        };

        void EnsureTextures(int32_t resolution);
        void ReleaseTextures();
        void RunWorkerBuild();
        void PollWorkerCompletion();

        ApplicationState *m_AppState = nullptr;
        std::shared_ptr<ComputeShader> m_GenerateShader;
        std::unique_ptr<GenerationWorker> m_Worker;
        uint32_t m_RendererID        = 0;
        uint32_t m_WorkingRendererID = 0;
        int32_t m_Resolution         = 0;
        bool m_IsReady               = false;
        bool m_Enabled               = false;
        uint64_t m_TerrainRevision   = 0;
        float m_TerrainWorldSize     = 0.0f;
        float m_AoRadius             = 0.0f;
        std::mutex m_WorkMutex;
        WorkParameters m_PendingWork;
        WorkParameters m_CompletedWork;
        bool m_WorkPending  = false;
        bool m_WorkComplete = false;
    };
} // namespace tf3d::renderer