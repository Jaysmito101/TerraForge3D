#include "TextureSettings.h"
#include <Utils.h>
#include <imgui/imgui.h>
#include <Texture2D.h>
#include <ShaderStorageBuffer.h>
#include <ComputeShader.h>
#include <TextureStore.h>
#include <json.hpp>

#include <ProjectData.h>

bool* reqrfrsh;
float* textureScale;
int co = 0;
int cTexID = 0;
uint32_t diffuseUBOLocCache = -1;
uint32_t diffuseUBO;
Texture2D* diffuse;

struct TextureLayer {
	Texture2D* texture;
	std::string name = "Texture Layer";
	float heightMax = -10;
	float heightMin = -10;
	float textureScale = 1;

	float randomnessScale = 1;
	float randomnessStrength = 0;
	float steepness = 1;

	nlohmann::json Save() {
		nlohmann::json data;
		data["name"] = name;
		data["hMx"] = heightMax;
		data["hMn"] = heightMin;
		data["texSc"] = textureScale;
		data["rSc"] = randomnessScale;
		data["rSt"] = randomnessStrength;
		data["step"] = steepness;
		std::string filePath = texture->GetPath();
		std::string hash = MD5File(filePath).ToString();
		if (GetProjectAsset(hash).size() == 0) {
			if (!PathExist(GetProjectResourcePath() + "\\textures\\"))
				MkDir(GetProjectResourcePath() + "\\textures\\");
			Log("Saving Texture To : " + GetProjectResourcePath() + "textures\\" + hash);
			CopyFileData(filePath, GetProjectResourcePath() + "\\textures\\" + hash);
			RegisterProjectAsset(hash, "textures\\" + hash);
		}
		else{
			Log("Texture Already Cached!");
		}
		data["texture"] = hash;
		return data;
	}

	void Load(nlohmann::json data) {
		name = data["name"];
		heightMax = data["hMx"];
		heightMin = data["hMn"];
		textureScale = data["texSc"];

		// Temporary
		try {
		 	randomnessScale = data["rSc"];
			randomnessStrength = data["rSt"];
			steepness = data["step"];
		}
		catch (...) {}

		if (texture)
			delete texture;
		Log("Laoding : " + GetProjectResourcePath() + "\\" + GetProjectAsset(data["texture"]));
		texture = new Texture2D(GetProjectResourcePath() + "\\" + GetProjectAsset(data["texture"]), false);
	}

	void Render(int id) {
		if (ImGui::ImageButton((ImTextureID)texture->GetRendererID(), ImVec2(30, 30))) {
			cTexID = id;
			ImGui::OpenPopup("Open Image");
		}
		ImGui::SameLine();
		ImGui::InputText(("##TLayerName" + std::to_string(id)).c_str(), name.data(), 256);
		ImGui::DragFloat(("Min##TLayerHeightMin" + std::to_string(id)).c_str(), &heightMin, 0.01f);
		ImGui::DragFloat(("Max##TLayerHeightMax" + std::to_string(id)).c_str(), &heightMax, 0.01f);
		ImGui::DragFloat(("Steepness##TLayerTScale" + std::to_string(id)).c_str(), &steepness, 0.01f);
		ImGui::DragFloat(("Texture Scale##TLayerTScale" + std::to_string(id)).c_str(), &textureScale, 0.001f);
		ImGui::DragFloat(("Randomness Scale##TLayerTScale" + std::to_string(id)).c_str(), &randomnessScale, 0.01f);
		ImGui::DragFloat(("Randomness Strength##TLayerTScale" + std::to_string(id)).c_str(), &randomnessStrength, 0.01f);
	}
};


std::vector<TextureLayer> textureLayers;

std::vector<Texture2D*>* textureThumbs;
nlohmann::json* texture_database;


static void AddNewLayer() {
	TextureLayer layer;
	layer.name.reserve(256);
	layer.texture = new Texture2D(GetExecutableDir() + "\\Data\\textures\\white.png");
	textureLayers.push_back(layer);
}

void TextureSettingsTick() {
	if (IsTextureThumnsLoaded()) {
		texture_database = GetTextureDatabase();
		textureThumbs = GetTextures();
	}
}

void SetupTextureSettings(bool* reqRefresh, float* textScale)
{
	for(int i=0;i<NUM_TEXTURE_LAYERS;i++)
		AddNewLayer();
	
	glGenBuffers(1, &diffuseUBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, diffuseUBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	reqrfrsh = reqRefresh;
	textureScale = textScale;
	if (diffuse)
		delete diffuse;
	diffuse = new Texture2D(GetExecutableDir() + "\\Data\\textures\\white.png", false);
}

static void LoadUpTexture(std::string path) {
	if (path.size() <= 3)
		return;
	textureLayers[cTexID].texture = new Texture2D(path, false);
	cTexID++;
	ImGui::CloseCurrentPopup();
}

void ShowOpenTextureModal() {
	if (ImGui::BeginPopupModal("Open Image"))
	{
		if (ImGui::Button("Close"))
			ImGui::CloseCurrentPopup();

		if (ImGui::Button("Open From File")) {
			std::string textureFilePath = ShowOpenFileDialog((wchar_t*)L".png\0");
			if (textureFilePath.size() > 1) {
				LoadUpTexture(textureFilePath);
			}
			ImGui::CloseCurrentPopup();
		}

		ImGui::NewLine();


		if (!IsTextureThumnsLoaded()) {
			if (ImGui::Button("Load Texture Data")) {
				LoadTextureThumbs();
				texture_database = GetTextureDatabase();
				textureThumbs = GetTextures();
			}
		}
		else {
			int tp = 0;
			int cop = 0;
			for (auto it = (*texture_database).begin(); it != (*texture_database).end();) {
				if (it.value()["downloaded"]) {
					it.value()["key"] = it.key();
					ImGui::BeginChild(it.key().c_str(), ImVec2(200, 200));
					if (ImGui::ImageButton((ImTextureID)(*textureThumbs)[tp]->GetRendererID(), ImVec2(150, 150))) {
						LoadUpTexture(it.value()["downloaddata"]["dpath"]);
					}
					ImGui::Text(std::string(it.value()["name"]).c_str());
					ImGui::EndChild();
					cop++;
				}
				it++;
				tp++; 
				if (it == (*texture_database).end())
					break;
				ImGui::SameLine();

				if (it.value()["downloaded"]) {
					it.value()["key"] = it.key();
					ImGui::BeginChild(it.key().c_str(), ImVec2(200, 200));
					if (ImGui::ImageButton((ImTextureID)(*textureThumbs)[tp]->GetRendererID(), ImVec2(150, 150))) {
						LoadUpTexture(it.value()["downloaddata"]["dpath"]);
					}
					ImGui::Text(std::string(it.value()["name"]).c_str());
					ImGui::EndChild();
					cop++;
				}
				it++;
				tp++;
				if (it == (*texture_database).end())
					break;
				ImGui::SameLine();

				if (it.value()["downloaded"]) {
					it.value()["key"] = it.key();
					ImGui::BeginChild(it.key().c_str(), ImVec2(200, 200));
					if (ImGui::ImageButton((ImTextureID)(*textureThumbs)[tp]->GetRendererID(), ImVec2(150, 150))) {
						LoadUpTexture(it.value()["downloaddata"]["dpath"]);
					}
					ImGui::Text(std::string(it.value()["name"]).c_str());
					ImGui::EndChild();
					cop++;
				}
				it++;
				tp++;
				if (cop == 3) {
					ImGui::NewLine();
					cop = 0;
				}
			}
		}


		ImGui::EndPopup();
	}
}

void ShowTextureSettings(bool* pOpen)
{
	ImGui::Begin("Texture Settings", pOpen);
	ShowOpenTextureModal();
	uint32_t id = diffuse ? diffuse->GetRendererID() : 0;
	//ImGui::Image((ImTextureID)(id), ImVec2(200, 200));

	int i = 0;
	for (auto& it = textureLayers.begin(); it != textureLayers.end();it++) {
		ImGui::Separator();
		it->Render(i);
		/*
		if (ImGui::Button("Delete")) {
			textureLayers.erase(it);
			break;
		}
		*/
		ImGui::Separator();
		i++;
	}

	/*
	if (textureLayers.size() <= 8) {
		if (ImGui::Button("Add Layer")) {
			AddNewLayer();
		}
	}
	*/
	 
	ImGui::DragFloat("Texture Scale", textureScale, 0.1f);


	ImGui::End();
} 

uint32_t UpdateDiffuseTexturesUBO(uint32_t shaderID, std::string diffuseUBOName) {
	for (int i = 0; i < NUM_TEXTURE_LAYERS; i++) {
		textureLayers[i].texture->Bind(i);
		GLuint uUniform = glGetUniformLocation(shaderID, (diffuseUBOName + "[" + std::to_string(i) + "]").c_str());
		glUniform1i(uUniform, i);
		uUniform = glGetUniformLocation(shaderID, (diffuseUBOName + "Heights[" + std::to_string(i) + "]").c_str());
		glUniform3f(uUniform, textureLayers[i].heightMin, textureLayers[i].heightMax, textureLayers[i].textureScale);
		uUniform = glGetUniformLocation(shaderID, (diffuseUBOName + "Data[" + std::to_string(i) + "]").c_str());
		glUniform3f(uUniform, textureLayers[i].randomnessScale, textureLayers[i].randomnessStrength, textureLayers[i].steepness);		
	}
	return diffuseUBO;
}

Texture2D* GetCurrentDiffuseTexture()
{ 
	return diffuse;
}

nlohmann::json SaveTextureLayerData() {
	nlohmann::json data;
	for (TextureLayer& tl : textureLayers)
		data.push_back(tl.Save());
	return data;
}

void LoadTextureLayerData(nlohmann::json data) {
	int i = 0;
	for (nlohmann::json tl : data) {
		textureLayers[i++].Load(tl);
	}
}