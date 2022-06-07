#pragma once

#include "Base/Window/KeyCodes.hpp"

#include <array>

#ifndef TF3D_DEFAULT_CALLBACKS_CAPACITY
#define TF3D_DEFAULT_CALLBACKS_CAPACITY 16
#endif

struct GLFWwindow;

namespace TerraForge3D
{
	/*
	* These are the available event callbacks you can subscribe to.
	*/
	enum InputEventType
	{
		InputEventType_None = 0,
		InputEventType_WindowResize,
		InputEventType_DragNDrop,
		InputEventType_MouseMove,
		InputEventType_MouseScroll,
		InputEventType_MouseButton,
		InputEventType_Key,
		InputEventType_WindowClose,
		InputEventType_Count
	};

	/*
	* This is the Generic InputEventParams data structuore for all the callbacks
	*/
	struct InputEventParams
	{
		InputEventType type = InputEventType_None;
		std::vector<std::string> paths;
		float mouseScrollAmount[2] = { 0.0f };
		float mousePos[2] = {0.0f};
		float windowSize[2] = {0.0f};
		MouseButton mouseButton = MouseButton_Left;
		KeyCode keyCode = KeyCode_Enter;
		bool pressed = false;
	};

	
	/*
	* These are the event parameters for various events
	* These are optional.
	*/

	struct MouseMoveEventParams
	{
		float mouseX = 0.0f;
		float mouseY = 0.0f;
	};

	struct MouseButtonEventParams
	{
		MouseButton mouseButton ;
		bool pressed = false;
	};

	struct MouseScrollEventParams
	{
		float offsetX = 0.0f;
		float offsetY = 0.0f;
	};

	struct KeyEventParams
	{
		KeyCode key ;
		bool pressed = false;
	};

	struct WindowResizeEventParams
	{
		int width = 0;
		int height = 0;
	};

	struct DragNDropEventParams
	{
		std::vector<std::string> paths;
	};

	struct WindowCloseEventParams
	{
		// TODO: Place something relevent here
	};

	template<typename ParamType>
	using InputEventCallback = std::function<bool(ParamType*)>;


	/*
	* The InputEventManager class is for registering to callbacks for input events
	* like mouse move, mouse click, window close button clicked, etc.
	*/
	class InputEventManager
	{
	public:
		/*
		* The cosntuctors just initializes the members
		* 
		* This also sets up the internal callbacks with GLFW
		*/
		InputEventManager(GLFWwindow* windowHandle);
		~InputEventManager();
		
		void SetupInternalCallbacks();

		/* 
		* Register a callback for a particulat type of event.
		* 
		* This return an uint32_t Id which wou can use to Query the Callback or deregister 
		*/
		uint32_t RegisterCallback(InputEventCallback<InputEventParams> callback, std::vector<InputEventType> type);

		/*
		* Deregisters a callback with a given ID
		* 
		* If there is no callback for that ID an aassertion faliure occurs
		* 
		* Note: This must be called from Main Thread only
		*/
		bool DeregisterCallback(uint32_t callbackID);

		/*
		* This is a wrapper for the regular InputEventManager::RegisterCallback function
		* but here you can setup a callback only for a specific event type.
		*/

		inline uint32_t RegisterCallback(InputEventCallback<MouseMoveEventParams> callback)
		{
			return RegisterCallback([callback](InputEventParams* params) -> bool {
				MouseMoveEventParams data;
				data.mouseX = params->mousePos[0];
				data.mouseY = params->mousePos[1];
				return callback(&data);
			}, {InputEventType_MouseMove});
		}

		inline uint32_t RegisterCallback(InputEventCallback<MouseButtonEventParams> callback)
		{
			return RegisterCallback([callback](InputEventParams* params) -> bool {
				MouseButtonEventParams data;
				data.mouseButton = params->mouseButton;
				data.pressed = params->pressed;
				return callback(&data);
			}, {InputEventType_MouseButton});
		}

		inline uint32_t RegisterCallback(InputEventCallback<MouseScrollEventParams> callback)
		{
			return RegisterCallback([callback](InputEventParams* params) -> bool {
				MouseScrollEventParams data;
				data.offsetX = params->mouseScrollAmount[0];
				data.offsetY = params->mouseScrollAmount[1];
				return callback(&data);
			}, {InputEventType_MouseScroll});
		}

		inline uint32_t RegisterCallback(InputEventCallback<KeyEventParams> callback)
		{
			return RegisterCallback([callback](InputEventParams* params) -> bool {
				KeyEventParams data;
				data.key = params->keyCode;
				data.pressed = params->pressed;
				return callback(&data);
			}, {InputEventType_Key});
		}

		inline uint32_t RegisterCallback(InputEventCallback<WindowResizeEventParams> callback)
		{
			return RegisterCallback([callback](InputEventParams* params) -> bool {
				WindowResizeEventParams data;
				data.width = static_cast<int>(params->windowSize[0]);
				data.height = static_cast<int>(params->windowSize[1]);
				return callback(&data);
			}, {InputEventType_WindowResize});
		}

		inline uint32_t RegisterCallback(InputEventCallback<WindowCloseEventParams> callback)
		{
			return RegisterCallback([callback](InputEventParams* params) -> bool {
				WindowCloseEventParams data;
				return callback(&data);
			}, {InputEventType_WindowClose});
		}

		inline uint32_t RegisterCallback(InputEventCallback<DragNDropEventParams> callback)
		{
			return RegisterCallback([callback](InputEventParams* params) -> bool {
				DragNDropEventParams data;
				data.paths = params->paths;
				return callback(&data);
			}, {InputEventType_DragNDrop});
		}

		// The internal callbacks
		void DragNDropCallback(int pathCount, const char** paths);
		void WindowSizeCallback(int width, int height);
		void KeyCallback(int key, int scanCode, int action, int mods);
		void ScrollCallback(double xOffset, double yOffset);
		void MouseButtonCallback(int button, int action);
		void WindowCloseCallback();
		void MousePositionCallback(double xPos, double yPos);


		inline static void Set(InputEventManager* eventManager) { TF3D_ASSERT(eventManager, "Null Pointer Exception");  mainInstance = eventManager; }
		inline static InputEventManager* Get() { TF3D_ASSERT(mainInstance, "Input Event Manager not yet initialized!"); return mainInstance; }
	private:
		// Helpers
		void CallCallbacks();

	private:
		InputEventParams eventParams;
		std::unordered_map<uint32_t, InputEventCallback<InputEventParams>>  callbacks;
		std::array<std::vector<uint32_t>, InputEventType_Count> callbacksReference;
		GLFWwindow* windowHandle = nullptr;


		static InputEventManager* mainInstance;
	};
}