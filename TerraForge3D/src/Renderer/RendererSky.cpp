#include "Renderer/RendererSky.h"
#include "Base/Base.h"
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
}

RendererSky::~RendererSky()
{
	if (m_SkyboxTextureID > -1) glDeleteTextures(1, &m_SkyboxTextureID);
	if (m_IrradianceMapTextureID > -1) glDeleteTextures(1, &m_IrradianceMapTextureID);
	delete m_EquirectToCube;
	delete m_SpecularMap;
	delete m_IrradianceMap;
	delete m_SkyboxModel;
	delete m_SkyboxShader;
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
	glm::mat4 mpv = viewport->m_Camera.pers * glm::mat4(glm::mat3(viewport->m_Camera.view));
	glUniformMatrix4fv(glGetUniformLocation(m_SkyboxShader->GetNativeShader(), "u_MPV"), 1, GL_FALSE, glm::value_ptr(mpv));
	m_SkyboxModel->Render();
	glEnable(GL_DEPTH_TEST);
}

void RendererSky::ReloadShaders()
{
	if (m_EquirectToCube) delete m_EquirectToCube;
	if (m_SpecularMap) delete m_SpecularMap;
	if (m_IrradianceMap) delete m_IrradianceMap;
	bool tmp = false;
	m_EquirectToCube = new ComputeShader(ReadShaderSourceFile(m_AppState->constants.shadersDir + PATH_SEPARATOR "equirect_to_cube" PATH_SEPARATOR "compute.glsl", &tmp));
	m_SpecularMap = new ComputeShader(ReadShaderSourceFile(m_AppState->constants.shadersDir + PATH_SEPARATOR "sky_specular_map" PATH_SEPARATOR "compute.glsl", &tmp));
	m_IrradianceMap = new ComputeShader(ReadShaderSourceFile(m_AppState->constants.shadersDir + PATH_SEPARATOR "sky_irradiance_map" PATH_SEPARATOR "compute.glsl", &tmp));
	m_SkyboxShader = new Shader(
		ReadShaderSourceFile(m_AppState->constants.shadersDir + PATH_SEPARATOR "skybox" PATH_SEPARATOR "vert.glsl", &tmp),
		ReadShaderSourceFile(m_AppState->constants.shadersDir + PATH_SEPARATOR "skybox" PATH_SEPARATOR "frag.glsl", &tmp));
}

// From : https://github.com/Nadrin/PBR/blob/master/src/opengl.cpp
bool RendererSky::LoadSkyboxTexture(const std::string& path)
{
	if (path.size() < 3) return false;
	if (m_SkyboxTextureID > -1) glDeleteTextures(1, &m_SkyboxTextureID);
	if (m_IrradianceMapTextureID > -1) glDeleteTextures(1, &m_IrradianceMapTextureID); 

	int32_t skyboxTextureEquirectWidth = 0, skyboxTextureEquirectHeight = 0;
	unsigned char* skyboxTextureEquirectData = stbi_load(path.c_str(), &skyboxTextureEquirectWidth, &skyboxTextureEquirectHeight, nullptr, 3);
	if (!skyboxTextureEquirectData)   
	{
		Log("Failed to load skybox texture : %s" + path);
		return false;
	}
	uint32_t skyboxTextureEquirect = -1;
	glGenTextures(1, &skyboxTextureEquirect);
	glBindTexture(GL_TEXTURE_2D, skyboxTextureEquirect);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1); glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, skyboxTextureEquirectWidth, skyboxTextureEquirectHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, skyboxTextureEquirectData);
	stbi_image_free(skyboxTextureEquirectData);

	uint32_t skyboxTextureUnfiltered = -1;
	glGenTextures(1, &skyboxTextureUnfiltered);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTextureUnfiltered);
	glTexStorage2D(GL_TEXTURE_CUBE_MAP, 6, GL_RGBA32F, m_SkyboxSize, m_SkyboxSize);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	
	glBindImageTexture(0, skyboxTextureUnfiltered, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);
	m_EquirectToCube->Bind();
	glActiveTexture(GL_TEXTURE0 + 1); 
	glBindTexture(GL_TEXTURE_2D, skyboxTextureEquirect);
	glUniform1i(glGetUniformLocation(m_EquirectToCube->GetNativeShader(), "u_InputTexture"), 1);
	glDispatchCompute(m_SkyboxSize / 16, m_SkyboxSize / 16, 6);

	glDeleteTextures(1, &skyboxTextureEquirect);
	 
	

	m_SkyboxTextureID = skyboxTextureUnfiltered; // TODO : Update this line

	
	glGenTextures(1, &m_IrradianceMapTextureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_IrradianceMapTextureID);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1); glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glTexStorage2D(GL_TEXTURE_CUBE_MAP, 6, GL_RGBA32F, m_IrradianceMapSize, m_IrradianceMapSize);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	 
	glBindImageTexture(0, m_IrradianceMapTextureID, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);
	m_IrradianceMap->Bind();  
	glActiveTexture(GL_TEXTURE1); 
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTextureUnfiltered);
	glUniform1i(glGetUniformLocation(m_IrradianceMap->GetNativeShader(), "u_InputTexture"), 1);
	glDispatchCompute(m_IrradianceMapSize / 16 + 1, m_IrradianceMapSize / 16 + 1, 6);

	glFinish();
	
	 
	m_SkyboxTexturePath = path;
	m_IsSkyReady = true;
	return true;
}
