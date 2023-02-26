#pragma once

#include "Base/Base.h"

class ApplicationState;

enum RendererLightType
{
	RendererLightType_Directional = 0,
	RendererLightType_Point,
	RendererLightType_Count
};

struct RendererLightData
{
	char name[1024];
	glm::vec3 position = glm::vec3(0.0f); // or direction
	glm::vec3 color = glm::vec3(1.0f);
	float intensity = 1.0f;
	RendererLightType type = RendererLightType_Directional;
};

class RendererLights
{
public:
	RendererLights(ApplicationState* appState);
	~RendererLights();

	void ShowSettings();

public:
	std::vector<RendererLightData> m_RendererLights;
	bool m_UseSkyLight = false;

private:
	ApplicationState* m_AppState = nullptr;
};