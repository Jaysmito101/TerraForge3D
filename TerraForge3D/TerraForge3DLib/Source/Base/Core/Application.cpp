#include "Base/Core/Application.hpp"

namespace TerraForge3D
{
	Application* Application::mainInstance = nullptr;

	Application::Application()
	{
		logger = Logger::Create();
		TF3D_ASSERT(mainInstance == nullptr, "Application already Initialized!")
		mainInstance = this;
	}

	Application::~Application()
	{
		if(logger)
			delete logger;
	}

	void Application::Run()
	{
		OnStart();
		isRunning = true;
		while (isRunning)
		{
			OnUpdate();
			OnImGuiRender();
		}
		OnEnd();
	}
}