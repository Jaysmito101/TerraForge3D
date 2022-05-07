#pragma once
#include "Base/Core/Core.hpp"
#include "Utils/Utils.hpp"


namespace TerraForge3D
{
	class Application;
	class Window;
	class MainMenu;
	namespace UI {
		class EditorManager;
	}

	/*
	* This class holds all the managers, job handlers, every thing of this application
	*/
	class ApplicationState
	{
	private:
		ApplicationState() = default;
		~ApplicationState() = default;
	public:

		const struct 
		{
			std::string version = TF3D_VERSION_STRING;
			std::string platform = TF3D_PLATFORM;
			std::string backend = TF3D_BACKEND;
			std::string name = std::string("TerraForge3D v") + version + " [" TF3D_PLATFORM "] [" TF3D_BACKEND "] - Jaysmito Mukherjee";
			std::string maintainers = {
				"Jaysmito Mukherjee"
			};
			uint32_t buildID = __TIME__[0] * __TIME__[1] + __TIME__[3] + __TIME__[4];
		} meta;

		const struct
		{
			std::string executablePath = Utils::GetExecutablePath();
			std::string executableDir = Utils::GetExecetableDirectory();
			std::string resourceDir = executableDir + PATH_SEPERATOR "Data";
			std::string logsDir = resourceDir + PATH_SEPERATOR "Logs";
			std::string currentLogFilePath = logsDir + PATH_SEPERATOR + Utils::GetTimeStamp() + ".log";
			std::string configDir = resourceDir + PATH_SEPERATOR "Configs";
			std::string windowConfigPath = configDir + PATH_SEPERATOR "WindowArrangement.ini";
			std::string preferencesPath = configDir + PATH_SEPERATOR "UserPreferences.json";
			std::string stylesDir = resourceDir + PATH_SEPERATOR "Styles";
			std::string fontsDir = resourceDir + PATH_SEPERATOR "Fonts";
			std::string fontsConfigPath = configDir + PATH_SEPERATOR "Fonts.json";
		} appResourcePaths;

		struct 
		{
			Application* app = nullptr;
			Window* window = nullptr;
		} core;

		struct
		{
			MainMenu* mainMenu = nullptr;
		} menus;

		struct
		{
			UI::EditorManager* manager = nullptr;
		} editors;
		

		static ApplicationState* Create();
		inline static ApplicationState* Get() { TF3D_ASSERT(appState, "Applicaiton Sate not yet created."); return appState; }
		static void Set(ApplicationState* appState);
		static void Destory();

	private:
		static ApplicationState* appState;
	};


	


}