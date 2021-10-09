#define _CRT_SECURE_NO_WARNINGS

#include <TextureStore.h>
#include <imgui.h>
#include <Utils.h>

void SetupTextureStore(std::string executablePath){
	Log("Setting up Texture Store ...");
	std::string tmp = FetchURL("https://api.polyhaven.com", "/assets?t=textures");
	SaveToFile(executablePath + "\\Data\\cache\\texture_database.terr3d", tmp);
}

void UpdateTextureStore(){

}

void ShowTextureStore(bool* pOpen){
	ImGui::Begin("Texture Store", pOpen);
	ImGui::Text("Work In Progress...");
	ImGui::End();
}