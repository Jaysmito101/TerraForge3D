                           #pragma once

#include <Window.h>
#include <string>


class Application {
public:
	Application();
	~Application();
	virtual void OnUpdate(float) {};
	virtual void OnOneSecondTick() {};
	virtual void OnImGuiRender() {};
	virtual void OnStart() {};
	virtual void OnEnd() {};
	virtual void OnPreload() {};

	void SetTitle(std::string title);
	void Init();
	bool IsActive();
	void RenderImGui();
	void Render();
	void ImGuiRenderBegin();
	void ImGuiRenderEnd();
	void Run();

	inline void Close() { isActive = false; }

	Window* GetWindow() { return m_Window; }

	static inline Application* Get() { return s_App; }
private:
	std::string m_WindowTitle = "Main Window";
	float previousTime;
	bool isActive;
	Window* m_Window;
private:
	static Application* s_App;
};                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     