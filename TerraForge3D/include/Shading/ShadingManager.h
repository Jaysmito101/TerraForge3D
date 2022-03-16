#pragma once

struct ApplicationState;

#include <string>
#include <vector>

#include "Shading/GLSLHandler.h"
#include "Shading/SharedMemoryManager.h"
#include "Shading/ShaderNodeEditor.h"
#include "Shading/ShaderTextureManager.h"

struct DefaultCustomNode
{
	std::string name = "";
	std::string content = "";
};

class ShadingManager
{
public:
	ShadingManager(ApplicationState *appState);
	~ShadingManager();

	void UpdateShaders();

	void ShowSettings(bool *pOpen);

private:
	void PrepVertShader();
	void PrepGeomShader();
	void PrepFragShader();

	void ReCompileShaders();

	void LoadDefaultCustomNodes();

public:
	ApplicationState *appState = nullptr;
	GLSLHandler *vsh = nullptr;
	GLSLHandler *gsh = nullptr;
	GLSLHandler *fsh = nullptr;
	SharedMemoryManager *sharedMemoryManager = nullptr;
	NodeEditor *shaderNodeEditor = nullptr;
	ShaderTextureManager *shaderTextureManager = nullptr;

	std::string vertexSource = "";
	std::string geometrySource = "";
	std::string fragmentSource = "";

	std::vector<std::string> logs;
	std::vector<DefaultCustomNode> defaultCustomNodes;
};