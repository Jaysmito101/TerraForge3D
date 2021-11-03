#include "TextureBake.h"
#include <ComputeShader.h>
#include <ShaderStorageBuffer.h>
#include <TextureSettings.h>
#include <Utils.h>
#include <regex> 

Texture2D* BakeTexture(Model* model)
{
    int res = model->mesh->res;
    unsigned char* data = new unsigned char[res*res*3];
    Texture2D* tex = new Texture2D(res, res);
    for (int x = 0; x < res; x++) { 
        for (int y = 0; y < res; y++) {
            float elev = (model->mesh->GetElevation(x, y) + model->mesh->minHeight)/(model->mesh->maxHeight);
            data[x * res + y * 3 + 0] = (unsigned char)(elev * 255);
            data[x * res + y * 3 + 1] = (unsigned char)(elev * 255);
            data[x * res + y * 3 + 2] =(unsigned char)( elev * 255);
        } 
    }
    tex->SetData(data, res * res * 3);
    delete[] data;
    return tex;
}
