#include "Terrain/Manager.hpp"
#include "Data/ApplicationState.hpp"
#include "Terrain/Generators/Generator.hpp"
#include "UI/Modals.hpp"
#include "UI/Editor.hpp"

namespace TerraForge3D
{

	namespace Terrain
	{

		Manager::Manager(ApplicationState* as)
		{
			this->appState = as;
			this->mesh = new Mesh("MainTerrain");
			this->mesh->Clear();
			this->Resize(TF3D_DEFAULT_TERRAIN_RESOLUTION, TF3D_DEFAULT_TERRAIN_SCALE);
			this->mesh->UploadToGPU();
		}

		Manager::~Manager()
		{
			for (auto& gen : generators)
			{
				gen->OnDetach();
			}
		}

		void Manager::Update()
		{
		}

		void Manager::AddGenerator(SharedPtr<Generator> generator)
		{
			this->generators.push_back(generator);
			this->generators.back()->OnAttach();
		}

		void Manager::ShowGeneratorSettings()
		{
			static char buffer[128] = {0};
			int i = 0;
			ImVec4 currentChildBgColor = ImGui::GetStyle().Colors[ImGuiCol_ChildBg];
			currentChildBgColor.x = 0.1f;
			currentChildBgColor.y = 0.1f;
			currentChildBgColor.z = 0.1f;
			ImGui::PushStyleColor(ImGuiCol_ChildBg, currentChildBgColor);
			for (auto& generator : generators)
			{
				sprintf(buffer, "##GeneratorSettingsHeader%d", i++);
				ImGui::BeginChild(buffer, ImVec2(ImGui::GetWindowWidth(), 100));
				Utils::ImGuiC::PushSubFont(appState->core.fonts["SemiHeader"].handle);
				ImGui::Text(generator->name.data());
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Double click to edit name");
				if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
					appState->modals.manager->InputBox("Enter Name", [&](std::string input, std::string& result)->bool { generator->name = input; return true; });
				Utils::ImGuiC::PopSubFont();
				if (ImGui::Button("Edit"))
				{
					generator->editor->SetVisible(true);
				}
				ImGui::EndChild();
				ImGui::Separator();
			}
			ImGui::PopStyleColor();
		}

		void Manager::Resize(uint32_t res, float sc, float* progress)
		{
			//if (progress)
			//	*progress = 0.0f;
			this->isBeingResized = true;
			this->resolution = res;
			this->scale = sc;
			this->mesh->Plane(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), resolution, scale, progress);
			mesh->RecalculateNormals();
			this->isBeingResized = false;
		}

	}
}