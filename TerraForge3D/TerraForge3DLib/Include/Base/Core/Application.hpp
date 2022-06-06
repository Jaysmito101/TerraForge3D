#pragma once

#include "json/json.hpp"

#include "Base/Core/Core.hpp"
#include "Base/Window/Window.hpp"
#include "Base/Renderer/Renderer.hpp"

struct ImFont;

namespace TerraForge3D
{

	struct ApplicationFont
	{
		std::string path = "";
		std::string name = "Font";
		ImFont* handle = nullptr;
		float size = 16.0f;
		bool isIconFont = false;
	};

	// -----------------

	/**
	* The primary application class which owns everything
	*/
	class Application
	{
	public:
		Application();
		virtual ~Application();

	protected:
		// Utility Functions for inherited classes
		// This functions can only bbe called from OnPreload method
		inline void SetLogFilePath(std::string path) { logFilePath = path; };
		inline void SetApplicationName(std::string name) { applicationName = name; };
		inline void SetWindowConfigPath(std::string path) { windowConfigPath = path; };
		inline void SetFontConfig(std::string config, std::string dir) { fonts.clear(); fontConfigs = config; fontsDir = dir; }

	public:
		// Some General Getters/Setters
		inline UUID& GetUUID() { return applicationUUID; };
		inline UUID GetUUID() const { return applicationUUID; };

		inline void Close() { isRunning = false; }

		inline std::unordered_map<std::string, ApplicationFont>& GetFonts() { return fonts; };
		inline Window* GetWindow() { return mainWindow; }
		inline InputEventManager* GetInputEventManager() { return mainWindow->eventManager; }

		/*
		* This is the entire lifecycle of the Application
		*/
		void Run();

		// Functions meant to be overloaded by clients

		/*
		* This function ic called once before startup.
		*
		* NOTE: You are only suppossed to initialize application vaiables in this function.
		*		Calling any other TerraForge3DLib function will result in unexpected behaviour.
		*/
		virtual void OnPreload() = 0;

		/*
		* Called once on application start
		*/
		virtual void OnStart() = 0;

		/*
		* Called every frame
		*/
		virtual void OnUpdate() = 0;

		/*
		* Called every frame
		*/
		virtual void OnImGuiRender() = 0;

		/*
		* Called once before shutdown
		*/
		virtual void OnEnd() = 0;

		inline static void Set(Application* application) { TF3D_ASSERT(application, "Null Pointer Exception");  mainInstance = application; }
		inline static Application* Get() { TF3D_ASSERT(mainInstance, "Application not yet initialized!"); return mainInstance; }


	private:
		static Application* mainInstance;

	public:
		SharedPtr<Logger> logger;
		Renderer* renderer = nullptr;
		bool isRunning = false;
		double deltaTime = 0.0;

		UUID applicationUUID;

		// Application Data
		std::string applicationName = "TerraForge3D";
		std::string logFilePath = "TerraForge3D.log";
		std::string windowConfigPath = "WindowConfigs.ini";
		std::unordered_map<std::string, ApplicationFont> fonts;
		std::string fontConfigs = "";
		std::string fontsDir = "";

		// Application Window
		Window* mainWindow = nullptr;
	};

}