#include <string>
#include <vector>
#include <iostream>
#include <json.hpp>

class Texture2D;

void SetupTextureStore(std::string executablePath, bool *reqrfersh);
void ShowTextureStore(bool* pOpen);
void UpdateTextureStore();
void LoadTextureThumbs();
static void DeleteTexture(std::string id, int k);

std::string GetTextureStoreSelectedTexture();

bool IsTextureThumnsLoaded();
std::vector<Texture2D*>* GetTextures();
nlohmann::json* GetTextureDatabase();