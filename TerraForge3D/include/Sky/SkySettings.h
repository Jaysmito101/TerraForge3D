#pragma once
#include <glm/glm.hpp>

void SetupSky();

void ShowSkySettings(bool *pOpen);

void RenderSky(glm::mat4 vie, glm::mat4 pers);