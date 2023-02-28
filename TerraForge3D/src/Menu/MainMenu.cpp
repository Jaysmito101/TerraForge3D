#include "Menu/MainMenu.h"

#include "imgui/imgui.h"

#include "Utils/Utils.h"
#include "Misc/AppStyles.h"
#include "Data/ApplicationState.h"
#include "Platform.h"

static void ShowWindowMenuItem(const char *title, bool *val)
{
	ImGui::Checkbox(title, val);
}



MainMenu::MainMenu(ApplicationState *as)
	:appState(as)
{
}

MainMenu::~MainMenu()
{
}

void MainMenu::ShowMainMenu()
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			ShowFileMenu();
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Options"))
		{
			ShowOptionsMenu();
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Windows"))
		{
			ShowWindowsMenu();
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Help"))
		{
			ShowHelpMenu();
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}
}

void MainMenu::ShowFileMenu()
{
	if (ImGui::MenuItem("Open")) appState->serailizer->LoadFile(ShowOpenFileDialog("*.terr3d"));
	if (ImGui::MenuItem("Save")) appState->serailizer->LoadFile(ShowSaveFileDialog("*.terr3d"));
	if (ImGui::MenuItem("Export ...")) appState->exportManager->SetVisible(true);
	if (ImGui::MenuItem("Close")) appState->globals.currentOpenFilePath = "";
	if (ImGui::MenuItem("Pack Project")) appState->serailizer->PackProject(ShowSaveFileDialog("*.terr3dpack"));
	if (ImGui::MenuItem("Load Packed Project")) appState->serailizer->LoadPackedProject(ShowOpenFileDialog("*.terr3dpack"));
	if (ImGui::MenuItem("Load Auto Saved Project")) appState->serailizer->LoadFile(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "cache" PATH_SEPARATOR "autosave" PATH_SEPARATOR "autosave.terr3d");
	if (ImGui::MenuItem("Exit")) exit(0);
}

void MainMenu::ShowOptionsMenu()
{
	if (ImGui::MenuItem("Toggle System Console")) ToggleSystemConsole();
	if (ImGui::MenuItem("Associate (.terr3d) File Type")) AccocFileType();
	if (ImGui::MenuItem("Copy Version Hash"))
	{
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
		std::cout << "Version Hash : " << output << std::endl;
#endif
	}

	if (ImGui::BeginMenu("Themes"))
	{
		if (ImGui::MenuItem("Default"))
		{
			appState->styleManager->LoadFromFile(appState->constants.stylesDir + PATH_SEPARATOR "Default.json");
			appState->styleManager->Apply();
		}

		if (ImGui::MenuItem("Load Theme From File"))
		{
			std::string path = ShowOpenFileDialog("*.json");
			if (path.size() > 3)
			{
				appState->styleManager->LoadFromFile(path);
				appState->styleManager->Apply();
			}
		}
		ImGui::EndMenu();
	}
}

void MainMenu::ShowWindowsMenu()
{
	if (ImGui::BeginMenu("Viewports"))
	{
		static bool s_IsViewportOpen = false;
		for (auto& vp : appState->viewportManagers)
		{
			s_IsViewportOpen = vp->IsVisible();
			ShowWindowMenuItem(("Viewport " + std::to_string(vp->GetID())).data(), &s_IsViewportOpen);
			vp->SetVisible(s_IsViewportOpen);
		}
		ImGui::EndMenu();
	}
	ShowWindowMenuItem("Dashboard", appState->dashboard->IsWindowVisiblePtr());
	ShowWindowMenuItem("Renderer Settings", appState->rendererManager->IsWindowVisiblePtr());
	ShowWindowMenuItem("Export Manager", appState->exportManager->IsWindowOpenPtr());
	ShowWindowMenuItem("Work Manager", appState->workManager->IsWindowVisiblePtr());
	ShowWindowMenuItem("Theme Editor", &appState->windows.styleEditor);
	ShowWindowMenuItem("Texture Store", &appState->windows.textureStore);
	ShowWindowMenuItem("Mesh Generators Settings", &appState->meshGenerator->windowStat);
	ShowWindowMenuItem("Supporters", &appState->windows.supportersTribute);
	ShowWindowMenuItem("Open Source Liscenses", &appState->windows.osLisc);

}

void MainMenu::ShowHelpMenu()
{
	if (ImGui::MenuItem("Tutorial")) OpenURL("https://www.youtube.com/playlist?list=PLl3xhxX__M4A74aaTj8fvqApu7vo3cOiZ");
	if (ImGui::MenuItem("Social Handle")) OpenURL("https://twitter.com/jaysmito101");
	if (ImGui::MenuItem("Discord Server")) OpenURL("https://discord.gg/AcgRafSfyB");
	if (ImGui::MenuItem("GitHub Page")) OpenURL("https://github.com/Jaysmito101/TerraForge3D");
	if (ImGui::MenuItem("Documentation")) OpenURL("https://github.com/Jaysmito101/TerraForge3D/wiki");
	if (ImGui::MenuItem("Open Source Liscenses")) appState->windows.osLisc = !appState->windows.osLisc;
}
