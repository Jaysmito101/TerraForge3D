#pragma once

struct GLFWwindow;

#include <functional>

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
	Window();
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

	inline bool IsVSyncEnabled() { return vSyncState; }
	inline GLFWwindow* GetNativeWindow() { return m_Window; }

private:
	bool isActive, vSyncState;
	GLFWwindow* m_Window;
	EventFn m_CloseEventCallback, m_ResizeEventCallback, m_MouseEventCallback;
	ClearColor m_ClearColor;
};