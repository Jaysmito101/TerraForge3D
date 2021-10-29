#include "CubeMap.h"

#include <cstdint>
#include <vector>
#include <string>
#include <iostream>

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


unsigned int loadCubemap(std::vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 3);
        if (data)
        {
            std::cout << "Loaded " << faces[i] << "\n";
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    skyboxShader = new Shader(vertShader, fragShader);
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
    textureID = loadCubemap(faces);

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
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    skyboxShader->Bind();
    view = glm::mat4(glm::mat3(view));
    skyboxShader->SetUniformMat4("_P", proj);
    skyboxShader->SetUniformMat4("_V", view);
    glUniform1i(glGetUniformLocation(skyboxShader->m_Shader, "skybox"), 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}