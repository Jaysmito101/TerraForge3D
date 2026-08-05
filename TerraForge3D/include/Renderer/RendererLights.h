#pragma once

#include "Base/Base.h"

class ApplicationState;

struct RendererSunData
{
	char name[1024];
	glm::vec3 direction = glm::vec3(2.260f, -2.440f, -0.740f);
	glm::vec3 color = glm::vec3(1.0f);
	float intensity = 2.0f;
};

class RendererLights
{
public:
	RendererLights(ApplicationState* appState);
	~RendererLights();

	void ShowSettings();

public:
	RendererSunData m_Sun;
	bool m_UseSkyLight = true;
	float m_SkyLightIntensity = 0.6f;

private:
	ApplicationState* m_AppState = nullptr;
};
