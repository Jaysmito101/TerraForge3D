#include "FoliagePlacement.h"
#include "Utils.h"
#include "Base/ModelImporter.h"
#include <Shader.h>

#include <glm/gtc/type_ptr.hpp>

#include <imgui/imgui.h>

std::vector<FoliageItem> fi;

void SetupFoliageManager()
{
	
}

void ShowFoliageManager(bool* pOpen)
{
	ImGui::Begin("Foliage Manager", pOpen);
	int id = 0;
	for (FoliageItem& t : fi) {
		ImGui::Separator();
		
		bool tmp = ImGui::CollapsingHeader((std::string("##foliageItem") + std::to_string(id++)).c_str());
		ImGui::SameLine();
		ImGui::Text(t.name.c_str());
		if (tmp) {
			ImGui::Text(t.modelDir.c_str());

			ImGui::Checkbox((std::string("Enabled##") + std::to_string(id++)).c_str(), &t.active);

			ImGui::DragFloat3((std::string("Position##") + std::to_string(id++)).c_str(), glm::value_ptr(t.model->position), 0.1f);
			ImGui::DragFloat3((std::string("Rotation##") + std::to_string(id++)).c_str(), glm::value_ptr(t.model->rotation), 0.1f);
			ImGui::DragFloat3((std::string("Scale##") + std::to_string(id++)).c_str(), glm::value_ptr(t.model->scale), 0.1f);


			uint32_t itexId = 0;
			if (t.textureLoaded) {
				itexId = t.texture->GetRendererID();
			}

			if (ImGui::ImageButton((ImTextureID)itexId, ImVec2(100, 100))) {
				if (t.textureLoaded)
					delete t.texture;
				std::string path = ShowOpenFileDialog();
				if (path.size() > 3) {
					t.texture = new Texture2D(path);
					t.textureLoaded = true;
				}
			}
		}
		ImGui::Separator();
	}

	ImGui::Separator();
	ImGui::Separator();

	if (ImGui::Button("Add Item")) {
		FoliageItem t;
		std::string tmp = ShowOpenFileDialog();
		t.modelDir = tmp.substr(0, tmp.rfind("\\"));
		t.name = tmp.substr(tmp.rfind("\\") + 1);
		t.model = LoadModel(tmp);
		t.model->SetupMeshOnGPU();
		t.model->UploadToGPU();
		fi.push_back(t);
	}

	ImGui::Separator();
	ImGui::Separator();
	ImGui::End();
}

void RenderFoliage(Shader* shader, Camera& camera)
{
	for (FoliageItem& t : fi) {
		if (!t.active)
			continue;
		t.model->Update();
		glm::mat4 tmp = t.model->modelMatrix * camera.pv;
		shader->Bind();
		shader->SetMPV(tmp);
		if (t.textureLoaded) {
			t.texture->Bind(5);
		}
		t.model->Render();
	}
}
