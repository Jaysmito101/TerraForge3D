#include "CubeMap.h"

#include <cstdint>
#include <vector>
#include <string>
#include <iostream>
#include <TextureCubemap.h>
#include <stb/stb_image.h>
#include <glad/glad.h>
#include <Shader.h>
#include <glm/glm.hpp>

#include <Utils.h>

static uint32_t textureID;
static uint32_t vao;
bool tmpb = false;
static std::string vertShader = ReadShaderSourceFile(GetExecutableDir() + "\\Data\\shaders\\skybox\\vert.glsl", &tmpb);
static std::string fragShader = ReadShaderSourceFile(GetExecutableDir() + "\\Data\\shaders\\skybox\\frag.glsl", &tmpb);


static Shader* skyboxShader;
float skyboxVertices[] = {
    // positions          
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
};

static TextureCubemap cubemap;


unsigned int loadCubemap(std::vector<std::string> faces)
{
    
    return textureID;
}

void SetupCubemap()
{

    std::vector<std::string> faces =
    {
            GetExecutableDir() + "\\Data\\skybox\\px.jpg",
            GetExecutableDir() + "\\Data\\skybox\\nx.jpg",
            GetExecutableDir() + "\\Data\\skybox\\py.jpg",
            GetExecutableDir() + "\\Data\\skybox\\ny.jpg",
            GetExecutableDir() + "\\Data\\skybox\\pz.jpg",
            GetExecutableDir() + "\\Data\\skybox\\nz.jpg"
    };
    
    cubemap.SetUpOnGPU();
    cubemap.LoadFaces(faces);
    cubemap.UploadDataToGPU();

    skyboxShader = new Shader(vertShader, fragShader);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}


void RenderSkybox(glm::mat4 view, glm::mat4 proj) {
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glBindVertexArray(vao);
    skyboxShader->Bind();
    cubemap.Bind(0);
    view = glm::mat4(glm::mat3(view));
    skyboxShader->SetUniformMat4("_P", proj);
    skyboxShader->SetUniformMat4("_V", view);
    glUniform1i(glGetUniformLocation(skyboxShader->m_Shader, "skybox"), 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

TextureCubemap* GetSkyboxCubemapTexture()
{
    return &cubemap;
}
