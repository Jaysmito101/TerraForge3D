#pragma once

#include <Model.h>
#include <FrameBuffer.h>
#include <Camera.h>
#include <Texture2D.h>
#include <Shader.h>

#include <vector>

namespace Renderer {

	void RenderModel(Model* model, Camera* camera, FrameBuffer* framebuffer, Shader* shader, Texture2D* diffuse, glm::vec3 lightPosition, float* lightColor, float time);

	void RenderModels(std::vector<Model*> models, Camera* camera, FrameBuffer* framebuffer, Shader* shader, std::vector<Texture2D*> diffuse, glm::vec3 lightPosition, float* lightColor, float time);


}
