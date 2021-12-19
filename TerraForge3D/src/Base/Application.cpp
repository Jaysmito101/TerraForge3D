#pragma once

#include <Application.h>
#include "implot.h"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <imnodes/imnodes.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>

static void Log(const char* log) {
	std::cout << log << std::endl;
};

static void InitGlad() {
	if (!gladLoadGL()) {
		Log("Failed to Initialize GLAD!");
		exit(-1);
	}
}

static void InitImGui(std::string& configPath) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();
	ImNodes::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.IniFilename = configPath.c_str();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	io.ConfigViewportsNoTaskBarIcon = false;
	io.ConfigViewportsNoAutoMerge = true;
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
	ImNodes::DestroyContext();
	ImPlot::DestroyContext();
	ImGui::DestroyContext();
}

Application* Application::s_App;



Application::Application()
{
}

void Application::SetWindowConfigPath(std::string path)
{
	windowConfigPath = path;
}

void Application::SetTitle(std::string title)
{
	m_WindowTitle = title;
}

void Application::Init() {
	m_Window = new Window(m_WindowTitle);
	m_Window->SetVSync(true);
	m_Window->SetVisible(false);
	isActive = true;
	s_App = this;
	InitGlad();
	InitImGui(windowConfigPath);
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

bool Application::IsActive()
{
	return isActive;
}

void Application::RenderImGui() {
	glEnable(GL_BLEND);
	ImGuiRenderBegin();
	OnImGuiRender();
	ImGuiRenderEnd();
	glDisable(GL_BLEND);
}

void Application::Run(std::string loadFile)
{
	m_Window->SetVisible(true);
	float oneSecCounter = 0;
	while (isActive) {
		float currentTime = glfwGetTime();
		float deltaTime = currentTime - previousTime;
		previousTime = currentTime;
		oneSecCounter += deltaTime;
		OnUpdate(deltaTime);
		if (oneSecCounter >= 1)
		{
			OnOneSecondTick();
			oneSecCounter = 0;
		}
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