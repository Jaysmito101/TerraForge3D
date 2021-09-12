#pragma once

#include <Application.h>


#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

static void Log(const char* log) {
	std::cout << log << std::endl;
};

static void InitGlad() {
	if (!gladLoadGL()) {
		Log("Failed to Initialize GLAD!");
		exit(-1);
	}
}

static void InitImGui() {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}
	ImGui_ImplGlfw_InitForOpenGL(static_cast<GLFWwindow*>(Application::Get()->GetWindow()->GetNativeWindow()) , true);
	ImGui_ImplOpenGL3_Init("#version 330");
}

static void ImGuiShutdown() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

Application* Application::s_App;

Application::Application() 
{
	m_Window = new Window();
	m_Window->SetVSync(true);
	isActive = true;
	s_App = this;
}

void Application::Render()
{

}

void Application::ImGuiRenderBegin() 
{
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplGlfw_NewFrame();
	ImGui_ImplOpenGL3_NewFrame();
	ImGui::NewFrame();
}

void Application::ImGuiRenderEnd() 
{
	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		GLFWwindow* backup_current_context = glfwGetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		glfwMakeContextCurrent(backup_current_context);
	}
}

void Application::Run() 
{
	InitGlad();
	InitImGui();
	OnStart();
	float oneSecCounter = 0;
	while (isActive) {
		float currentTime = glfwGetTime();
		float deltaTime = currentTime - previousTime;
		previousTime = currentTime;
		oneSecCounter += deltaTime;
		m_Window->Clear();
		OnUpdate(deltaTime);
		ImGuiRenderBegin();		
		OnImGuiRender(deltaTime);
		if (oneSecCounter >= 1)
		{
			OnOneSecondTick();
			oneSecCounter = 0;
		}
		ImGuiRenderEnd();
		Render();
		m_Window->Update();
	}
	OnEnd();
	ImGuiShutdown();
}

Application::~Application()
{
	std::cout << "Shutting down Application" << std::endl;
	delete m_Window;
}