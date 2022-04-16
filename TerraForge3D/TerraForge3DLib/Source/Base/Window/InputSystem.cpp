#include "Base/Window/InputSystem.hpp"
#include "Base/Window/EventManager.hpp"
#include "Base/Window/Window.hpp"

#include "GLFW/glfw3.h"

namespace TerraForge3D
{
	InputSystem* InputSystem::mainInstance = nullptr;

	InputSystem::InputSystem()
	{
		TF3D_ASSERT(mainInstance == nullptr, "Input System Already Initialized");

		windowHandle = Window::Get()->GetHandle();
		TF3D_ASSERT(windowHandle, "Window not yet initialized");

		// Setup The Calbacks
		TF3D_ASSERT(InputEventManager::Get(), "Input Event Manager not yet initialized");
		
		callbackID = InputEventManager::Get()->RegisterCallback(std::bind(&InputSystem::EventCallback, this, std::placeholders::_1), {InputEventType_Key, InputEventType_MouseMove, InputEventType_MouseButton, InputEventType_MouseScroll});
		
		mainInstance = this;
	}

	InputSystem::~InputSystem()
	{
		if (InputEventManager::Get())
		{
			InputEventManager::Get()->DeregisterCallback(callbackID);
		}
	}

	bool InputSystem::IsKeyPressed(KeyCode code)
	{
		return keyStates[code];
	}

	bool InputSystem::IsMouseButtonPressed(MouseButton button)
	{
		return mouseButtonStates[button];
	}

	std::pair<float, float> InputSystem::GetMousePosition()
	{

		return { mousePos[0] , mousePos[1] };
	}

	std::pair<float, float> InputSystem::GetMouseDelta()
	{
		return { mouseDelta[0] , mouseDelta[1] };
	}

	std::pair<float, float> InputSystem::GetMouseScrollDelta()
	{
		return { mouseScrollDelta[0] , mouseScrollDelta[1] };
	}

	void InputSystem::PollEvents()
	{
		TF3D_ASSERT(windowHandle, "Window Handler is NULL");
		double mpx, mpy;
		glfwGetCursorPos(windowHandle, &mpx, &mpy);
		mouseDelta[0] = static_cast<float>(mpx) - mousePos[0];
		mouseDelta[1] = static_cast<float>(mpy) - mousePos[1];

		mousePos[0] = static_cast<float>(mpx);
		mousePos[1] = static_cast<float>(mpy);
	}

	bool InputSystem::EventCallback(InputEventParams* params)
	{
		switch (params->type)
		{ // Switch Begin
		case InputEventType_MouseMove:
		{
			// Not needed as of now
			break;
		}
		case InputEventType_MouseButton:
		{
			mouseButtonStates[params->mouseButton] = params->pressed;
			break;
		}
		case InputEventType_MouseScroll:
		{
			mouseScrollDelta[0] = params->mouseScrollAmount[0];
			mouseScrollDelta[1] = params->mouseScrollAmount[1];
			break;
		}
		case InputEventType_Key:
		{
			keyStates[params->keyCode] = params->pressed;
			break;
		}
		} // Switch End
		return true;
	}

}