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
			~EditorManager();

			void Show();
			void Update();

			void AddEditor(Editor* editor);

			inline std::vector<Editor*> GetEditors() { return editors; }

			inline const std::vector<Editor*>::iterator& begin() { return editors.begin(); }
			inline const std::vector<Editor*>::iterator& end() { return editors.end(); }

		private:
			std::string name = "Editor Manager";
			UUID uid;
			std::string uidStr = "";
			std::vector<Editor*> editors;
		};

	}

}