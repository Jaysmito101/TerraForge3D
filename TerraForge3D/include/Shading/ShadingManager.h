#pragma once

struct ApplicationState;

#include "Shading/GLSLHandler.h"

class ShadingManager
{
public:
	ShadingManager(ApplicationState* appState);
	~ShadingManager();


	void ShowSettings(bool* pOpen);

private:
	void PrepVertShader();
	void ReCompileShaders();

public:
	ApplicationState* appState = nullptr;
	GLSLHandler* vsh = nullptr;
	GLSLHandler* gsh = nullptr;
	GLSLHandler* fsh = nullptr;

	std::string vetexSource = "";
};