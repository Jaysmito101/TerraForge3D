#pragma once
#include "Base/Core/Core.hpp"
#include "Base/Core/Application.hpp"
#include "Utils/Utils.hpp"

#define VIEWPORT_COUNT 4


namespace TerraForge3D
{
	class Window;
	class MainMenu;
	class ProjectManager;
	class StartUpScreen;
	class Preferences;
	class Renderer;
	namespace UI 
	{
		class EditorManager;
		class ModalManager;
	}
	namespace JobSystem
	{
		class JobSystem;
	}
	// Editors
	class Viewport;
	class Inspector;
	

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
			std::string shaderIncludeDir = resourceDir + PATH_SEPERATOR "Shader";
		} appResourcePaths;

		struct 
		{
			Application* app = nullptr;
			Window* window = nullptr;
			std::unordered_map<std::string, ApplicationFont> fonts;
		} core;

		struct
		{
			SharedPtr<MainMenu> mainMenu ;
		} menus;

		struct
		{
			SharedPtr<UI::EditorManager> manager;
			SharedPtr<StartUpScreen> startUpScreen;
			SharedPtr<Viewport> viewports[VIEWPORT_COUNT];
			SharedPtr<Inspector> inspector;
		} editors;

		struct
		{
			SharedPtr<UI::ModalManager> manager;
		} modals;

		struct
		{
			SharedPtr<ProjectManager> manager;
		} project;
		
		struct
		{
			SharedPtr<JobSystem::JobSystem> manager ;
		} jobs;

		SharedPtr<Preferences> preferences;


		Renderer* renderer = nullptr;

		// TEMP
		SharedPtr<Mesh> mesh;

		static ApplicationState* Create();
		inline static ApplicationState* Get() { TF3D_ASSERT(appState, "Applicaiton Sate not yet created."); return appState; }
		static void Set(ApplicationState* appState);
		static void Destory();

	private:
		static ApplicationState* appState;
	};


	


}