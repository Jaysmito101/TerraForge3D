#include "Renderer/TerrainSelfShadow.h"

#include "Data/ApplicationState.h"
#include "Data/ResourceManager.h"
#include "Generators/GeneratorData.h"
#include "Generators/HeightfieldPyramid.h"

#include <algorithm>

namespace
{
    constexpr int32_t WorkgroupSize = 8;
}

TerrainSelfShadow::TerrainSelfShadow(ApplicationState *appState)
    : m_AppState(appState)
{
    if (m_AppState != nullptr && m_AppState->resourceManager != nullptr) {
        m_Shader = m_AppState->resourceManager->LoadComputeShader("heightfield/self_shadow/compute", true);
    }
}

TerrainSelfShadow::~TerrainSelfShadow()
{
    ReleaseTexture();
}

void TerrainSelfShadow::ReleaseTexture()
{
    if (m_RendererID != 0) {
        glDeleteTextures(1, &m_RendererID);
        m_RendererID = 0;
    }
    m_Resolution = 0;
    m_IsReady    = false;
}

void TerrainSelfShadow::EnsureTexture(int32_t resolution)
{
    if (resolution <= 0)
        return;
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

bool TerrainSelfShadow::Update(GeneratorData *heightmap, HeightfieldPyramid *heightPyramid,
                               uint64_t terrainRevision, const glm::vec3 &sunDirection, float terrainWorldSize)
{
    if (heightmap == nullptr || heightPyramid == nullptr || !heightPyramid->IsReady() ||
        heightmap->GetResolution() <= 0 || m_Shader == nullptr) {
        return false;
    }

    const float directionLength = glm::length(sunDirection);
    if (directionLength <= 0.000001f || terrainWorldSize <= 0.000001f)
        return false;

    const glm::vec3 normalizedSunDirection = sunDirection / directionLength;
    const bool requiresRebuild             = !m_IsReady ||
                                 m_TerrainRevision != terrainRevision ||
                                 glm::length(normalizedSunDirection - m_SunDirection) > 0.00001f ||
                                 std::abs(m_TerrainWorldSize - terrainWorldSize) > 0.00001f ||
                                 m_Resolution != heightmap->GetResolution();
    if (!requiresRebuild)
        return false;

    EnsureTexture(heightmap->GetResolution());

    heightmap->BindAsTexture(0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, heightPyramid->GetRendererID());

    m_Shader->Bind();
    m_Shader->SetUniform1i("u_Heightmap", 0);
    m_Shader->SetUniform1i("u_HeightPyramid", 1);
    m_Shader->SetUniform1i("u_PyramidLevels", heightPyramid->GetMipLevels());
    m_Shader->SetUniform1i("u_OutputResolution", m_Resolution);
    m_Shader->SetUniform3f("u_LightDirection", -normalizedSunDirection);
    m_Shader->SetUniform2f("u_TerrainWorldSize", terrainWorldSize, terrainWorldSize);
    m_Shader->SetUniform1f("u_HeightBias", std::max(0.001f, terrainWorldSize / static_cast<float>(m_Resolution) * 1.5f));

    glBindImageTexture(0, m_RendererID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8);
    glDispatchCompute((m_Resolution + WorkgroupSize - 1) / WorkgroupSize,
                      (m_Resolution + WorkgroupSize - 1) / WorkgroupSize, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_RendererID);
    glGenerateMipmap(GL_TEXTURE_2D);
    glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

    m_Shader->Unbind();
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_TerrainRevision  = terrainRevision;
    m_SunDirection     = normalizedSunDirection;
    m_TerrainWorldSize = terrainWorldSize;
    m_IsReady          = true;
    return true;
}

void TerrainSelfShadow::Bind(uint32_t textureSlot) const
{
    if (!m_IsReady || m_RendererID == 0)
        return;
    glActiveTexture(GL_TEXTURE0 + textureSlot);
    glBindTexture(GL_TEXTURE_2D, m_RendererID);
}
