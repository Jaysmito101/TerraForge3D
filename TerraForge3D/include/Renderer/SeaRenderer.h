#pragma once

#include "Renderer/RendererBase.h"

#include <cstdint>

class SeaRenderer : public RendererBase
{
public:
	struct Settings
	{
		bool enabled = true;
		float seaLevel = 0.0f;
		float waveAmplitude = 0.022f;
		float waveLength = 0.18f;
		float waveSpeed = 0.16f;
		float waveChoppiness = 0.36f;
		float shoreWidth = 0.055f;
		float deepDepth = 0.48f;
		float normalStrength = 0.20f;
		float normalScale = 4.2f;
		float refractionStrength = 0.006f;
		float reflectionStrength = 0.52f;
		float opacity = 0.96f;
		float foamStrength = 0.36f;
		float foamScale = 5.5f;
		float foamSpeed = 0.24f;
		int32_t gridResolution = 96;
		glm::vec3 shallowColor = glm::vec3(0.012f, 0.19f, 0.23f);
		glm::vec3 deepColor = glm::vec3(0.002f, 0.018f, 0.055f);
		glm::vec3 foamColor = glm::vec3(0.56f, 0.76f, 0.76f);
	};

	explicit SeaRenderer(ApplicationState* appState);
	~SeaRenderer() override;

	void Render(RendererViewport* viewport) override;
	void ShowSettings() override;

	inline bool IsEnabled() const { return m_Settings.enabled; }
	inline Settings& GetSettings() { return m_Settings; }
	inline const Settings& GetSettings() const { return m_Settings; }

protected:
	void ReloadShaders() override;

private:
	void RebuildGrid(int32_t resolution);
	void BuildSideGeometry();
	void BindUniforms(RendererViewport* viewport, float terrainWorldSize,
		float terrainHeightOffset, float seaWorldHeight,
		const glm::vec2& surfaceMinimumXZ, const glm::vec2& surfaceWorldSize);

	Settings m_Settings;
	uint32_t m_Vao = 0;
	uint32_t m_Vbo = 0;
	uint32_t m_Ebo = 0;
	uint32_t m_SideVao = 0;
	uint32_t m_SideVbo = 0;
	uint32_t m_SideEbo = 0;
	int32_t m_GridIndexCount = 0;
	int32_t m_SideIndexCount = 0;
	int32_t m_AllocatedGridResolution = 0;
	float m_ElapsedTime = 0.0f;
};
