#pragma once

#include "Base/Core/Core.hpp"

namespace TerraForge3D
{

	/**
	* The primary application class which owns everything
	*/
	class Application
	{
	public:
		Application();
		~Application();
		
		/*
		* This is the entire lifecycle of the Application
		*/
		void Run();

		// Functions to be overloaded by clients
		
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

	private:
		static Application* mainInstance;

		Logger* logger = nullptr;
		bool isRunning = false;
	};

}