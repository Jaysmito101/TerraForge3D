#include "MainMenu.h"

#include "imgui/imgui.h"

#include "Utils.h"
#include "AppStyles.h"
#include "ExportManager.h"
#include "Data/ApplicationState.h"

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
	if (ImGui::MenuItem("Open"))
	{
		appState->serailizer->LoadFile(ShowOpenFileDialog("*.terr3d"));
	}

	if (ImGui::MenuItem("Save"))
	{
		appState->serailizer->LoadFile(ShowSaveFileDialog("*.terr3d"));
	}

	if (ImGui::MenuItem("Install Module"))
	{
		appState->modules.manager->InstallModule(ShowOpenFileDialog("*.terr3dmodule"));
	}

	Model *modelToExport;

	switch (appState->mode)
	{
		case ApplicationMode::TERRAIN:
			modelToExport = appState->models.coreTerrain;
			break;

		case ApplicationMode::CUSTOM_BASE:
			modelToExport = appState->models.customBase;
			break;

		default:
			modelToExport = appState->models.coreTerrain;
			break;
	}

	if (ImGui::BeginMenu("Export Mesh As"))
	{
		if (ImGui::MenuItem("Wavefont OBJ"))
		{
			ExportModelAssimp(modelToExport, "obj", ShowSaveFileDialog(".obj\0"), "obj");
		}

		if (ImGui::MenuItem("FBX"))
		{
			ExportModelAssimp(modelToExport, "fbx", ShowSaveFileDialog(".fbx\0"), "fbx");
		}

		if (ImGui::MenuItem("GLTF v2"))
		{
			ExportModelAssimp(modelToExport, "gltf2", ShowSaveFileDialog(".gltf\0"), "gltf");
		}

		if (ImGui::MenuItem("GLB v2"))
		{
			ExportModelAssimp(modelToExport, "glb2", ShowSaveFileDialog(".glb\0"), "glb");
		}

		if (ImGui::MenuItem("JSON"))
		{
			ExportModelAssimp(modelToExport, "json", ShowSaveFileDialog(".json\0"));
		}

		if (ImGui::MenuItem("STL"))
		{
			ExportModelAssimp(modelToExport, "stl", ShowSaveFileDialog(".stl\0"));
		}

		if (ImGui::MenuItem("PLY"))
		{
			ExportModelAssimp(modelToExport, "ply", ShowSaveFileDialog(".ply\0"));
		}

		if (ImGui::MenuItem("Collada"))
		{
			ExportModelAssimp(modelToExport, "collada", ShowSaveFileDialog(".dae\0"), "dae");
		}

		ImGui::EndMenu();
	}

	if (ImGui::MenuItem("Close"))
	{
		appState->globals.currentOpenFilePath = "";
	}

	if (ImGui::MenuItem("Pack Project"))
	{
		appState->serailizer->PackProject(ShowSaveFileDialog("*.terr3dpack"));
	}

	if (ImGui::MenuItem("Load Packed Project"))
	{
		appState->serailizer->LoadPackedProject(ShowOpenFileDialog("*.terr3dpack"));
	}

	if (ImGui::MenuItem("Load Auto Saved Project"))
	{
		appState->serailizer->LoadFile(GetExecutableDir() + "\\Data\\cache\\autosave\\autosave.terr3d");
	}

	if (ImGui::MenuItem("Exit"))
	{
		exit(0);
	}
}

void MainMenu::ShowOptionsMenu()
{
	if (ImGui::MenuItem("Toggle System Console"))
	{
		ToggleSystemConsole();
	}

	if (ImGui::MenuItem("Associate (.terr3d) File Type"))
	{
		AccocFileType();
	}

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
			LoadDefaultStyle();
		}

		if(ImGui::MenuItem("Maya Theme"))
		{
			LoadMayaStyle();
		}

		if (ImGui::MenuItem("Black & White"))
		{
			LoadBlackAndWhite();
		}

		if (ImGui::MenuItem("Cool Dark"))
		{
			LoadDarkCoolStyle();
		}

		if (ImGui::MenuItem("Light Orange"))
		{
			LoadLightOrngeStyle();
		}

		if (ImGui::MenuItem("Load Theme From File"))
		{
			LoadThemeFromFile(openfilename());
		}

		ImGui::EndMenu();
	}
}

void MainMenu::ShowWindowsMenu()
{
	ShowWindowMenuItem("Statistics", &appState->windows.statsWindow);
	ShowWindowMenuItem("Theme Editor", &appState->windows.styleEditor);
//	ShowWindowMenuItem("Shader Editor", &appState->windows.shaderEditorWindow);
	ShowWindowMenuItem("Foliage Manager", &appState->windows.foliageManager);
//	ShowWindowMenuItem("Texture Settings", &appState->windows.texturEditorWindow);
	ShowWindowMenuItem("Texture Store", &appState->windows.textureStore);
	ShowWindowMenuItem("Sea Settings", &appState->windows.seaEditor);
	ShowWindowMenuItem("Light Settings", &appState->windows.lightControls);
	ShowWindowMenuItem("Camera Settings", &appState->windows.cameraControls);
	ShowWindowMenuItem("Mesh Generators Settings", &appState->meshGenerator->windowStat);
	ShowWindowMenuItem("Sky Settings", &appState->windows.skySettings);
	ShowWindowMenuItem("Filters Manager", &appState->windows.filtersManager);
	ShowWindowMenuItem("Module Manager", &appState->windows.modulesManager);
	ShowWindowMenuItem("Supporters", &appState->windows.supportersTribute);
	ShowWindowMenuItem("Open Source Liscenses", &appState->windows.osLisc);
}

void MainMenu::ShowHelpMenu()
{
	if (ImGui::MenuItem("Tutorial"))
	{
		OpenURL("https://www.youtube.com/playlist?list=PLl3xhxX__M4A74aaTj8fvqApu7vo3cOiZ");
	}

	if (ImGui::MenuItem("Social Handle"))
	{
		OpenURL("https://twitter.com/jaysmito101");
	}

	if (ImGui::MenuItem("Discord Server"))
	{
		OpenURL("https://discord.gg/AcgRafSfyB");
	}

	if (ImGui::MenuItem("GitHub Page"))
	{
		OpenURL("https://github.com/Jaysmito101/TerraForge3D");
	}

	if (ImGui::MenuItem("Documentation"))
	{
		OpenURL("https://github.com/Jaysmito101/TerraForge3D/wiki");
	}

	if (ImGui::MenuItem("Open Source Liscenses"))
	{
		appState->windows.osLisc = !appState->windows.osLisc;
	}
}