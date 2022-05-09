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

		void Create();

		bool Load(std::string path);

	private:

	public:
		std::string projectDir = "";
		nlohmann::json projectMeta;
	};
}
