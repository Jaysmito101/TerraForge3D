#pragma once

#include "Base/Base.h"

#include <cstdint>

class ApplicationState;
class ComputeShader;
class HeightfieldPyramid;

class PlanarShadowCache
{
public:
	 explicit PlanarShadowCache(ApplicationState* appState);
	~PlanarShadowCache();

	bool Update(HeightfieldPyramid* heightPyramid, uint64_t terrainRevision,
		const glm::vec3& sunDirection, const glm::vec2& terrainMinimumXZ,
		float terrainWorldSize, float terrainHeightOffset, float terrainMaximumHeight,
		float receiverHeight = 0.0f);
	void Bind(uint32_t textureSlot) const;

	inline uint32_t GetRendererID() const { return m_RendererID; }
	inline int32_t GetResolution() const { return m_Resolution; }
	inline bool IsReady() const { return m_IsReady; }
	inline const glm::vec2& GetAtlasMinimumXZ() const { return m_AtlasMinimumXZ; }
	inline const glm::vec2& GetAtlasWorldSize() const { return m_AtlasWorldSize; }

private:
	void EnsureTexture(int32_t resolution);
	void ReleaseTexture();

	ApplicationState* m_AppState = nullptr;
	std::shared_ptr<ComputeShader> m_Shader;
	uint32_t m_RendererID = 0;
	int32_t m_Resolution = 0;
	bool m_IsReady = false;
	uint64_t m_TerrainRevision = 0;
	glm::vec3 m_SunDirection = glm::vec3(0.0f);
	glm::vec2 m_AtlasMinimumXZ = glm::vec2(0.0f);
	glm::vec2 m_AtlasWorldSize = glm::vec2(0.0f);
	float m_TerrainWorldSize = 0.0f;
	float m_TerrainHeightOffset = 0.0f;
	float m_TerrainMaximumHeight = 0.0f;
	float m_ReceiverHeight = 0.0f;
};
