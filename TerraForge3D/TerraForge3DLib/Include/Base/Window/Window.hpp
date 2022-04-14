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
		void Update(); // This updates the windows, polls the native window for events, ...
		void SetTitle(std::string title); // Sets the title of the window
		void SetIcon(uint8_t* image, uint32_t width, uint32_t height); // Sets the icon of the window
		void SetPosition(int32_t x, int32_t y); // Sets the position of the window
		void SetSize(uint32_t width, uint32_t height); // Sets the size of the window

		// Static methods
		static Window* Create(); // Creates a window handle (error if window handle alrady exists)
		static void Destroy(); // Destroys the main window handle
		static Window* Get(); // Returns the current window handle (error if Window::Create has not been called before)

	private:
		static Window* mainInstance;

		UUID windowUUID;
		GLFWwindow* windowHandle = nullptr;
	};
}