#pragma once
#include <glm/glm.hpp>

#include "Base/TextureCubemap.h"

class ApplicationState;
class Shader;
class TextureCubemap;
class Model;

class CubeMapManager
{
public:
	//! CubeMapManager Constructor
	/*!
		For Internal Use Only
	*/
	CubeMapManager(ApplicationState *appState);
	
	//! CubeMapManager Destructor
	/*!
		For Internal Use Only
	*/
	~CubeMapManager();

	void RenderSkybox(glm::mat4 view, glm::mat4 proj, bool useBox, bool useProcedural, float cirrus, float cumulus, float time, float *fsun, float upF);

	TextureCubemap *GetSkyboxCubemapTexture();
public:
	ApplicationState *appState = nullptr; /*!< The ApplicationState allows you to control anything in TerraForge3D */
	Shader *skyboxShader = nullptr;
	Shader *skyproShader = nullptr;
	TextureCubemap *cubemap = nullptr;
	Model *skySphere = nullptr;
	uint32_t textureID = 0;
	uint32_t vao = 0;
};
