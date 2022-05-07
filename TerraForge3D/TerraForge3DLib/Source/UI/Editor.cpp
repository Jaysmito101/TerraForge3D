#include "UI/Editor.hpp"
#include "UI/EditorManager.hpp"
#include "UI/Menu.hpp"

#include "imgui/imgui.h"

namespace TerraForge3D
{

	namespace UI
	{



		Editor::Editor(std::string name)
		{
			this->name = name;
			this->uid = UUID::Generate();
			this->uidStr = this->uid.ToString();
			this->menu = new Menu();
			this->menu->SetMainMenu(false);
			this->menu->SetEnabled(false);
			this->subEditorManager = new EditorManager(name + " [Editor Manager]");
		}

		Editor::~Editor()
		{
			TF3D_SAFE_DELETE(this->menu);
			TF3D_SAFE_DELETE(this->subEditorManager);
		}

		void Editor::Update()
		{
			OnUpdate();
			subEditorManager->Update();
		}

		void Editor::RenderUI()
		{
			if (menu->IsEnabled())
				windowFlags |= ImGuiWindowFlags_MenuBar;

			if (isEnabled && isVisible)
			{
				if ( ImGui::Begin(name.data(), &isVisible, windowFlags))
				{
					ImGui::PushID(uidStr.data());
					menu->Show();
					OnUIRender();
					ImGui::PopID();
				}
				ImGui::End();
				subEditorManager->RenderUI();
			}
		}

		void Editor::Setup()
		{
			OnStart();
		}

		void Editor::Shutdown()
		{
			OnEnd();
		}

	}

}