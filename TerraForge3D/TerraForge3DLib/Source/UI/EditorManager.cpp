#include "UI/EditorManager.hpp"
#include "TerraForge3D.hpp"

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
				// TF3D_SAFE_DELETE(editor);
			}
			editors.clear();
		}

		void EditorManager::Show()
		{
			for (auto& editor : editors)
			{
				ImGui::PushID(uidStr.data());
				editor->Show();
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

		SharedPtr<Editor> EditorManager::AddEditor(SharedPtr<Editor> editor)
		{
			TF3D_ASSERT(editor, "Cannot add NULL editor");
			this->editors.push_back(editor);
			editor->Setup();
			return editor;
		}

	}

}