#pragma once

#include "Base/Core/Core.hpp"
#include "Base/Window/EventManager.hpp"
#include "Base/Window/InputSystem.hpp"

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

		void Setup(); // This sets up the event managers, input system, ...

		// Getters / Setters
		inline UUID GetUUID() { return windowUUID; }


		// General Methods
		void Update(); // This updates the windows, polls the native window for events, ...
		void SetTitle(std::string title); // Sets the title of the window
		void SetIcon(uint8_t* image, uint32_t width, uint32_t height); // Sets the icon of the window
		void SetPosition(int32_t x, int32_t y); // Sets the position of the window
		void SetSize(uint32_t width, uint32_t height); // Sets the size of the window
		int32_t GetHeight(); // returns the height of the windoe
		int32_t GetWidth(); // returns the width of the windoe
		int32_t GetPositionX(); // returns the x position of the windoe
		int32_t GetPositionY(); // returns the y position of the windoe
		std::pair<int32_t, int32_t> GetSize();
		std::pair<int32_t, int32_t> GetPosition();

		inline GLFWwindow* GetHandle() { return windowHandle; } /* returns the internal glfw handler */

		// Static methods
		static Window* Create(); // Creates a window handle (error if window handle alrady exists)
		static void Destroy(); // Destroys the main window handle

		inline static void Set(Window* window) { TF3D_ASSERT(window, "Null Pointer Exception");  mainInstance = window; }
		inline static Window* Get() { TF3D_ASSERT(mainInstance, "Window not yet initialized!"); return mainInstance; }

	public:
		InputEventManager* eventManager = nullptr;
		InputSystem* inputSystem = nullptr;
	private:
		int32_t height = 600, width = 800;
		int32_t positionX=0, positionY = 0;
		UUID windowUUID;
		GLFWwindow* windowHandle = nullptr;

		static Window* mainInstance;
	};
}