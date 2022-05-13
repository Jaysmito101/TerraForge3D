#pragma once
#include "Base/Base.hpp"

#include "json/json.hpp"

namespace TerraForge3D
{
	class ApplicationState;

	class ProjectManager
	{
	public:
		ProjectManager(ApplicationState* appState);
		~ProjectManager();

		bool Create(std::string path);
		bool Load(std::string path);
		void Close();
		void Update();

		bool Save();

	private:
		ApplicationState* appState = nullptr;

	public:
		std::string projectDir = "";
		std::string name = "";
		bool isOpen = false;
	};
}
