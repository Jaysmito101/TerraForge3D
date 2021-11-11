#pragma once

#include <cstdint>
#include <string>

#include <json.hpp>

#define NUM_TEXTURE_LAYERS 8

class Model;

class Texture2D;

void SetupTextureSettings(bool* reqRefresh, float* textureScale, Model* model);

void ShowTextureSettings(bool* pOpen);

void TextureSettingsTick();

Texture2D* GetCurrentDiffuseTexture();

uint32_t UpdateDiffuseTexturesUBO(uint32_t shaderID, std::string diffuseUBOName);

nlohmann::json SaveTextureLayerData();

void LoadTextureLayerData(nlohmann::json data);