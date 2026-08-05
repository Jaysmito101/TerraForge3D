#include "Menu/MainMenu.h"

#include "imgui/imgui.h"

#include "Data/ApplicationState.h"
#include "Misc/AppStyles.h"
#include "Platform.h"
#include "Profiler.h"
#include "Utils/Utils.h"

#ifdef TF3D_ENABLE_MCP
#include "UI/McpControlPanel.h"
#endif

static void ShowWindowMenuItem(const char *title, bool *val)
{
    ImGui::Checkbox(title, val);
}

MainMenu::MainMenu(ApplicationState *as)
    : appState(as)
{
}

MainMenu::~MainMenu()
{
}

void MainMenu::ShowMainMenu()
{
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ShowFileMenu();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Options")) {
            ShowOptionsMenu();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Windows")) {
            ShowWindowsMenu();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            ShowHelpMenu();
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void MainMenu::ShowFileMenu()
{
    // if (ImGui::MenuItem("Open")) appState->serailizer->LoadFile(ShowOpenFileDialog("*.terr3d"));
    // if (ImGui::MenuItem("Save")) appState->serailizer->LoadFile(ShowSaveFileDialog("*.terr3d"));
    if (ImGui::MenuItem("Export ..."))
        appState->exportManager->SetVisible(true);
    if (ImGui::MenuItem("Close"))
        appState->globals.currentOpenFilePath = "";
    // if (ImGui::MenuItem("Pack Project")) appState->serailizer->PackProject(ShowSaveFileDialog("*.terr3dpack"));
    // if (ImGui::MenuItem("Load Packed Project")) appState->serailizer->LoadPackedProject(ShowOpenFileDialog("*.terr3dpack"));
    // if (ImGui::MenuItem("Load Auto Saved Project")) appState->serailizer->LoadFile(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "cache" PATH_SEPARATOR "autosave" PATH_SEPARATOR "autosave.terr3d");
    if (ImGui::MenuItem("Exit"))
        appState->mainApp->Close();
}

void MainMenu::ShowOptionsMenu()
{
    if (ImGui::MenuItem("Toggle System Console"))
        ToggleSystemConsole();
    if (ImGui::MenuItem("Associate (.terr3d) File Type"))
        AccocFileType();
    if (ImGui::MenuItem("Copy Version Hash")) {
        char *output = new char[MD5File(GetExecutablePath()).ToString().size() + 1];
        strcpy(output, MD5File(GetExecutablePath()).ToString().c_str());
        const size_t len = strlen(output) + 1;
#ifdef TERR3D_WIN32
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
        memcpy(GlobalLock(hMem), output, len);
        GlobalUnlock(hMem);
        OpenClipboard(0);
        EmptyClipboard();
        SetClipboardData(CF_TEXT, hMem);
        CloseClipboard();
        delete[] output;
#else
        TF3D_LOG_DEBUG("Build version hash: {}", output);
#endif
    }

    if (ImGui::BeginMenu("Themes")) {
        if (ImGui::MenuItem("Default")) {
            appState->styleManager->LoadFromFile(appState->constants.stylesDir + PATH_SEPARATOR "Default.json");
            appState->styleManager->Apply();
            SetCurrentThemeName("Default");
            CaptureCurrentThemeDefaults();
        }

        if (ImGui::BeginMenu("Select")) {
            std::error_code error;
            const std::filesystem::path stylesDirectory(appState->constants.stylesDir);
            bool hasThemes = false;
            if (std::filesystem::exists(stylesDirectory, error)) {
                std::vector<std::filesystem::path> themeFiles;
                for (const auto &entry : std::filesystem::directory_iterator(stylesDirectory, error)) {
                    if (!entry.is_regular_file(error) || entry.path().extension() != ".json")
                        continue;
                    if (entry.path().stem() == "Default")
                        continue;
                    themeFiles.push_back(entry.path());
                }
                std::sort(themeFiles.begin(), themeFiles.end());

                for (const auto &themeFile : themeFiles) {
                    const std::string themeFileName = themeFile.stem().string();
                    const bool isSelected           = themeFileName == GetCurrentThemeName();
                    if (ImGui::MenuItem(themeFileName.c_str(), nullptr, isSelected)) {
                        appState->styleManager->LoadFromFile(themeFile.string());
                        appState->styleManager->Apply();
                        SetCurrentThemeName(themeFileName);
                        CaptureCurrentThemeDefaults();
                    }
                }
                hasThemes = !themeFiles.empty();
            }

            if (!hasThemes)
                ImGui::TextDisabled("No custom themes found");
            ImGui::EndMenu();
        }

        if (ImGui::MenuItem("Load Theme From File")) {
            std::string path = ShowOpenFileDialog("*.json");
            if (path.size() > 3) {
                appState->styleManager->LoadFromFile(path);
                appState->styleManager->Apply();
                SetCurrentThemeName(std::filesystem::path(path).stem().string());
                CaptureCurrentThemeDefaults();
            }
        }
        ImGui::EndMenu();
    }
}

void MainMenu::ShowWindowsMenu()
{
    if (ImGui::BeginMenu("Viewports")) {
        static bool s_IsViewportOpen = false;
        for (auto &vp : appState->viewportManagers) {
            s_IsViewportOpen = vp->IsVisible();
            ShowWindowMenuItem(("Viewport " + std::to_string(vp->GetID())).data(), &s_IsViewportOpen);
            vp->SetVisible(s_IsViewportOpen);
        }
        ImGui::EndMenu();
    }
    ShowWindowMenuItem("Dashboard", appState->dashboard->IsWindowVisiblePtr());
    ShowWindowMenuItem("Generation Manager", appState->generationManager->IsWindowVisiblePtr());
    ShowWindowMenuItem("Renderer Settings", appState->rendererManager->IsWindowVisiblePtr());
    ShowWindowMenuItem("Export Manager", appState->exportManager->IsWindowOpenPtr());
    ShowWindowMenuItem("Job Manager", appState->jobManager->IsWindowOpenPtr());
    ShowWindowMenuItem("Performance Monitor", PerformanceMonitor::Get().IsWindowOpenPtr());
    ShowWindowMenuItem("Theme Editor", &appState->windows.styleEditor);
    ShowWindowMenuItem("Texture Store", &appState->windows.textureStore);
    ShowWindowMenuItem("Supporters", &appState->windows.supportersTribute);
    ShowWindowMenuItem("Open Source Liscenses", &appState->windows.osLisc);
#ifdef TF3D_ENABLE_MCP
    if (appState->mcpControlPanel)
        ShowWindowMenuItem("MCP Server", appState->mcpControlPanel->IsWindowVisiblePtr());
#endif
}

void MainMenu::ShowHelpMenu()
{
    if (ImGui::MenuItem("Tutorial"))
        OpenURL("https://www.youtube.com/playlist?list=PLl3xhxX__M4A74aaTj8fvqApu7vo3cOiZ");
    if (ImGui::MenuItem("Social Handle"))
        OpenURL("https://twitter.com/jaysmito101");
    if (ImGui::MenuItem("Discord Server"))
        OpenURL("https://discord.gg/AcgRafSfyB");
    if (ImGui::MenuItem("GitHub Page"))
        OpenURL("https://github.com/Jaysmito101/TerraForge3D");
    if (ImGui::MenuItem("Documentation"))
        OpenURL("https://github.com/Jaysmito101/TerraForge3D/wiki");
    if (ImGui::MenuItem("Open Source Liscenses"))
        appState->windows.osLisc = !appState->windows.osLisc;
}
