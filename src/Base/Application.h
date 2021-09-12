#pragma once

#include <Window.h>

class Application {
public:
	Application();
	~Application();
	virtual void OnUpdate(float deltatime) {};
	virtual void OnOneSecondTick() {};
	virtual void OnImGuiRender(float deltatime) {};
	virtual void OnStart() {};
	virtual void OnEnd() {};

	void Render();
	void ImGuiRenderBegin();
	void ImGuiRenderEnd();
	void Run();

	inline void Close() { isActive = false; }

	Window* GetWindow() { return m_Window; }

	static inline Application* Get() { return s_App; }
private:
	float previousTime;
	bool isActive;
	Window* m_Window;
private:
	static Application* s_App;
};