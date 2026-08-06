#include <glad/gl.h>
#include "Base/Base.h"
#include "Data/ApplicationState.h"
#include "Data/VersionInfo.h"
#include "Exporters/ExportManager.h"
#include "Misc/AppStyles.h"
#include "Misc/OSLiscences.h"
#include "Misc/SupportersTribute.h"
#include "Platform.h"
#include "Profiler.h"
#include "TextureStore/TextureStore.h"
#include "Utils/Utils.h"
#include "Base/SplashScreen.h"
#include <iostream>
#include <string>

#undef cNear
#undef cFar
#include <nlohmann/json.hpp>
#include <sys/stat.h>

#include "Misc/CustomInspector.h"

#ifdef TF3D_ENABLE_MCP
#include "MCP/TerraForgeMcpServer.h"
#include "UI/McpControlPanel.h"
#endif

namespace tf3d
{

    class TerraForge3D : public base::Application
    {
    public:
        virtual void OnPreload() override
        {
            PerformanceMonitor::Get().SetCurrentThreadName("Main Thread");
            SetTitle("TerraForge3D - Jaysmito Mukherjee");
            MkDir(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "configs");
            SetWindowConfigPath(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "configs" PATH_SEPARATOR "windowconfigs.terr3d");
            MkDir(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "cache" PATH_SEPARATOR "autosave");
            MkDir(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "temp");
        }

        virtual void OnUpdate(float deltatime) override
        {
            TF3D_PROFILE_SCOPE("app/update");
#ifdef TF3D_ENABLE_MCP
            if (appState->mcpServer) {
                TF3D_PROFILE_SCOPE("app/update/mcp");
                appState->mcpServer->Update();
            }
#endif
            if (!appState->states.ruinning)
                return;
            {
                TF3D_PROFILE_SCOPE("app/update/jobs");
                {
                    TF3D_PROFILE_SCOPE("app/update/jobsystem");
                    appState->jobSystem->Update();
                }
                {
                    TF3D_PROFILE_SCOPE("app/update/dashboard");
                    appState->dashboard->Update();
                }
                {
                    TF3D_PROFILE_SCOPE("app/update/export");
                    appState->exportManager->Update();
                }
                {
                    TF3D_PROFILE_SCOPE("app/update/generation");
                    appState->generationManager->Update();
                }
            }

            {
                TF3D_PROFILE_SCOPE("app/update/renderer");
                appState->rendererManager->Update();
            }

            {
                TF3D_PROFILE_SCOPE("app/update/viewports");
                for (int i = 0; i < MAX_VIEWPORT_COUNT; i++) {
                    appState->viewportManagers[i]->Update();
                }
            }

            {
                TF3D_PROFILE_SCOPE("app/update/misc");
                // NOTE: This is a temporary hack to fix the brush not working on all viewports
                appState->rendererManager->GetObjectRenderer()->SetCustomBaseShapeDrawSettings(nullptr);

                // CTRL Shortcuts
                if ((glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_LEFT_CONTROL) || glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_RIGHT_CONTROL))) {
                    // Open Shortcut
                    // if (glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_O)) appState->serailizer->LoadFile(ShowOpenFileDialog("*.terr3d"));

                    // Exit Shortcut
                    if (glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_Q))
                        Close();

                    // Save Shortcut
                    if (glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_S)) {
                        if (appState->globals.currentOpenFilePath.size() > 3) {
                            TF3D_LOG_INFO("Saved project: '{}'", appState->globals.currentOpenFilePath);
                            // appState->serailizer->SaveFile(appState->globals.currentOpenFilePath);
                        }
                        // else appState->serailizer->SaveFile(ShowSaveFileDialog("*.terr3d"));
                    }

                    // Close Shortcut
                    if (glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_W)) {
                        if (appState->globals.currentOpenFilePath.size() > 3) {
                            TF3D_LOG_INFO("Closed project: '{}'", appState->globals.currentOpenFilePath);
                            appState->globals.currentOpenFilePath = "";
                        }

                        else {
                            TF3D_LOG_INFO("Shutdown requested");
                            Close();
                        }
                    }

                    // CTRL + SHIFT Shortcuts
                    if ((glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_LEFT_SHIFT) || glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_RIGHT_SHIFT))) // Save Shortcut
                    {
                        // Save As Shortcuts
                        if (glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_S)) {
                            appState->globals.currentOpenFilePath = "";
                            // appState->serailizer->SaveFile(ShowSaveFileDialog("*.terr3d"));
                        }
                    }
                }

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }
            {
                TF3D_PROFILE_SCOPE("app/ui");
                RenderImGui();
            }
        }

        virtual void OnOneSecondTick() override
        {
            if (!appState->states.ruinning)
                return;
            appState->globals.secondCounter++;
            if (appState->globals.secondCounter % 5 == 0) {
                if (appState->states.autoSave) {
                    // appState->serailizer->SaveFile(appState->constants.cacheDir + PATH_SEPARATOR "autosave" PATH_SEPARATOR "autosave.terr3d");
                    // if (appState->globals.currentOpenFilePath.size() > 3) appState->serailizer->SaveFile(appState->globals.currentOpenFilePath);
                }
            }
        }

        virtual void OnImGuiRender() override
        {
            OnBeforeImGuiRender();
            appState->mainMenu->ShowMainMenu();
            appState->dashboard->ShowSettings();
            appState->generationManager->ShowSettings();
            for (int i = 0; i < MAX_VIEWPORT_COUNT; i++)
                appState->viewportManagers[i]->Show();
            appState->exportManager->ShowSettings();
            appState->jobManager->ShowSettings();
            appState->rendererManager->ShowSettings();
            PerformanceMonitor::Get().RenderUI();
#ifdef TF3D_ENABLE_MCP
            if (appState->mcpControlPanel)
                appState->mcpControlPanel->ShowSettings();
#endif
            if (appState->windows.styleEditor)
                ShowStyleEditor(&appState->windows.styleEditor);
            if (appState->windows.textureStore)
                appState->textureStore->ShowSettings(&appState->windows.textureStore);
            if (appState->windows.osLisc)
                appState->osLiscences->ShowSettings(&appState->windows.osLisc);
            if (appState->windows.supportersTribute)
                appState->supportersTribute->ShowSettings(&appState->windows.supportersTribute);

            if (appState->configManager) {
                Style currentTheme;
                currentTheme.LoadCurrent();
                currentTheme.SetName(GetCurrentThemeName());
                appState->configManager->SaveLastUsedThemeIfChanged(
                    GetCurrentThemeName(), currentTheme.SaveToString());
            }

            OnImGuiRenderEnd();
        }

        virtual void OnStart(std::string loadFile) override
        {
            srand((uint32_t)time(NULL));
            utils::SetUpIcon();
            appState                          = new ApplicationState();
            appState->mainApp                 = this;
            appState->constants.executableDir = GetExecutableDir();
            appState->constants.dataDir       = appState->constants.executableDir + PATH_SEPARATOR "Data";
            appState->constants.cacheDir      = appState->constants.dataDir + PATH_SEPARATOR "cache";
            appState->constants.texturesDir   = appState->constants.dataDir + PATH_SEPARATOR "textures";
            appState->constants.projectsDir   = appState->constants.cacheDir + PATH_SEPARATOR "project_data";
            appState->constants.tempDir       = appState->constants.dataDir + PATH_SEPARATOR "temp";
            appState->constants.shadersDir    = appState->constants.dataDir + PATH_SEPARATOR "shaders";
            appState->constants.fontsDir      = appState->constants.dataDir + PATH_SEPARATOR "fonts";
            appState->constants.liscensesDir  = appState->constants.dataDir + PATH_SEPARATOR "licenses";
            appState->constants.configsDir    = appState->constants.dataDir + PATH_SEPARATOR "configs";
            appState->constants.logsDir       = appState->constants.dataDir + PATH_SEPARATOR "logs";
            appState->constants.modelsDir     = appState->constants.dataDir + PATH_SEPARATOR "models";
            appState->constants.stylesDir     = appState->constants.dataDir + PATH_SEPARATOR "styles";
            appState->configManager           = new ConfigManager();
#ifdef TF3D_ENABLE_MCP
            appState->mcpEnabled = true;
            appState->configManager->GetBool("mcp", "enabled", appState->mcpEnabled);
#endif

            ImGui::GetStyle().WindowMenuButtonPosition = ImGuiDir_None;

            // LoadDefaultStyle();

            GetWindow()->SetShouldCloseCallback([&](int x, int y) -> void { TF3D_LOG_INFO("Window close requested ({}x{})", x, y); appState->mainApp->Close(); });
            glfwSetDropCallback(GetWindow()->GetNativeWindow(), [](GLFWwindow *, int count, const char **paths) {
                for (int i = 0; i < count; i++) {
                    std::string path = paths[i];
                    // if (path.find(".terr3d") != std::string::npos) { appState->serailizer->LoadFile(path); break; }
                    // else if (path.find(".terr3dpack") != std::string::npos) { appState->serailizer->LoadPackedProject(path); break; }
                }
            });
            GetWindow()->SetClearColor({0.1f, 0.1f, 0.1f});
            appState->mainModel = new Model("Main_Model");
            appState->mainModel->mesh->GeneratePlane(1024, 1.0f);
            appState->mainModel->isGeneratedPlane = true;
            appState->mainModel->planeSolidDepth  = 0.1f;
            appState->mainModel->mesh->RecalculateNormals();
            appState->mainModel->SetupMeshOnGPU();
            appState->mainModel->UploadToGPU();

            appState->mainMap.tileCount      = 1;
            appState->mainMap.mapResolution  = 1024;
            appState->mainMap.tileResolution = 1024;
            appState->mainMap.tileSize       = 1.0f;
            appState->mainMap.tileOffsetX = appState->mainMap.tileOffsetY = 0.0f;
            appState->mainMap.currentTileX = appState->mainMap.currentTileY = 0;

            appState->jobSystem         = new JobSystem::JobSystem(appState);
            appState->jobManager        = new JobSystem::JobManager(appState);
            appState->eventManager      = new EventManager();
            appState->resourceManager   = ResourceManager::GetInstance(appState);
            appState->dashboard         = new Dashboard(appState);
            appState->generationManager = new GenerationManager(appState);
            appState->supportersTribute = new SupportersTribute();
            appState->rendererManager   = new renderer::RendererManager(appState);
            appState->mainMenu          = new MainMenu(appState);
            // appState->projectManager = new ProjectManager(appState);
            // appState->serailizer = new Serializer(appState);
            appState->osLiscences   = new OSLiscences(appState);
            appState->textureStore  = new texture_store::TextureStore(appState);
            appState->exportManager = new ExportManager(appState);
            appState->styleManager  = new Style();
            for (int i = 0; i < MAX_VIEWPORT_COUNT; i++)
                appState->viewportManagers[i] = new misc::ViewportManager(appState);

            appState->styleManager->LoadFromFile(appState->constants.stylesDir + PATH_SEPARATOR "Default.json");
            appState->styleManager->Apply();
            SetCurrentThemeName("Default");
            CaptureCurrentThemeDefaults();

            std::string lastThemeName;
            std::string lastThemeData;
            if (appState->configManager->LoadLastUsedTheme(lastThemeName, lastThemeData)) {
                Style lastTheme;
                lastTheme.LoadFormString(lastThemeData);
                lastTheme.Apply();
                SetCurrentThemeName(lastThemeName);
                CaptureCurrentThemeDefaults();
                TF3D_LOG_INFO("Loaded last used theme: '{}'", lastThemeName);
            }

            if (loadFile.size() > 0) {
                TF3D_LOG_INFO("Loading project: '{}'", loadFile);
                // appState->serailizer->LoadFile(loadFile);
            }
            // appState->projectManager->SetId(GenerateId(32));
            // Load Fonts
            LoadUIFont("Open-Sans-Regular", 18, appState->constants.fontsDir + PATH_SEPARATOR "OpenSans-Regular.ttf");
            LoadUIFont("OpenSans-Bold", 25, appState->constants.fontsDir + PATH_SEPARATOR "OpenSans-Bold.ttf");
            LoadUIFont("OpenSans-Semi-Bold", 22, appState->constants.fontsDir + PATH_SEPARATOR "OpenSans-Bold.ttf");
            TF3D_LOG_INFO("Application started");
            appState->eventManager->RaiseEvent("TileResolutionChanged", "256");
            appState->eventManager->RaiseEvent("OnStartUpComplete");

#ifdef TF3D_ENABLE_MCP
            appState->mcpServer = new mcp_layer::TerraForgeMcpServer(appState, appState->constants.logsDir);
            if (appState->mcpEnabled && !appState->mcpServer->Start()) {
                TF3D_LOG_ERROR("Failed to start the TerraForge3D MCP server");
            } else if (!appState->mcpEnabled) {
                TF3D_LOG_INFO("TerraForge3D MCP server is disabled in the user config");
            }
            appState->mcpControlPanel = new ui::McpControlPanel(appState);
#endif
        }

        void OnEnd() override
        {
#ifdef TF3D_ENABLE_MCP
            if (appState->mcpServer) {
                appState->mcpServer->Stop();
                delete appState->mcpServer;
                appState->mcpServer = nullptr;
            }
            delete appState->mcpControlPanel;
            appState->mcpControlPanel = nullptr;
#endif
            appState->eventManager->RaiseEvent("OnEnd");
            for (int i = 0; i < MAX_VIEWPORT_COUNT; i++)
                delete appState->viewportManagers[i];
            appState->jobSystem->WaitAll();
            delete appState->jobManager;
            delete appState->jobSystem;
            delete appState->eventManager;
            delete appState->textureStore;
            delete appState->styleManager;
            delete appState->generationManager;
            delete appState->dashboard;
            delete appState->rendererManager;
            delete appState->mainModel;
            delete appState->supportersTribute;
            delete appState->mainMenu;
            delete appState->osLiscences;
            delete appState->exportManager;
            delete appState->resourceManager;
            delete appState->configManager;
            // delete appState->serailizer;
            delete appState;
        }

    private:
        data::ApplicationState *appState;
    };

} // namespace tf3d

int main(int argc, char **argv)
{
    const std::string logsDir = tf3d::utils::GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "logs";
    tf3d::utils::MkDir(logsDir);
    tf3d::base::Logger logger(logsDir);

    auto app = std::make_unique<tf3d::TerraForge3D>();
    app->OnPreload();
    app->Init();

    {
        std::string args = "";

        if (argc == 2) {
            args = std::string(argv[1]);
        }

        tf3d::base::SplashScreen::Init();
        app->OnStart(args);
        tf3d::base::SplashScreen::Destory();
    }
    app->Run();
}
