#include "SkySettings.h"

#include "CubeMap.h"

#include <imgui.h>
#include "Utils.h"

TextureCubemap* cubemap;

void SetupSky()
{
	SetupCubemap();
	cubemap = GetSkyboxCubemapTexture();
}

static void ChangeCubemapTile(int face) {
	std::string path = ShowOpenFileDialog(L".png\0");
	if (path.size() > 2) {
		cubemap->LoadFace(path, face);
		cubemap->UploadDataToGPU();
	}
}

void ShowSkySettings(bool* pOpen)
{
	ImGui::Begin("Sky Settings", pOpen);
	
	ImGui::Text("PX");
	ImGui::SameLine();
	if (ImGui::ImageButton((ImTextureID)cubemap->textures[TEXTURE_CUBEMAP_PX]->GetRendererID(), ImVec2(50, 50))) {
		ChangeCubemapTile(TEXTURE_CUBEMAP_PX);
	}
	ImGui::SameLine();
	ImGui::Text("PY");
	ImGui::SameLine();
	if (ImGui::ImageButton((ImTextureID)cubemap->textures[TEXTURE_CUBEMAP_PY]->GetRendererID(), ImVec2(50, 50))) {
		ChangeCubemapTile(TEXTURE_CUBEMAP_PY);
	}
	ImGui::SameLine();
	ImGui::Text("PZ");
	ImGui::SameLine();
	if (ImGui::ImageButton((ImTextureID)cubemap->textures[TEXTURE_CUBEMAP_PZ]->GetRendererID(), ImVec2(50, 50))) {
		ChangeCubemapTile(TEXTURE_CUBEMAP_PZ);
	}

	ImGui::Text("NX");
	ImGui::SameLine();
	if (ImGui::ImageButton((ImTextureID)cubemap->textures[TEXTURE_CUBEMAP_NX]->GetRendererID(), ImVec2(50, 50))) {
		ChangeCubemapTile(TEXTURE_CUBEMAP_NX);
	}
	ImGui::SameLine();
	ImGui::Text("NY");
	ImGui::SameLine();
	if (ImGui::ImageButton((ImTextureID)cubemap->textures[TEXTURE_CUBEMAP_NY]->GetRendererID(), ImVec2(50, 50))) {
		ChangeCubemapTile(TEXTURE_CUBEMAP_NY);
	}
	ImGui::SameLine();
	ImGui::Text("NZ");
	ImGui::SameLine();
	if (ImGui::ImageButton((ImTextureID)cubemap->textures[TEXTURE_CUBEMAP_NZ]->GetRendererID(), ImVec2(50, 50))) {
		ChangeCubemapTile(TEXTURE_CUBEMAP_NZ);
	}
	ImGui::Separator();
	ImGui::End();

}

void RenderSky(glm::mat4 view, glm::mat4 pers)
{
	RenderSkybox(view, pers);
}

