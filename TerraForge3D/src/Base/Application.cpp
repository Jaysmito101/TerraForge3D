#include "Base/Application.h"
#include "Base/Logging/Logger.h"
#include "Data/VersionInfo.h"
#include "Profiler.h"

#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/imgui.h>

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>

namespace tf3d::base
{

    namespace
    {
        static void InitGlad()
        {
            if (!gladLoadGL(glfwGetProcAddress)) {
                TF3D_LOG_ERROR("Failed to initialize GLAD");
                exit(-1);
            }
        }

        static void InitImGui(std::string &configPath)
        {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO &io = ImGui::GetIO();
            (void)io;
            io.IniFilename = configPath.c_str();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#ifdef TERR3D_WIN32 // Multiviewport is not supported stable on linux
            io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
#endif
            io.ConfigViewportsNoTaskBarIcon = false;
            io.ConfigViewportsNoAutoMerge   = true;
            ImGui::StyleColorsDark();
            ImGuiStyle &style = ImGui::GetStyle();

            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                style.WindowRounding              = 0.0f;
                style.Colors[ImGuiCol_WindowBg].w = 1.0f;
            }

            ImGui_ImplGlfw_InitForOpenGL(static_cast<GLFWwindow *>(Application::Get()->GetWindow()->GetNativeWindow()), true);
            ImGui_ImplOpenGL3_Init("#version 330");
        }

        static void ImGuiShutdown()
        {
            ImGuiIO &io = ImGui::GetIO();
            if (io.IniFilename != nullptr)
                ImGui::SaveIniSettingsToDisk(io.IniFilename);
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();
        }

    } // namespace

    Application *Application::s_App;

    Application::Application()
    {
        isActive     = false;
        m_Window     = nullptr;
        previousTime = 0.0f;
    }

    void Application::SetWindowConfigPath(std::string path)
    {
        windowConfigPath = path;
    }

    void Application::SetTitle(std::string title)
    {
        m_WindowTitle = title;
    }

    void Application::Init()
    {
        m_Window = new Window(m_WindowTitle);
        m_Window->SetVSync(true);
        m_Window->SetVisible(false);
        isActive = true;
        s_App    = this;
        InitGlad();
        auto setGlMetadata = [](const char *key, GLenum name) {
            const GLubyte *value = glGetString(name);
            if (value != nullptr)
                TF3D_PROFILE_SET_METADATA(key, reinterpret_cast<const char *>(value));
        };
        setGlMetadata("gl/vendor", GL_VENDOR);
        setGlMetadata("gl/renderer", GL_RENDERER);
        setGlMetadata("gl/version", GL_VERSION);
        TF3D_PROFILE_SET_METADATA("process/name", "TerraForge3D");
        TF3D_PROFILE_SET_METADATA("build/version", TERR3D_VERSION_STRING);
        TF3D_PROFILE_SET_METADATA("trace/format", "chrome-trace-event");
        TF3D_PROFILE_SET_METADATA("window/vsync", m_Window->IsVSyncEnabled() ? "on" : "off");
#if defined(TF3D_PROFILER_ENABLED) && TF3D_PROFILER_ENABLED
#if defined(TF3D_PROFILER_GPU) && TF3D_PROFILER_GPU
        TF3D_PROFILE_SET_METADATA("profiler/compile-mode", "FULL");
#else
        TF3D_PROFILE_SET_METADATA("profiler/compile-mode", "CPU");
#endif
#else
        TF3D_PROFILE_SET_METADATA("profiler/compile-mode", "OFF");
#endif
        InitImGui(windowConfigPath);
    }

    void Application::Render()
    {
    }

    void Application::ImGuiRenderBegin()
    {
        ImGui_ImplGlfw_NewFrame();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();
    }

    void Application::ImGuiRenderEnd()
    {
        TF3D_PROFILE_SCOPE_DOMAIN("imgui/draw", PerformanceMonitor::Domain::Ui);
        ImGui::EndFrame();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        ImGuiIO &io = ImGui::GetIO();

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow *backup_current_context = glfwGetCurrentContext();
            {
                TF3D_PROFILE_SCOPE_DOMAIN("imgui/platform-windows/update", PerformanceMonitor::Domain::Ui);
                ImGui::UpdatePlatformWindows();
            }
            {
                TF3D_PROFILE_SCOPE_DOMAIN("imgui/platform-windows/render-and-swap", PerformanceMonitor::Domain::Ui);
                ImGui::RenderPlatformWindowsDefault();
            }
            glfwMakeContextCurrent(backup_current_context);
        }
    }

    bool Application::IsActive()
    {
        return isActive;
    }

    void Application::RenderImGui()
    {
        glEnable(GL_BLEND);
        {
            TF3D_PROFILE_SCOPE_DOMAIN("imgui/build", PerformanceMonitor::Domain::Ui);
            ImGuiRenderBegin();
            OnImGuiRender();
        }
        ImGuiRenderEnd();
        glDisable(GL_BLEND);
    }

    void Application::Run(std::string loadFile)
    {
        m_Window->SetVisible(true);
        TF3D_PROFILE_THREAD_NAME("Main Thread");
        float oneSecCounter = 0;

        while (isActive) {
            TF3D_PROFILE_FRAME_BEGIN();
            TF3D_PROFILE_BEGIN(frameProfile, "app/frame");
            float currentTime = (float)glfwGetTime();
            float deltaTime   = currentTime - previousTime;
            previousTime      = currentTime;
            oneSecCounter += deltaTime;
            OnUpdate(deltaTime);

            if (oneSecCounter >= 1) {
                TF3D_PROFILE_SCOPE_DOMAIN("app/one-second-tick", PerformanceMonitor::Domain::Cpu);
                OnOneSecondTick();
                oneSecCounter = 0;
            }

            {
                TF3D_PROFILE_SCOPE_DOMAIN("app/render", PerformanceMonitor::Domain::Renderer);
                Render();
            }
            {
                TF3D_PROFILE_SCOPE_DOMAIN("app/present", PerformanceMonitor::Domain::Present);
                m_Window->Update();
            }
            frameProfile.End();
            TF3D_PROFILE_FRAME_END();
        }

        OnEnd();
        ImGuiShutdown();
    }

    Application::~Application()
    {
        TF3D_LOG_INFO("Application shutdown complete");
        delete m_Window;
    }
} // namespace tf3d::base
