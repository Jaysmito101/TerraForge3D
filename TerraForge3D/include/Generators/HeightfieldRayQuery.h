#pragma once

#include "Base/AsyncTextureReadback.h"
#include "Base/Shader.h"
#include "Base/Texture2DStorage.h"

#include <array>
#include <cstdint>
#include <optional>
#include <unordered_map>

namespace tf3d::data
{
    class ResourceManager;
}

namespace tf3d::generators
{
    struct HeightfieldRayHit {
        glm::vec3 worldPosition{0.0f};
        glm::vec2 terrainUv{0.0f};
        float terrainHeight = 0.0f;
        float distance      = 0.0f;
    };

    class HeightfieldRayQuery
    {
    public:
        explicit HeightfieldRayQuery(data::ResourceManager *resourceManager);
        ~HeightfieldRayQuery();

        HeightfieldRayQuery(const HeightfieldRayQuery &)            = delete;
        HeightfieldRayQuery &operator=(const HeightfieldRayQuery &) = delete;
        HeightfieldRayQuery(HeightfieldRayQuery &&)                 = delete;
        HeightfieldRayQuery &operator=(HeightfieldRayQuery &&)      = delete;

        bool Intersect(uint32_t heightPyramidRendererID, int32_t pyramidLevels,
                       const glm::vec3 &rayOrigin, const glm::vec3 &rayDirection,
                       const glm::vec2 &terrainMinimumXZ, const glm::vec2 &terrainWorldSize,
                       float terrainHeightOffset, HeightfieldRayHit &hit, float heightBias = 0.0001f);
        void Invalidate();

    private:
        struct RayQueryRequest {
            glm::vec3 rayOrigin{0.0f};
            glm::vec3 rayDirection{0.0f};
            glm::vec2 terrainMinimumXZ{0.0f};
            glm::vec2 terrainWorldSize{0.0f};
            float terrainHeightOffset = 0.0f;
            float heightBias          = 0.0f;
        };

        struct PendingRequest {
            RayQueryRequest request;
            uint64_t generation = 0;
        };

        void PollReadbacks();
        void ApplyRayQueryResult(const PendingRequest &pending, uint64_t sequence,
                                 const std::array<float, 4> &result);
        bool QueueReadback(const RayQueryRequest &request);
        static bool IsSameRequest(const RayQueryRequest &left, const RayQueryRequest &right);

        std::optional<base::ComputeShader> m_Shader;
        base::Texture2DStorage m_ResultTexture;
        base::AsyncTextureReadback m_Readback;
        std::unordered_map<uint64_t, PendingRequest> m_PendingRequests;
        RayQueryRequest m_LastSubmittedRequest;
        HeightfieldRayHit m_LastRayHit;
        uint64_t m_NextSequence          = 1;
        uint64_t m_LastCompletedSequence = 0;
        uint64_t m_Generation            = 1;
        bool m_HasLastRayHit             = false;
        bool m_HasSubmittedRequest       = false;
        bool m_NeedsRefresh              = true;
    };
} // namespace tf3d::generators
