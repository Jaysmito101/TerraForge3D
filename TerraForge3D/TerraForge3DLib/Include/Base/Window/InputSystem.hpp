#pragma once

#include "Base/Core/Core.hpp"
#include "Base/Window/KeyCodes.hpp"

struct GLFWwindow;

namespace TerraForge3D
{
	struct InputEventParams;

	/*
	* Unlike the InputEventManager class which is used for having callbacks for
	* certain events the InputSystem class is used to store a state of important
	* input methods like the keys pressed, also this can be used to query things
	* like mouse position, ...
	* 
	* See Also: TerraForge3D::InputEventManager (Base/Window/EventManager.hpp)
	*/
	class InputSystem
	{
	public:
		InputSystem();
		~InputSystem();

		void PollEvents(); /* Polls the mouse events */

		bool IsKeyPressed(KeyCode code); /* returns true if key is pressed else false */

		bool IsMouseButtonPressed(MouseButton button); /* returns true if the mouse button is pressed */

		std::pair<float, float> GetMousePosition(); /* returns the current mouse positon */

		std::pair<float, float> GetMouseDelta(); /* returns the current mouse delta */

		std::pair<float, float> GetMouseScrollDelta(); /* returns the current mouse scroll delta*/

		// Static Versions of Memeber methods
		inline static bool IsKeyPressedS(KeyCode code) { TF3D_ASSERT(mainInstance, "Input System not yet initialized!"); return mainInstance->IsKeyPressed(code); }
		inline static bool IsMouseButtonPressedS(MouseButton button) { TF3D_ASSERT(mainInstance, "Input System not yet initialized!"); return mainInstance->IsMouseButtonPressed(button); }
		inline static std::pair<float, float> GetMousePositionS() { TF3D_ASSERT(mainInstance, "Input System not yet initialized!"); return mainInstance->GetMousePosition(); }
		inline static std::pair<float, float> GetMouseDeltaS() { TF3D_ASSERT(mainInstance, "Input System not yet initialized!"); return mainInstance->GetMouseDelta(); }
		inline static std::pair<float, float> GetMouseScrollDeltaS() { TF3D_ASSERT(mainInstance, "Input System not yet initialized!"); return mainInstance->GetMouseScrollDelta(); }
		

	private:
		bool EventCallback(SharedPtr<InputEventParams> params);

	private:
		bool keyStates[512] = { false }; /* Stores the state of all the keys in KeyCode */
		bool mouseButtonStates[8] = { false }; /* Stores the state of all the mouse buttons in MouseButton */

		float mousePos[2] = { 0.0f };
		float mouseScrollDelta[2] = { 0.0f };
		float mouseDelta[2] = { 0.0f };

		GLFWwindow* windowHandle = nullptr;

		int callbackID = 0;

		static InputSystem* mainInstance;
	};
}
