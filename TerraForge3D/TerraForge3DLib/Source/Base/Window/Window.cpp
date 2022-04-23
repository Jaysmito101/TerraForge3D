#include "Base/Window/Window.hpp"
#include "Base/Core/Core.hpp"

#include "GLFW/glfw3.h"

// TEMPORARY
#include "imgui/backends/imgui_impl_glfw.cpp"

static void GLFWErrorCallback(int error, const char* description)
{
	TF3D_LOG_ERROR("GLFW Error: {0}: {1}", error, description);
}

namespace TerraForge3D
{
	Window* Window::mainInstance = nullptr;

	// Static Window Mathods

	Window* Window::Create()
	{
		TF3D_ASSERT(mainInstance == nullptr, "Window already created");
		mainInstance = new Window();
		mainInstance->Setup();
		// TF3D_LOG("Main Window Created with UUID : {}", mainInstance->GetUUID())
		return mainInstance;
	}

	void Window::Destroy()
	{
		TF3D_ASSERT(mainInstance != nullptr, "Window not yet created");
		TF3D_SAFE_DELETE(mainInstance);
	}

	// Window member methods

	Window::Window()
	{
		windowUUID = UUID::Generate();
		
		if (!glfwInit())
		{
			TF3D_LOG_ERROR("Error in initializeing GLFW!");
			exit(-1);
		}

		glfwSetErrorCallback(GLFWErrorCallback);

#if defined(TF3D_OPENGL_BACKEND)
		glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
		// TerraForge3D uses OpenGL 4.6
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);

		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#elif defined(TF3D_VULKAN_BACKEND)
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#endif

		glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		windowHandle = glfwCreateWindow(width, height, "WINDOW", nullptr, nullptr);
#if defined(TF3D_OPENGL_BACKEND)
		glfwMakeContextCurrent(windowHandle);
#endif
	}

	void Window::Setup()
	{
		eventManager = new InputEventManager(windowHandle);
		eventManager->SetupInternalCallbacks();

		inputSystem = new InputSystem();
	}

	Window::~Window()
	{
		delete inputSystem;
		delete eventManager;
		glfwDestroyWindow(windowHandle);
		glfwTerminate();
	}

	void Window::Update()
	{
		glfwPollEvents();
		inputSystem->PollEvents();
	}

	void Window::SetTitle(std::string title)
	{
		glfwSetWindowTitle(windowHandle, title.data());
	}

	void Window::SetIcon(uint8_t* image, uint32_t width, uint32_t height)
	{
		GLFWimage images[1] = {0};
		TF3D_ASSERT(image != nullptr, "Icon image is null");
		images[0].pixels = image;
		images[0].width = width;
		images[0].height = height;
		TF3D_ASSERT(width > 0 && height > 0, "Icon image size mist be greater than 0");
		glfwSetWindowIcon(windowHandle, 1, images);
	}

	void Window::SetPosition(int32_t x, int32_t y)
	{
		glfwSetWindowPos(windowHandle, x, y);
	}

	void Window::SetSize(uint32_t width, uint32_t height)
	{
		glfwSetWindowSize(windowHandle, width, height);
	}

	int32_t Window::GetHeight()
	{
		glfwGetWindowSize(windowHandle, &width, &height);
		return height;
	}

	int32_t Window::GetWidth()
	{
		glfwGetWindowSize(windowHandle, &width, &height);
		return width;
	}
	int32_t Window::GetPositionX()
	{
		glfwGetWindowPos(windowHandle, &positionX, &positionY);
		return positionX;
	}

	int32_t Window::GetPositionY()
	{
		glfwGetWindowPos(windowHandle, &positionX, &positionY);
		return positionY;
	}

	std::pair<int32_t, int32_t> Window::GetSize()
	{
		glfwGetWindowSize(windowHandle, &width, &height);
		return { width, height };
	}

	std::pair<int32_t, int32_t> Window::GetPosition()
	{
		glfwGetWindowPos(windowHandle, &positionX, &positionY);
		return {positionX, positionY};
	}
}