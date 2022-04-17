#include "Base/Core/Core.hpp"
#include "Base/Window/EventManager.hpp"

#include "GLFW/glfw3.h"

namespace TerraForge3D
{
	static uint32_t CurrentEventID = 1;

	InputEventManager* InputEventManager::mainInstance = nullptr;

	InputEventManager::InputEventManager(GLFWwindow* wh)
	{
		TF3D_ASSERT(wh, "Window is NULL");
		windowHandle = wh;
		callbacks.clear();
		callbacks.reserve(TF3D_DEFAULT_CALLBACKS_CAPACITY);
		mainInstance = this;
	}

	void InputEventManager::SetupInternalCallbacks()
	{
		// Register the GLFW Window Handles

		glfwSetDropCallback(windowHandle, [](GLFWwindow* window, int pathCount, const char** paths) {
			mainInstance->DragNDropCallback(pathCount, paths);
			});
		glfwSetWindowSizeCallback(windowHandle, [](GLFWwindow* window, int width, int height) {
			mainInstance->WindowSizeCallback(width, height);
			});
		glfwSetKeyCallback(windowHandle, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
			mainInstance->KeyCallback(key, scancode, action, mods);
			});
		glfwSetScrollCallback(windowHandle, [](GLFWwindow* window, double xOffset, double yOffset) {
			mainInstance->ScrollCallback(xOffset, yOffset);
			});
		glfwSetMouseButtonCallback(windowHandle, [](GLFWwindow* window, int button, int action, int mods) {
			mainInstance->MouseButtonCallback(button, action);
			});
		glfwSetCursorPosCallback(windowHandle, [](GLFWwindow* window, double xPos, double yPos) {
			mainInstance->MousePositionCallback(xPos, yPos);
			});
		glfwSetWindowCloseCallback(windowHandle, [](GLFWwindow* window) {
			mainInstance->WindowCloseCallback();
			});
	}

	InputEventManager::~InputEventManager()
	{
		callbacks.clear();
	}

	uint32_t InputEventManager::RegisterCallback(std::function<bool(InputEventParams*)> callback, std::vector<InputEventType> types)
	{
		TF3D_ASSERT(callback, "Callback is NULL");
		uint32_t currentEventID = CurrentEventID++;
		callbacks[currentEventID] = callback;
		for (InputEventType type : types)
		{
			callbacksReference[type].push_back(currentEventID);
		}
		return currentEventID;
	}

	bool InputEventManager::DeregisterCallback(uint32_t callbackID)
	{
		TF3D_ASSERT(callbacks.find(callbackID) != callbacks.end(), "Callback Not Found!");
		callbacks.erase(callbackID);
		for (auto& value : callbacksReference)
		{
			for (int i = 0 ; i < value.size(); i++)
			{
				if (value[i] == callbackID)
				{
					value.erase(value.begin() + i);
					break;
				}
			}
		}
		return true; // for future use
	}

	void InputEventManager::DragNDropCallback(int pathCount, const char** paths)
	{
		eventParams.paths.clear();
		for (int i = 0; i < pathCount; i++)
		{
			eventParams.paths.push_back(std::string(paths[i]));
		}
		eventParams.type = InputEventType_DragNDrop;
		CallCallbacks();
		eventParams.paths.clear();
	}

	void InputEventManager::WindowSizeCallback(int width, int height)
	{
		eventParams.type = InputEventType_WindowResize;
		eventParams.windowSize[0] = static_cast<float>(width);
		eventParams.windowSize[1] = static_cast<float>(height);
		CallCallbacks();
	}

	void InputEventManager::KeyCallback(int key, int scanCode, int action, int mods)
	{
		eventParams.type = InputEventType_Key;
		eventParams.pressed = (action == GLFW_REPEAT || action == GLFW_PRESS);
		eventParams.keyCode = static_cast<KeyCode>(key);
		CallCallbacks();
	}

	void InputEventManager::ScrollCallback(double xOffset, double yOffset)
	{
		eventParams.type = InputEventType_MouseScroll;
		eventParams.mouseScrollAmount[0] = static_cast<float>(xOffset);
		eventParams.mouseScrollAmount[1] = static_cast<float>(yOffset);
		CallCallbacks();
	}

	void InputEventManager::MouseButtonCallback(int button, int action)
	{
		eventParams.type = InputEventType_MouseButton;
		eventParams.pressed = (action == GLFW_REPEAT || action == GLFW_PRESS);
		eventParams.mouseButton = static_cast<MouseButton>(button);
		CallCallbacks();
	}

	void InputEventManager::WindowCloseCallback()
	{
		eventParams.type = InputEventType_WindowClose;
		CallCallbacks();
	}

	void InputEventManager::MousePositionCallback(double xPos, double yPos)
	{
		eventParams.type = InputEventType_MouseMove;
		eventParams.mousePos[0] = static_cast<float>(xPos);
		eventParams.mousePos[1] = static_cast<float>(yPos);
		CallCallbacks();
	}

	void InputEventManager::CallCallbacks()
	{
		std::vector<uint32_t>& callbackIDs = callbacksReference[eventParams.type];
		for (auto& callbackID : callbackIDs)
		{
			if (callbacks[callbackID](&eventParams))
			{
				// break;
			}
		}
	}
}
