#include "Renderer.h"
#include <Texture2D.h>

namespace tf3d::base
{

    namespace Renderer
    {

        void RenderModel(Model *model, Camera *camera, FrameBuffer *framebuffer, GraphicsShader *shader, Texture2D *diffuse, glm::vec3 lightPosition, float *lightColor, float time)
        {
            framebuffer->Begin();
            shader->Bind();
            shader->SetUniform1f("_Time", time);
            shader->SetUniformMat4("_PV", camera->GetProjectionViewMatrix());
            shader->SetUniformMat4("_Model", model->modelMatrix);
            shader->SetUniform3f("_LightColor", lightColor);
            shader->SetUniform3f("_LightPosition", lightPosition);
            float tmp[3];
            shader->SetUniform3f("_MousePos", tmp);
            tmp[0] = 800;
            tmp[1] = 600;
            tmp[2] = 1;
            shader->SetUniform3f("_Resolution", tmp);
            diffuse->Bind(5);
            shader->SetUniform1i("_Diffuse", 5);
            model->Render();
            framebuffer->End();
        }

        void RenderModels(std::vector<Model *> models, Camera *camera, FrameBuffer *framebuffer, GraphicsShader *shader, std::vector<Texture2D *> diffuse, glm::vec3 lightPosition, float *lightColor, float time)
        {
            if (models.size() != diffuse.size()) {
                return;
            }

            framebuffer->Begin();

            for (int i = 0; i < models.size(); i++) {
                shader->Bind();
                shader->SetUniform1f("_Time", time);
                shader->SetUniformMat4("_PV", camera->GetProjectionViewMatrix());
                shader->SetUniformMat4("_Model", models[i]->modelMatrix);
                shader->SetUniform3f("_LightColor", lightColor);
                shader->SetUniform3f("_LightPosition", lightPosition);
                float tmp[3];
                shader->SetUniform3f("_MousePos", tmp);
                tmp[0] = 800;
                tmp[1] = 600;
                tmp[2] = 1;
                shader->SetUniform3f("_Resolution", tmp);
                diffuse[i]->Bind(5);
                shader->SetUniform1i("_Diffuse", 5);
                models[i]->Render();
            }

            framebuffer->End();
        }

    } // namespace Renderer

} // namespace tf3d::base
