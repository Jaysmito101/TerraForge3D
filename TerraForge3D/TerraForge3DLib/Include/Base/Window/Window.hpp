#pragma once

#include "Base/Core/Core.hpp"

#include <string>

struct GLFWwindow;

namespace TerraForge3D
{

	/*
	* This is an abstraction over GLFW to provide a window.
	* 
	* NOTE: This is made keeping in mind that there needs to be only one window
	*/
	class Window
	{
	public:
		/*
		* Ideally this are meant to be used only from Window::Create and Window::Destory
		*/
		Window();
		~Window();

		// Getters / Setters
		inline UUID GetUUID() { return windowUUID; }


		// General Methods
		void Update();
		void SetTitle(std::string title);
		void SetIcon(uint8_t* image, uint32_t width, uint32_t height);
		void SetPosition(int32_t x, int32_t y);
		void SetSize(uint32_t width, uint32_t height);

		// Static methods
		static Window* Create();
		static void Destroy();
		static Window* Get();

	private:
		static Window* mainInstance;

		UUID windowUUID;
		GLFWwindow* windowHandle = nullptr;
	};
}