#include "Base/Core/Application.hpp"

// TEMP
#include "GLFW/glfw3.h"

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
		double previousTime = glfwGetTime();
		double currentTime = 0;
		while (isRunning)
		{
			currentTime = glfwGetTime();
			deltaTime = currentTime - previousTime;
			previousTime = currentTime;
			deltaTime = deltaTime == 0 ? 0.001 : deltaTime;
			OnUpdate();
			renderer->BeginUI();
			OnImGuiRender();
			renderer->EndUI();
			mainWindow->Update();
		}
		OnEnd();
		Renderer::Destroy();
	}
}