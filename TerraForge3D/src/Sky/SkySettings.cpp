#include "Sky/SkySettings.h"
#include "Sky/CubeMap.h"
#include "Utils/Utils.h"
#include "Data/ApplicationState.h"

#include <imgui/imgui.h>
#include <hdritocubemap/HdriToCubemap.hpp>

SkyManager::~SkyManager()
{
	if(cubemapManager)
	{
		delete cubemapManager;
	}
}

SkyManager::SkyManager(ApplicationState *as)
{
	appState = as;
	cubemapManager = new CubeMapManager(appState);
	cubemap = cubemapManager->GetSkyboxCubemapTexture();
}

void SkyManager::ChangeCubemapTile(int face)
{
	std::string path = ShowOpenFileDialog(".png");

	if (path.size() > 2)
	{
		cubemap->LoadFace(path, face);
		cubemap->UploadDataToGPU();
	}
}



void SkyManager::LoadHDRI(std::string path)
{
	// load hdr or ldr image
	int cubemapResolution = 1024;
	bool linearFilter = true;
	HdriToCubemap<unsigned char> hdriToCube_ldr(path, cubemapResolution, linearFilter);
	cubemap->facesData[TEXTURE_CUBEMAP_PX] = hdriToCube_ldr.getLeft();
	cubemap->facesSizes[TEXTURE_CUBEMAP_PX] = IVec2(hdriToCube_ldr.getCubemapResolution());
	cubemap->facesData[TEXTURE_CUBEMAP_NX] = hdriToCube_ldr.getRight();
	cubemap->facesSizes[TEXTURE_CUBEMAP_NX] = IVec2(hdriToCube_ldr.getCubemapResolution());
	cubemap->facesData[TEXTURE_CUBEMAP_PY] = hdriToCube_ldr.getDown();
	cubemap->facesSizes[TEXTURE_CUBEMAP_PY] = IVec2(hdriToCube_ldr.getCubemapResolution());
	cubemap->facesData[TEXTURE_CUBEMAP_NY] = hdriToCube_ldr.getUp();
	cubemap->facesSizes[TEXTURE_CUBEMAP_NY] = IVec2(hdriToCube_ldr.getCubemapResolution());
	cubemap->facesData[TEXTURE_CUBEMAP_PZ] = hdriToCube_ldr.getBack();
	cubemap->facesSizes[TEXTURE_CUBEMAP_PZ] = IVec2(hdriToCube_ldr.getCubemapResolution());
	cubemap->facesData[TEXTURE_CUBEMAP_NZ] = hdriToCube_ldr.getFront();
	cubemap->facesSizes[TEXTURE_CUBEMAP_NZ] = IVec2(hdriToCube_ldr.getCubemapResolution());
	cubemap->UploadDataToGPU();
}

void SkyManager::ShowSettings(bool *pOpen)
{
	ImGui::Begin("Sky Settings", pOpen);

	if(!useProcedural )
	{
		if (ImGui::Button("Load HDRI"))
		{
			std::string path = ShowOpenFileDialog(".png");

			if (path.size() > 2)
			{
				LoadHDRI(path);
			}
		}

		ImGui::Separator();
		ImGui::Text("Cubemap Faces");
		ImGui::Text("PX");
		ImGui::SameLine();

		if (ImGui::ImageButton((ImTextureID)cubemap->textures[TEXTURE_CUBEMAP_PX]->GetRendererID(), ImVec2(50, 50)))
		{
			ChangeCubemapTile(TEXTURE_CUBEMAP_PX);
		}

		ImGui::SameLine();
		ImGui::Text("PY");
		ImGui::SameLine();

		if (ImGui::ImageButton((ImTextureID)cubemap->textures[TEXTURE_CUBEMAP_PY]->GetRendererID(), ImVec2(50, 50)))
		{
			ChangeCubemapTile(TEXTURE_CUBEMAP_PY);
		}

		ImGui::SameLine();
		ImGui::Text("PZ");
		ImGui::SameLine();

		if (ImGui::ImageButton((ImTextureID)cubemap->textures[TEXTURE_CUBEMAP_PZ]->GetRendererID(), ImVec2(50, 50)))
		{
			ChangeCubemapTile(TEXTURE_CUBEMAP_PZ);
		}

		ImGui::Text("NX");
		ImGui::SameLine();

		if (ImGui::ImageButton((ImTextureID)cubemap->textures[TEXTURE_CUBEMAP_NX]->GetRendererID(), ImVec2(50, 50)))
		{
			ChangeCubemapTile(TEXTURE_CUBEMAP_NX);
		}

		ImGui::SameLine();
		ImGui::Text("NY");
		ImGui::SameLine();

		if (ImGui::ImageButton((ImTextureID)cubemap->textures[TEXTURE_CUBEMAP_NY]->GetRendererID(), ImVec2(50, 50)))
		{
			ChangeCubemapTile(TEXTURE_CUBEMAP_NY);
		}

		ImGui::SameLine();
		ImGui::Text("NZ");
		ImGui::SameLine();

		if (ImGui::ImageButton((ImTextureID)cubemap->textures[TEXTURE_CUBEMAP_NZ]->GetRendererID(), ImVec2(50, 50)))
		{
			ChangeCubemapTile(TEXTURE_CUBEMAP_NZ);
		}

		ImGui::Separator();
	}

	else
	{
		ImGui::DragFloat("Clouds Time", &cltime, 0.01f);
		ImGui::DragFloat("Clouds UPF", &upf, 0.01f);
		ImGui::DragFloat("Cirrus Clouds", &cirrus, 0.005f, -1, 1);
		ImGui::DragFloat("Cumulus Clouds", &cumulus, 0.005f, -1, 1);
		ImGui::DragFloat3("Sun Position", fsun, 0.01f);
	}

	if(!useProcedural)
	{
		useProcedural = ImGui::Button("Use Procedural Sky");
	}

	else
	{
		useProcedural = !ImGui::Button("Use Cubemap Sky");
	}

	if(!useBox)
	{
		useBox = ImGui::Button("Use Sky Box");
	}

	else
	{
		useBox = !ImGui::Button("Use Sky Sphere");
	}

	ImGui::End();
}

void SkyManager::RenderSky(glm::mat4 view, glm::mat4 pers)
{
	cubemapManager->RenderSkybox(view, pers, useBox, useProcedural, cirrus, cumulus, cltime, fsun, upf);
}

