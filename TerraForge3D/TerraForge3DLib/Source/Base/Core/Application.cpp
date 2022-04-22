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
	}

	void Application::Run()
	{
		OnPreload();
		logger = Logger::Create(logFilePath, "TerraForge3D");
		mainWindow = Window::Create();
		mainWindow->SetTitle(applicationName);
		// Setup Vulkan
		TF3D_LOG_INFO("Initializing Render");
		renderer = Renderer::Create();
		OnStart();
		isRunning = true;
		while (isRunning)
		{
			OnUpdate();
			renderer->BeginImGui();
			OnImGuiRender();
			renderer->EndImGui();
			mainWindow->Update();
		}
		OnEnd();
		Renderer::Destroy();
		Window::Destroy();
		Logger::Destroy();
	}
}