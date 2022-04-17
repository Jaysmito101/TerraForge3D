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
		TF3D_SAFE_DELETE(logger);
		TF3D_SAFE_DELETE(vulkanContext);
		Window::Destroy();
	}

	void Application::Run()
	{
		OnPreload();
		logger = Logger::Create(logFilePath);
		mainWindow = Window::Create();
		mainWindow->SetTitle(applicationName);
		// Setup Vulkan
		TF3D_LOG_INFO("Initializing Vulkan");
		vulkanContext = new Vulkan::Context();
		OnStart();
		isRunning = true;
		while (isRunning)
		{
			OnUpdate();
			OnImGuiRender();
			mainWindow->Update();
		}
		OnEnd();
	}
}