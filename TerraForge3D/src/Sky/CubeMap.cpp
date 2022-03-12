#include "Sky/CubeMap.h"

#include <cstdint>
#include <vector>
#include <string>
#include <iostream>
#include <TextureCubemap.h>
#include <stb/stb_image.h>
#include <glad/glad.h>
#include <Shader.h>
#include <glm/glm.hpp>

#include "Platform.h"
#include "Base/Model.h"
#include "Base/ModelImporter.h"
#include "Utils/Utils.h"
#include "Data/ApplicationState.h"

static bool tmpb = false;
static std::string vertShader = ReadShaderSourceFile(GetExecutableDir() + PATH_SEPERATOR "Data" PATH_SEPERATOR "shaders" PATH_SEPERATOR "skybox" PATH_SEPERATOR "vert.glsl", &tmpb);
static std::string fragShader = ReadShaderSourceFile(GetExecutableDir() + PATH_SEPERATOR "Data" PATH_SEPERATOR "shaders" PATH_SEPERATOR "skybox" PATH_SEPERATOR "frag.glsl", &tmpb);
static std::string fragProShader = ReadShaderSourceFile(GetExecutableDir() + PATH_SEPERATOR "Data" PATH_SEPERATOR "shaders" PATH_SEPERATOR "skybox" PATH_SEPERATOR "procedural_clouds.glsl", &tmpb);


static float skyboxVertices[] =
{
	// positions
	-1.0f,  1.0f, -1.0f,
	    -1.0f, -1.0f, -1.0f,
	    1.0f, -1.0f, -1.0f,
	    1.0f, -1.0f, -1.0f,
	    1.0f,  1.0f, -1.0f,
	    -1.0f,  1.0f, -1.0f,

	    -1.0f, -1.0f,  1.0f,
	    -1.0f, -1.0f, -1.0f,
	    -1.0f,  1.0f, -1.0f,
	    -1.0f,  1.0f, -1.0f,
	    -1.0f,  1.0f,  1.0f,
	    -1.0f, -1.0f,  1.0f,

	    1.0f, -1.0f, -1.0f,
	    1.0f, -1.0f,  1.0f,
	    1.0f,  1.0f,  1.0f,
	    1.0f,  1.0f,  1.0f,
	    1.0f,  1.0f, -1.0f,
	    1.0f, -1.0f, -1.0f,

	    -1.0f, -1.0f,  1.0f,
	    -1.0f,  1.0f,  1.0f,
	    1.0f,  1.0f,  1.0f,
	    1.0f,  1.0f,  1.0f,
	    1.0f, -1.0f,  1.0f,
	    -1.0f, -1.0f,  1.0f,

	    -1.0f,  1.0f, -1.0f,
	    1.0f,  1.0f, -1.0f,
	    1.0f,  1.0f,  1.0f,
	    1.0f,  1.0f,  1.0f,
	    -1.0f,  1.0f,  1.0f,
	    -1.0f,  1.0f, -1.0f,

	    -1.0f, -1.0f, -1.0f,
	    -1.0f, -1.0f,  1.0f,
	    1.0f, -1.0f, -1.0f,
	    1.0f, -1.0f, -1.0f,
	    -1.0f, -1.0f,  1.0f,
	    1.0f, -1.0f,  1.0f
    };

CubeMapManager::~CubeMapManager()
{
	if(skyboxShader)
	{
		delete skyboxShader;
	}

	if(skyproShader)
	{
		delete skyproShader;
	}

	if(skySphere)
	{
		delete skySphere;
	}

	if(cubemap)
	{
		delete cubemap;
	}
}

CubeMapManager::CubeMapManager(ApplicationState *as)
{
	appState = as;
	static std::vector<std::string> faces =
	{
		appState->constants.skyboxDir + PATH_SEPERATOR "px.jpg",
		appState->constants.skyboxDir + PATH_SEPERATOR "nx.jpg",
		appState->constants.skyboxDir + PATH_SEPERATOR "py.jpg",
		appState->constants.skyboxDir + PATH_SEPERATOR "ny.jpg",
		appState->constants.skyboxDir + PATH_SEPERATOR "pz.jpg",
		appState->constants.skyboxDir + PATH_SEPERATOR "nz.jpg"
	};
	cubemap = new TextureCubemap();
	cubemap->SetUpOnGPU();
	cubemap->LoadFaces(faces);
	cubemap->UploadDataToGPU();
	skyboxShader = new Shader(vertShader, fragShader);
	skyproShader = new Shader(vertShader, fragProShader);
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	unsigned int VBO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
	glEnableVertexAttribArray(0);
	skySphere = LoadModel(GetExecutableDir() + PATH_SEPERATOR "Data" PATH_SEPERATOR "models" PATH_SEPERATOR "icosphere.obj");
	skySphere->SetupMeshOnGPU();
	skySphere->UploadToGPU();
}


void CubeMapManager::RenderSkybox(glm::mat4 view, glm::mat4 proj, bool useBox, bool useProcedural, float cirrus, float cumulus, float time, float *fsun, float upf)
{
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glBindVertexArray(vao);
	Shader *shd = skyboxShader;

	if(useProcedural)
	{
		shd = skyproShader;
	}

	shd->Bind();
	cubemap->Bind(0);
	view = glm::mat4(glm::mat3(view));
	shd->SetUniformMat4("_P", proj);
	shd->SetUniformMat4("_V", view);
	shd->SetUniformf("time", time);
	shd->SetUniformf("cirrus", cirrus);
	shd->SetUniformf("upf", upf);
	shd->SetUniformf("cumulus", cumulus);
	shd->SetUniform3f("fsun", fsun);
	glUniform1i(glGetUniformLocation(shd->m_Shader, "skybox"), 0);

	if(useBox)
	{
		glDrawArrays(GL_TRIANGLES, 0, 36);
	}

	else
	{
		skySphere->Update();
		skySphere->Render();
	}

	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
}

TextureCubemap *CubeMapManager::GetSkyboxCubemapTexture()
{
	return cubemap;
}
