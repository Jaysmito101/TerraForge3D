#pragma once

struct GLFWwindow;

#include <functional>
#include <string>

using EventFn = std::function<void(int, int)>;

struct ClearColor {
	ClearColor() {
		r = g = b = 0;
	}

	ClearColor(float r, float g, float b)
		: r(r), g(g), b(b) {}

	float r;
	float g;
	float b;
};

class Window {
public:
	Window(std::string title);
	~Window();

	void SetShouldCloseCallback(EventFn callbackFunction);
	void SetResizeCallback(EventFn callbackFunction);
	void SetMouseCallback(EventFn callbackFunction);

	void SetVSync(bool enabled);
	void MakeCurrentContext();
	void SetClearColor(ClearColor color);
	void Update();
	void Close();
	void Clear();
	void SetFullScreen(bool fullscreen);
	void SetVisible(bool visibility);

	inline bool IsVSyncEnabled() { return vSyncState; }
	inline GLFWwindow* GetNativeWindow() { return m_Window; }

private:
	bool isActive, vSyncState, isFullscreen = false;
	GLFWwindow* m_Window;
	EventFn m_CloseEventCallback, m_ResizeEventCallback, m_MouseEventCallback;
	ClearColor m_ClearColor;
};