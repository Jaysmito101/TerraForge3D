#include "UI/EditorManager.hpp"
#include "UI/Editor.hpp"

#include "imgui/imgui.h"

namespace TerraForge3D
{

	namespace UI
	{



		EditorManager::EditorManager(std::string name)
		{
			this->name = name;
			this->uid = UUID::Generate();
			this->uidStr = this->uid.ToString();
			this->editors.clear();
		}

		EditorManager::~EditorManager()
		{
			for (auto& editor : editors)
			{
				editor->Shutdown();
				TF3D_SAFE_DELETE(editor);
			}
		}

		void EditorManager::RenderUI()
		{
			for (auto& editor : editors)
			{
				ImGui::PushID(uidStr.data());
				editor->RenderUI();
				ImGui::PopID();
			}
		}

		void EditorManager::Update()
		{
			for (auto& editor : editors)
			{
				editor->Update();
			}
		}

		void EditorManager::AddEditor(Editor* editor)
		{
			TF3D_ASSERT(editor, "Cannot add NULL editor");
			this->editors.push_back(editor);
			editor->Setup();
		}

	}

}