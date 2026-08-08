#pragma once

#include "Base/Base.h"
#include "Generators/HeightfieldRayQuery.h"

#include <cstdint>

namespace tf3d::data
{
    class ApplicationState;
}
using tf3d::data::ApplicationState;

namespace tf3d::generators
{

    class GeneratorData;

    class HeightfieldPyramid
    {
    public:
        explicit HeightfieldPyramid(ApplicationState *appState);
        ~HeightfieldPyramid();

        bool Rebuild(GeneratorData *heightmap);

        bool IntersectWorldRay(const glm::vec3 &rayOrigin,
                               const glm::vec3 &rayDirection,
                               const glm::vec2 &terrainMinimumXZ,
                               const glm::vec2 &terrainWorldSize,
                               float terrainHeightOffset,
                               HeightfieldRayHit &hit,
                               float heightBias = 0.0001f);

        inline uint32_t GetRendererID() const
        {
            return m_RendererID;
        }
        inline int32_t GetResolution() const
        {
            return m_Resolution;
        }
        inline int32_t GetMipLevels() const
        {
            return m_MipLevels;
        }
        inline bool IsReady() const
        {
            return m_IsReady;
        }

    private:
        void EnsureTexture(int32_t resolution);
        void ReleaseTexture();

        data::ApplicationState *m_AppState = nullptr;
        std::optional<base::ComputeShader> m_Shader;
        HeightfieldRayQuery m_RayQuery;
        uint32_t m_RendererID = 0;
        int32_t m_Resolution  = 0;
        int32_t m_MipLevels   = 0;
        bool m_IsReady        = false;
    };

} // namespace tf3d::generators
using tf3d::generators::HeightfieldPyramid;
