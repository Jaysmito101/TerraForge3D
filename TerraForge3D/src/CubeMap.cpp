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

#include "Model.h"
#include "ModelImporter.h"

#include <Utils.h>

static uint32_t textureID;
static uint32_t vao;
bool tmpb = false;
static std::string vertShader = ReadShaderSourceFile(GetExecutableDir() + "\\Data\\shaders\\skybox\\vert.glsl", &tmpb);
static std::string fragShader = ReadShaderSourceFile(GetExecutableDir() + "\\Data\\shaders\\skybox\\frag.glsl", &tmpb);
static std::string fragProShader = ReadShaderSourceFile(GetExecutableDir() + "\\Data\\shaders\\skybox\\procedural_clouds.glsl", &tmpb);


static Shader* skyboxShader, *skyproShader;
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
static Model* skySphere;

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
    skyproShader = new Shader(vertShader, fragProShader);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    skySphere = LoadModel(GetExecutableDir() + "\\Data\\models\\icosphere.obj");
    skySphere->SetupMeshOnGPU();
    skySphere->UploadToGPU();
}


void RenderSkybox(glm::mat4 view, glm::mat4 proj, bool useBox, bool useProcedural, float cirrus, float cumulus, float time, float* fsun, float upf){
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glBindVertexArray(vao);
    
    Shader* shd = skyboxShader;
    if(useProcedural)
        shd = skyproShader;
    shd->Bind();
    cubemap.Bind(0);
    view = glm::mat4(glm::mat3(view));
    shd->SetUniformMat4("_P", proj);
    shd->SetUniformMat4("_V", view);
    shd->SetUniformf("time", time);
    shd->SetUniformf("cirrus", cirrus);
    shd->SetUniformf("upf", upf);
    shd->SetUniformf("cumulus", cumulus);
    shd->SetUniform3f("fsun", fsun);
    glUniform1i(glGetUniformLocation(shd->m_Shader, "skybox"), 0);
    if(useBox)
        glDrawArrays(GL_TRIANGLES, 0, 36);
    else{
        skySphere->Update();
        skySphere->Render();
    }
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

TextureCubemap* GetSkyboxCubemapTexture()
{
    return &cubemap;
}
