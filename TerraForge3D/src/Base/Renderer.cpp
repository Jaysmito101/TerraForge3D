#include "Renderer.h"
#include <Texture2D.h>

namespace Renderer {

	void RenderModel(Model* model, Camera* camera, FrameBuffer* framebuffer, Shader* shader, Texture2D* diffuse, glm::vec3 lightPosition, float* lightColor, float time)
	{
		framebuffer->Begin(); 
		shader->Bind();
		shader->SetTime(&time);
		shader->SetMPV(camera->pv);
		shader->SetUniformMat4("_Model", model->modelMatrix);
		shader->SetLightCol(lightColor);
		shader->SetLightPos(lightPosition);
		float tmp[3];
		shader->SetUniform3f("_MousePos", tmp);
		tmp[0] = 800;
		tmp[1] = 600;
		tmp[2] = 1;
		shader->SetUniform3f("_Resolution", tmp);
		diffuse->Bind(5);
		shader->SetUniformi("_Diffuse", 5);
		model->Render();
		framebuffer->End();
	}

	void RenderModels(std::vector<Model*> models, Camera* camera, FrameBuffer* framebuffer, Shader* shader, std::vector<Texture2D*> diffuse, glm::vec3 lightPosition, float* lightColor, float time)
	{
		if (models.size() != diffuse.size())
			return;
		framebuffer->Begin();
		for (int i = 0; i < models.size(); i++)
		{
			shader->Bind();
			shader->SetTime(&time);
			shader->SetMPV(camera->pv);
			shader->SetUniformMat4("_Model", models[i]->modelMatrix);
			shader->SetLightCol(lightColor);
			shader->SetLightPos(lightPosition);
			float tmp[3];
			shader->SetUniform3f("_MousePos", tmp);
			tmp[0] = 800;
			tmp[1] = 600;
			tmp[2] = 1;
			shader->SetUniform3f("_Resolution", tmp);
			diffuse[i]->Bind(5);
			shader->SetUniformi("_Diffuse", 5);
			models[i]->Render();
		}
		framebuffer->End();
	}

}