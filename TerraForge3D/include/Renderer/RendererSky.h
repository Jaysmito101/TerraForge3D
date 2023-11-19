#pragma once

#include "Renderer/RendererViewport.h"
#include "Base/Base.h"

class ApplicationState;

class RendererSky
{
public:
	RendererSky(ApplicationState* appState);
	~RendererSky();

	void ShowSettings();
	void Render(RendererViewport* viewport);

	inline int32_t GetIrradianceMap() { return m_IrradianceMapTextureID; }
	inline int32_t GetSkyboxMap() { return m_SkyboxTextureID; }
	inline bool IsSkyReady() { return m_IsSkyReady; }

private:
	bool LoadSkyboxTexture(const std::string& path);
	void ReloadShaders();

private:
	ApplicationState* m_AppState = nullptr;
	std::shared_ptr<ComputeShader> m_EquirectToCube = nullptr;
	std::shared_ptr<ComputeShader> m_SpecularMap = nullptr;
	std::shared_ptr<ComputeShader> m_IrradianceMap = nullptr;
	std::shared_ptr<Shader> m_SkyboxShader = nullptr;
	Model* m_SkyboxModel = nullptr;
	bool m_RenderSky = true;
	bool m_IsSkyReady = false;
	std::string m_SkyboxTexturePath = "";
	uint32_t m_SkyboxTextureID = -1;
	uint32_t m_IrradianceMapTextureID = -1;
	int32_t m_SkyboxSize = 512;
	int32_t m_IrradianceMapSize = 32;
	int32_t temp = 0;
};