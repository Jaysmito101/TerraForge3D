#pragma once
#include "Base/Core/Core.hpp"

namespace TerraForge3D
{

	namespace UI
	{

		class Editor;

		class EditorManager
		{
		public:
			EditorManager(std::string name);
			virtual ~EditorManager();

			void Show();
			void Update();

			SharedPtr<Editor> AddEditor(SharedPtr<Editor> editor);

			inline std::vector<SharedPtr<Editor>> GetEditors() { return editors; }

			inline const std::vector<SharedPtr<Editor>>::iterator& begin() { return editors.begin(); }
			inline const std::vector<SharedPtr<Editor>>::iterator& end() { return editors.end(); }

		private:
			std::string name = "Editor Manager";
			UUID uid;
			std::string uidStr = "";
			std::vector<SharedPtr<Editor>> editors;
		};

	}

}