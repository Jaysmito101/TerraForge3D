#pragma once
#include <glm/glm.hpp>

#include <TextureCubemap.h>

void SetupCubemap();

void RenderSkybox(glm::mat4 view, glm::mat4 proj, bool useBox, bool useProcedural, float cirrus, float cumulus, float time, float* fsun, float upF);

TextureCubemap* GetSkyboxCubemapTexture();