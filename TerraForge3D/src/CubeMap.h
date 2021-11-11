#pragma once
#include <glm/glm.hpp>

#include <TextureCubemap.h>

void SetupCubemap();

void RenderSkybox(glm::mat4 view, glm::mat4 proj);

TextureCubemap* GetSkyboxCubemapTexture();