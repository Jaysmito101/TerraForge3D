#include "Shading/ShaderTextureManager.h"
#include "Shading/ShaderNodes/ShaderTextureNode.h"

#include "glad/glad.h"


ShaderTextureManager::ShaderTextureManager()
{
    textureArray = 0;
    glGenTextures(1, &textureArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

ShaderTextureManager::~ShaderTextureManager()
{

}

void ShaderTextureManager::Register(ShaderTextureNode* node)
{
    textureNodes.push_back(node);
    node->zCoord = textureNodes.size() - 1;
}

void ShaderTextureManager::Unregister(ShaderTextureNode* node)
{
    for(int i = 0; i < textureNodes.size(); i++)
    {
        if(textureNodes[i]->id == node->id)
        {
            textureNodes.erase(textureNodes.begin() + i);
            break;
        }
    }
    for(int i = 0; i < textureNodes.size(); i++)
    {
        textureNodes[i]->zCoord = i;
    }
}

void ShaderTextureManager::UpdateShaders()
{
    glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGB8, 512, 512, textureNodes.size());

    for(int i = 0; i < textureNodes.size(); i++)
    {
        textureNodes[i]->texture->Resize(512, 512);
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, 512, 512, 1, GL_RGB, GL_UNSIGNED_BYTE, textureNodes[i]->texture->GetData());
    }

    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

void ShaderTextureManager::Bind(uint32_t slot)
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray);
}
