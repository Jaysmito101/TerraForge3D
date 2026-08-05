#pragma once

#include "Base/Base.h"

#include <cstdint>

class ApplicationState;
class GeneratorData;

class HeightfieldPyramid
{
public:
	 explicit HeightfieldPyramid(ApplicationState* appState);
	~HeightfieldPyramid();

	bool Rebuild(GeneratorData* heightmap);

	inline uint32_t GetRendererID() const { return m_RendererID; }
	inline int32_t GetResolution() const { return m_Resolution; }
	inline int32_t GetMipLevels() const { return m_MipLevels; }
	inline bool IsReady() const { return m_IsReady; }

private:
	void EnsureTexture(int32_t resolution);
	void ReleaseTexture();

	ApplicationState* m_AppState = nullptr;
	std::shared_ptr<ComputeShader> m_Shader;
	uint32_t m_RendererID = 0;
	int32_t m_Resolution = 0;
	int32_t m_MipLevels = 0;
	bool m_IsReady = false;
};
