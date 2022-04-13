#include "Base/Core/Application.hpp"

namespace TerraForge3D
{
	Application* Application::mainInstance = nullptr;

	Application::Application()
	{
		mainInstance = this;
		applicationUUID = UUID::Generate();
	}

	Application::~Application()
	{
		if(logger)
			delete logger;
	}

	void Application::Run()
	{
		OnPreload();
		logger = Logger::Create(logFilePath);
		mainWindow = Window::Create();
		mainWindow->SetTitle(applicationName);
		OnStart();
		isRunning = true;
		while (isRunning)
		{
			OnUpdate();
			OnImGuiRender();
			mainWindow->Update();
		}
		OnEnd();
		Window::Destroy();
	}
}