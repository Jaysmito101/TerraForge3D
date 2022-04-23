#include "Base/OpenGL/UIManager.hpp"
#include "Base/Window/Window.hpp"
#include "Base/Core/Application.hpp"

#include "GLFW/glfw3.h"
#include "imgui/imgui.h"

#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "imgui/backends/imgui_impl_opengl3.cpp"


namespace TerraForge3D
{

	namespace OpenGL
	{
		UIManager::UIManager()
		{
			window = Window::Get();
			SetupImGui();
		}

		UIManager::~UIManager()
		{
			ImGui_ImplOpenGL3_Shutdown();
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();
		}

		void UIManager::SetupImGui()
		{
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO();
			(void)io;
			io.IniFilename = Application::Get()->windowConfigPath.c_str();
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
			// io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Remove this later
			io.ConfigViewportsNoTaskBarIcon = false;
			io.ConfigViewportsNoAutoMerge = true;
			ImGui::StyleColorsDark();
			ImGuiStyle& style = ImGui::GetStyle();

			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				style.WindowRounding = 0.0f;
				style.Colors[ImGuiCol_WindowBg].w = 1.0f;
			}

			// TODO : Load Fonts

			glfwSwapInterval(1); // VSync is alwasy enabled 
			ImGui_ImplGlfw_InitForOpenGL(window->GetHandle(), true);
			ImGui_ImplOpenGL3_Init("#version 460");
		}

		void UIManager::Begin()
		{
			int w, h;
			glEnable(GL_BLEND);
			glBindFramebuffer(GL_FRAMEBUFFER, 0); // Bind the default window framebuffer
			glfwGetFramebufferSize(window->GetHandle(), &w, &h);
			glViewport(0, 0, w, h);
			glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
			glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
			ImGuiIO& io = ImGui::GetIO();
			ImGui_ImplGlfw_NewFrame();
			ImGui_ImplOpenGL3_NewFrame();
			ImGui::NewFrame();
		}

		void UIManager::End()
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
			glfwSwapBuffers(window->GetHandle());
		}

	}

}