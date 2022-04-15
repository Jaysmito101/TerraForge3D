#pragma once

#ifndef TF3D_DEFAULT_CALLBACKS_CAPACITY
#define TF3D_DEFAULT_CALLBACKS_CAPACITY 16
#endif

struct GLFWwindow;

namespace TerraForge3D
{
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

	struct InputEventParams
	{
		InputEventType type = InputEventType_None;
		std::vector<std::string> paths;
		float mouseScrollAmount[2] = { 0.0f };
		float mousePos[2] = {0.0f};
		float windowSize[2] = {0.0f};
		int mouseButton = 0;
		int keyCode = 0;
		bool pressed = false;
	};

	typedef std::function<bool(InputEventParams*)> InputEventCallback;

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
		uint32_t RegisterCallback(InputEventCallback callback, std::vector<InputEventType> type);

		/*
		* Deregisters a callback with a given ID
		* 
		* If there is no callback for that ID an aassertion faliure occurs
		* 
		* Note: This must be called from Main Thread only
		*/
		bool DeregisterCallback(uint32_t callbackID);


		// The internal callbacks
		void DragNDropCallback(int pathCount, const char** paths);
		void WindowSizeCallback(int width, int height);
		void KeyCallback(int key, int scanCode, int action, int mods);
		void ScrollCallback(double xOffset, double yOffset);
		void MouseButtonCallback(int button, int action);
		void WindowCloseCallback();
		void MousePositionCallback(double xPos, double yPos);

	private:
		// Helpers
		void CallCallbacks();

	private:
		InputEventParams eventParams;
		std::unordered_map<uint32_t, InputEventCallback>  callbacks;
		std::unordered_map<InputEventType, std::vector<uint32_t>> callbacksReference;
		GLFWwindow* windowHandle = nullptr;


		static InputEventManager* mainInstance;
	};
}