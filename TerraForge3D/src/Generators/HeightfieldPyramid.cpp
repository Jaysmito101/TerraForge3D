#include "Generators/HeightfieldPyramid.h"

#include "Data/ApplicationState.h"
#include "Data/ResourceManager.h"
#include "Generators/GeneratorData.h"

#include <algorithm>

	constexpr int32_t WorkgroupSize = 16;

HeightfieldPyramid::HeightfieldPyramid(ApplicationState* appState)
	: m_AppState(appState)
{
	if (m_AppState != nullptr && m_AppState->resourceManager != nullptr)
	{
		m_Shader = m_AppState->resourceManager->LoadComputeShader("heightfield/minmax_pyramid/compute", true);
	}
}

HeightfieldPyramid::~HeightfieldPyramid()
{
	ReleaseTexture();
}

void HeightfieldPyramid::ReleaseTexture()
{
	if (m_RendererID != 0)
	{
		glDeleteTextures(1, &m_RendererID);
		m_RendererID = 0;
	}
	m_Resolution = 0;
	m_MipLevels = 0;
	m_IsReady = false;
}

void HeightfieldPyramid::EnsureTexture(int32_t resolution)
{
	if (resolution <= 0) return;

	int32_t mipLevels = 1;
	for (int32_t mipSize = resolution; mipSize > 1; mipSize >>= 1) ++mipLevels;
	if (m_RendererID != 0 && m_Resolution == resolution && m_MipLevels == mipLevels) return;

	ReleaseTexture();
	m_Resolution = resolution;
	m_MipLevels = mipLevels;

	glGenTextures(1, &m_RendererID);
	glBindTexture(GL_TEXTURE_2D, m_RendererID);
	glTexStorage2D(GL_TEXTURE_2D, m_MipLevels, GL_RG32F, m_Resolution, m_Resolution);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, m_MipLevels - 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
	m_IsReady = false;
}

bool HeightfieldPyramid::Rebuild(GeneratorData* heightmap)
{
	if (heightmap == nullptr || heightmap->GetResolution() <= 0 || m_Shader == nullptr) return false;

	const int32_t resolution = heightmap->GetResolution();
	EnsureTexture(resolution);

	heightmap->BindAsTexture(0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_RendererID);

	m_Shader->Bind();
	m_Shader->SetUniform1i("u_Heightmap", 0);
	m_Shader->SetUniform1i("u_Pyramid", 1);

	int32_t sourceWidth = resolution;
	int32_t sourceHeight = resolution;
	for (int32_t level = 0; level < m_MipLevels; ++level)
	{
		const bool sourceIsHeightmap = level == 0;
		const int32_t outputWidth = sourceIsHeightmap ? resolution : std::max(sourceWidth / 2, 1);
		const int32_t outputHeight = sourceIsHeightmap ? resolution : std::max(sourceHeight / 2, 1);

		m_Shader->SetUniform1i("u_SourceIsHeightmap", sourceIsHeightmap ? 1 : 0);
		m_Shader->SetUniform1i("u_SourceLevel", sourceIsHeightmap ? 0 : level - 1);
		m_Shader->SetUniform2f("u_SourceSize", static_cast<float>(sourceWidth), static_cast<float>(sourceHeight));
		m_Shader->SetUniform2f("u_OutputSize", static_cast<float>(outputWidth), static_cast<float>(outputHeight));
		glBindImageTexture(0, m_RendererID, level, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
		glDispatchCompute((outputWidth + WorkgroupSize - 1) / WorkgroupSize,
			(outputHeight + WorkgroupSize - 1) / WorkgroupSize, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

		sourceWidth = outputWidth;
		sourceHeight = outputHeight;
	}

	m_Shader->Unbind();
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	m_IsReady = true;
	return true;
}
