#include "Renderer/RendererSky.h"
#include "Base/Base.h"
#include "UI/ImGuiComponents.h"
#include "Utils/Utils.h"
#include "Data/ApplicationState.h"

#include "stb/stb_image.h"

RendererSky::RendererSky(ApplicationState* appState)
{
	m_AppState = appState;
	m_SkyboxModel = new Model("Skybox");
	m_SkyboxModel->mesh->GenerateCube();
	m_SkyboxModel->mesh->RecalculateNormals();
	m_SkyboxModel->SetupMeshOnGPU();
	m_SkyboxModel->UploadToGPU();
	ReloadShaders();


	LoadSkyboxTexture(m_AppState->constants.texturesDir + PATH_SEPARATOR "default_sky.hdr");
}

RendererSky::~RendererSky()
{
	if (m_SkyboxTextureID > -1) glDeleteTextures(1, &m_SkyboxTextureID);
	if (m_IrradianceMapTextureID > -1) glDeleteTextures(1, &m_IrradianceMapTextureID);
	if (m_SpecularMapTextureID > -1) glDeleteTextures(1, &m_SpecularMapTextureID);
	if (m_BrdfLutTextureID > -1) glDeleteTextures(1, &m_BrdfLutTextureID);
	delete m_SkyboxModel;
}

void RendererSky::ShowSettings()
{
	ImGui::Text("Is Sky Ready : %s", m_IsSkyReady ? "Yes" : "No");
	ImGui::Checkbox("Render Sky", &m_RenderSky);
	if (ImGui::Button("Load Sky Texture"))
	{
		auto path = ShowOpenFileDialog("*.hdr");
		if (path.size() > 3) LoadSkyboxTexture(path); 
	}
	if (ImGui::Button("Reload Shaders")) ReloadShaders();
	if (PowerOfTwoDropDown("Environment Map Size", &m_SkyboxSize, 6, 12)) LoadSkyboxTexture(m_SkyboxTexturePath);
	if (PowerOfTwoDropDown("Irradiance Map Size", &m_IrradianceMapSize, 4, 8)) LoadSkyboxTexture(m_SkyboxTexturePath);
}
 
void RendererSky::Render(RendererViewport* viewport)
{
	if (!m_IsSkyReady || !m_RenderSky) return; 
	glDisable(GL_DEPTH_TEST);
	m_SkyboxShader->Bind();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_SkyboxTextureID);
	glUniform1i(glGetUniformLocation(m_SkyboxShader->GetNativeShader(), "u_Skybox"), 0);
	glm::mat4 mpv = viewport->m_Camera.GetProjectionMatrix() * glm::mat4(glm::mat3(viewport->m_Camera.GetViewMatrix()));
	glUniformMatrix4fv(glGetUniformLocation(m_SkyboxShader->GetNativeShader(), "u_MPV"), 1, GL_FALSE, glm::value_ptr(mpv));
	m_SkyboxModel->Render();
	glEnable(GL_DEPTH_TEST);
}

void RendererSky::ReloadShaders()
{
	m_EquirectToCube = m_AppState->resourceManager->LoadComputeShader("equirect_to_cube/compute", true);
	m_SpecularMap = m_AppState->resourceManager->LoadComputeShader("sky_specular_map/compute", true);
	m_IrradianceMap = m_AppState->resourceManager->LoadComputeShader("sky_irradiance_map/compute", true);
	m_BrdfLut = m_AppState->resourceManager->LoadComputeShader("sky_brdf_lut/compute", true);
	m_SkyboxShader = m_AppState->resourceManager->LoadShader("skybox", true);
}

// From : https://github.com/Nadrin/PBR/blob/master/src/opengl.cpp
bool RendererSky::LoadSkyboxTexture(const std::string& path)
{
	if (path.size() < 3) return false;
	m_IsSkyReady = false;
	if (m_SkyboxTextureID > -1) glDeleteTextures(1, &m_SkyboxTextureID);
	if (m_IrradianceMapTextureID > -1) glDeleteTextures(1, &m_IrradianceMapTextureID);
	if (m_SpecularMapTextureID > -1) glDeleteTextures(1, &m_SpecularMapTextureID);
	if (m_BrdfLutTextureID > -1) glDeleteTextures(1, &m_BrdfLutTextureID);

	TF3D_LOG_DEBUG("Loading skybox texture '{}'", path);

	int32_t skyboxTextureEquirectWidth = 0, skyboxTextureEquirectHeight = 0;
	int32_t skyboxTextureEquirectChannels = 0;
	const bool isHDR = stbi_is_hdr(path.c_str()) != 0;
	float* skyboxTextureEquirectFloatData = nullptr;
	unsigned char* skyboxTextureEquirectByteData = nullptr;
	if (isHDR)
	{
		skyboxTextureEquirectFloatData = stbi_loadf(path.c_str(), &skyboxTextureEquirectWidth, &skyboxTextureEquirectHeight, &skyboxTextureEquirectChannels, 4);
	}
	else
	{
		skyboxTextureEquirectByteData = stbi_load(path.c_str(), &skyboxTextureEquirectWidth, &skyboxTextureEquirectHeight, &skyboxTextureEquirectChannels, 3);
	}
	void* skyboxTextureEquirectData = isHDR
		? static_cast<void*>(skyboxTextureEquirectFloatData)
		: static_cast<void*>(skyboxTextureEquirectByteData);
	if (!skyboxTextureEquirectData)
	{
		TF3D_LOG_ERROR("Failed to load skybox texture '{}'", path);
		return false;
	}
	TF3D_LOG_DEBUG("Loaded {} skybox texture '{}' ({}x{}, {} channels)", isHDR ? "floating-point HDR" : "8-bit LDR", path, skyboxTextureEquirectWidth, skyboxTextureEquirectHeight, skyboxTextureEquirectChannels);
	uint32_t skyboxTextureEquirect = -1;
	glGenTextures(1, &skyboxTextureEquirect);
	glBindTexture(GL_TEXTURE_2D, skyboxTextureEquirect);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1); glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	if (isHDR)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, skyboxTextureEquirectWidth, skyboxTextureEquirectHeight, 0, GL_RGBA, GL_FLOAT, skyboxTextureEquirectData);
	}
	else
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, skyboxTextureEquirectWidth, skyboxTextureEquirectHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, skyboxTextureEquirectData);
	}
	stbi_image_free(skyboxTextureEquirectData);

	uint32_t skyboxTextureUnfiltered = -1;
	glGenTextures(1, &skyboxTextureUnfiltered);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTextureUnfiltered);
	int32_t skyboxMipLevels = 1;
	for (int32_t mipSize = m_SkyboxSize; mipSize > 1; mipSize >>= 1) ++skyboxMipLevels;
	glTexStorage2D(GL_TEXTURE_CUBE_MAP, skyboxMipLevels, GL_RGBA32F, m_SkyboxSize, m_SkyboxSize);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	
	glBindImageTexture(0, skyboxTextureUnfiltered, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);
	m_EquirectToCube->Bind();
	glActiveTexture(GL_TEXTURE0 + 1); 
	glBindTexture(GL_TEXTURE_2D, skyboxTextureEquirect);
	glUniform1i(glGetUniformLocation(m_EquirectToCube->GetNativeShader(), "u_InputTexture"), 1);
	glDispatchCompute(m_SkyboxSize / 16, m_SkyboxSize / 16, 6);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_TEXTURE_UPDATE_BARRIER_BIT |
		GL_TEXTURE_FETCH_BARRIER_BIT);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

	glDeleteTextures(1, &skyboxTextureEquirect);
	 
	

	m_SkyboxTextureID = skyboxTextureUnfiltered; // TODO : Update this line

	const int32_t specularMapSize = m_SkyboxSize < 256 ? m_SkyboxSize : 256;
	int32_t specularMipLevels = 1;
	for (int32_t mipSize = specularMapSize; mipSize > 1; mipSize >>= 1) ++specularMipLevels;

	glGenTextures(1, &m_SpecularMapTextureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_SpecularMapTextureID);
	glTexStorage2D(GL_TEXTURE_CUBE_MAP, specularMipLevels, GL_RGBA32F, specularMapSize, specularMapSize);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	m_SpecularMap->Bind();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTextureUnfiltered);
	glUniform1i(glGetUniformLocation(m_SpecularMap->GetNativeShader(), "inputTexture"), 0);
	const GLint roughnessLocation = glGetUniformLocation(m_SpecularMap->GetNativeShader(), "roughness");
	for (int32_t mip = 0; mip < specularMipLevels; ++mip)
	{
		const int32_t mipSize = specularMapSize >> mip;
		glBindImageTexture(1, m_SpecularMapTextureID, mip, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		const float roughness = specularMipLevels > 1 ? static_cast<float>(mip) / static_cast<float>(specularMipLevels - 1) : 0.0f;
		glUniform1f(roughnessLocation, roughness);
		glDispatchCompute((mipSize + 15) / 16, (mipSize + 15) / 16, 6);
	}
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

	constexpr int32_t brdfLutSize = 256;
	glGenTextures(1, &m_BrdfLutTextureID);
	glBindTexture(GL_TEXTURE_2D, m_BrdfLutTextureID);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RG16F, brdfLutSize, brdfLutSize);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	m_BrdfLut->Bind();
	glBindImageTexture(0, m_BrdfLutTextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG16F);
	glDispatchCompute((brdfLutSize + 15) / 16, (brdfLutSize + 15) / 16, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

	glGenTextures(1, &m_IrradianceMapTextureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_IrradianceMapTextureID);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1); glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glTexStorage2D(GL_TEXTURE_CUBE_MAP, 6, GL_RGBA32F, m_IrradianceMapSize, m_IrradianceMapSize);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	 
	glBindImageTexture(0, m_IrradianceMapTextureID, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);
	m_IrradianceMap->Bind();  
	glActiveTexture(GL_TEXTURE1); 
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTextureUnfiltered);
	glUniform1i(glGetUniformLocation(m_IrradianceMap->GetNativeShader(), "u_InputTexture"), 1);
	glDispatchCompute((m_IrradianceMapSize + 15) / 16, (m_IrradianceMapSize + 15) / 16, 6);

	glFinish();
	
	 
	m_SkyboxTexturePath = path;
	m_IsSkyReady = true;
	return true;
}
