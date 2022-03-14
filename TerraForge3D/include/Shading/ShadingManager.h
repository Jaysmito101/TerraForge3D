#pragma once

struct ApplicationState;

#include "Shading/GLSLHandler.h"

class ShadingManager
{
public:
	ShadingManager(ApplicationState *appState);
	~ShadingManager();


	void ShowSettings(bool *pOpen);

private:
	void PrepVertShader();
	void PrepGeomShader();
	void PrepFragShader();

	void ReCompileShaders();

public:
	ApplicationState *appState = nullptr;
	GLSLHandler *vsh = nullptr;
	GLSLHandler *gsh = nullptr;
	GLSLHandler *fsh = nullptr;

	std::string vertexSource = "";
	std::string geometrySource = "";
	std::string fragmentSource = "";
};