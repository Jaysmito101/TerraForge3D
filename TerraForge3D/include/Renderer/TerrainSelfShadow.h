#pragma once

#include "Base/Base.h"

#include <cstdint>

class ApplicationState;
class ComputeShader;
class GeneratorData;
class HeightfieldPyramid;

class TerrainSelfShadow
{
public:
    explicit TerrainSelfShadow(ApplicationState *appState);
    ~TerrainSelfShadow();

    bool Update(GeneratorData *heightmap, HeightfieldPyramid *heightPyramid,
                uint64_t terrainRevision, const glm::vec3 &sunDirection, float terrainWorldSize);
    void Bind(uint32_t textureSlot) const;

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
    inline uint64_t GetTerrainRevision() const
    {
        return m_TerrainRevision;
    }

private:
    void EnsureTexture(int32_t resolution);
    void ReleaseTexture();

    ApplicationState *m_AppState = nullptr;
    std::shared_ptr<ComputeShader> m_Shader;
    uint32_t m_RendererID      = 0;
    int32_t m_Resolution       = 0;
    bool m_IsReady             = false;
    uint64_t m_TerrainRevision = 0;
    glm::vec3 m_SunDirection   = glm::vec3(0.0f);
    float m_TerrainWorldSize   = 0.0f;
};
