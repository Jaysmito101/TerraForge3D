#include <Window.h>

#include <GLFW/glfw3.h>

#include <iostream>

static bool isGLFWInitialized = false;

static void GLFWErrorCallback(int error, const char* description)
{
	fprintf(stderr, "GLFW Error: %d: %s\n", error, description);
}

static void InitGLFW()
{
	if (isGLFWInitialized)
		return;
	if (!glfwInit()) {
		std::cout << "Error in initializeing GLFW!" << std::endl;
		exit(-1);
	}
	glfwSetErrorCallback(GLFWErrorCallback);
	glfwWindowHint(GLFW_MAXIMIZED, GL_TRUE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	isGLFWInitialized = true;
}


Window::Window(std::string title = "Window") 
{
	InitGLFW();
	m_Window = glfwCreateWindow(640, 480, title.c_str(), NULL, NULL);
	if (!m_Window)
	{
		glfwTerminate();
		std::cout << "Error in creating window!" << std::endl;
		exit(-1);
	}
	isActive = true;
	glfwMakeContextCurrent(m_Window);
}

void Window::SetVSync(bool enabled)
{
	if (enabled == vSyncState)
		return;
	if (enabled) {
		glfwSwapInterval(1);
		vSyncState = enabled;
	}
	else {
		glfwSwapInterval(0);
		vSyncState = enabled;
	}
}

void Window::Clear() 
{
	glClearColor(m_ClearColor.r, m_ClearColor.g, m_ClearColor.b, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Window::SetFullScreen(bool fullscreen)
{
	if (isFullscreen == fullscreen)
		return;

	static int x, y;
	static int sx, sy;

	isFullscreen = fullscreen;

	if (fullscreen)
	{
		
		// backup window position and window size
		glfwGetWindowPos(m_Window, &x, &y);
		glfwGetWindowSize(m_Window, &sx, &sy);

		// get resolution of monitor
		const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

		// switch to full screen
		glfwSetWindowMonitor(m_Window, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, 0);
	}
	else
	{
		// restore last window size and position
		glfwSetWindowMonitor(m_Window, nullptr, x, y, sx, sy, 0);
	}

}

void Window::SetVisible(bool visibility)
{
	if (visibility)
		glfwShowWindow(m_Window);
	else
		glfwHideWindow(m_Window);
}

void Window::Update()
{
	if (!isActive)
		return;
	if (glfwWindowShouldClose(m_Window)) {
		if(m_CloseEventCallback)
			m_CloseEventCallback(0, 0);
		isActive = false;
	}
	glfwSwapBuffers(m_Window);
	glfwPollEvents();
}

void Window::SetMouseCallback(EventFn callback)
{
	m_MouseEventCallback = callback;
}

void Window::SetResizeCallback(EventFn callback) 
{
	m_ResizeEventCallback = callback;
}

void Window::SetShouldCloseCallback(EventFn callback)
{
	m_CloseEventCallback = callback;
}

void Window::SetClearColor(ClearColor color)
{
	m_ClearColor = color;
}

void Window::MakeCurrentContext() 
{
	if (m_Window) {
		glfwMakeContextCurrent(m_Window);
	}
}

Window::~Window() 
{
	Close();
	std::cout << "Destroying Window!" << std::endl;
}

void Window::Close() 
{
	if (m_Window) {
		glfwDestroyWindow(m_Window);
	}
}