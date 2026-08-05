#pragma once

#include "Base/Camera.h"
#include "Base/FrameBuffer.h"
#include "Base/Model.h"
#include "Base/Shader.h"
#include "Base/Texture2D.h"

#include <vector>

namespace Renderer
{

    void RenderModel(Model *model, Camera *camera, FrameBuffer *framebuffer, Shader *shader, Texture2D *diffuse, glm::vec3 lightPosition, float *lightColor, float time);

    void RenderModels(std::vector<Model *> models, Camera *camera, FrameBuffer *framebuffer, Shader *shader, std::vector<Texture2D *> diffuse, glm::vec3 lightPosition, float *lightColor, float time);

} // namespace Renderer
