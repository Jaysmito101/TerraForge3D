#include "resource.h"
#include <glad/glad.h>
#ifdef TERR3D_WIN32
#define GLFW_EXPOSE_NATIVE_WIN32 // For Windows
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#endif
#include "Base/Base.h"
#include "Base/EntryPoint.h"
#include "Data/ApplicationState.h"
#include "Data/VersionInfo.h"
#include "TextureStore/TextureStore.h"
#include "Exporters/ExportManager.h"
#include "Misc/OSLiscences.h"
#include "Misc/AppStyles.h"
#include "Misc/SupportersTribute.h"
#include "Platform.h"
#include "Utils/Utils.h"
#undef cNear
#undef cFar
#include "json.hpp"
#include <sys/stat.h>

#include "Misc/CustomInspector.h"

static ApplicationState* appState;
static Application* mainApp;


void SetUpIcon()
{
#ifdef TERR3D_WIN32
	HWND hwnd = glfwGetWin32Window(mainApp->GetWindow()->GetNativeWindow());
	HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
	if (hIcon)
	{
		//Change both icons to the same icon handle.
		SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
		//This will ensure that the application icon gets changed too.
		SendMessage(GetWindow(hwnd, GW_OWNER), WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		SendMessage(GetWindow(hwnd, GW_OWNER), WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	}
#endif
}


class MyApp : public Application
{
	CustomInspector inspector;

public:
	virtual void OnPreload() override
	{
		SetTitle("TerraForge3D - Jaysmito Mukherjee");
		MkDir(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "logs");
		SetLogsDir(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "logs");
		SetWindowConfigPath(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "configs" PATH_SEPARATOR "windowconfigs.terr3d");
		MkDir(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "cache" PATH_SEPARATOR "autosave\"");
		MkDir(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "temp\"");
	}

	virtual void OnUpdate(float deltatime) override
	{
		if (!appState->states.ruinning) return;
		appState->dashboard->Update();
		appState->exportManager->Update(); 
		appState->generationManager->Update();
		for (int i = 0; i < MAX_VIEWPORT_COUNT; i++) appState->viewportManagers[i]->Update();


		// CTRL Shortcuts
		if ((glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_LEFT_CONTROL) || glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_RIGHT_CONTROL)))
		{
			// Open Shortcut
			// if (glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_O)) appState->serailizer->LoadFile(ShowOpenFileDialog("*.terr3d"));

			// Exit Shortcut
			if (glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_Q)) exit(0);

			// Save Shortcut
			if (glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_S))
			{
				if (appState->globals.currentOpenFilePath.size() > 3)
				{
					Log("Saved to " + appState->globals.currentOpenFilePath);
					// appState->serailizer->SaveFile(appState->globals.currentOpenFilePath);
				}
				// else appState->serailizer->SaveFile(ShowSaveFileDialog("*.terr3d"));
			}

			// Close Shortcut
			if (glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_W))
			{
				if (appState->globals.currentOpenFilePath.size() > 3)
				{
					Log("CLosed file " + appState->globals.currentOpenFilePath);
					appState->globals.currentOpenFilePath = "";
				}

				else
				{
					Log("Shutting Down");
					exit(0);
				}
			}

			// CTRL + SHIFT Shortcuts
			if ((glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_LEFT_SHIFT) || glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_RIGHT_SHIFT)))  // Save Shortcut
			{
				// Save As Shortcuts
				if (glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_S))
				{
					appState->globals.currentOpenFilePath = "";
					// appState->serailizer->SaveFile(ShowSaveFileDialog("*.terr3d"));
				}
			}
		}


		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		RenderImGui();
	}

	virtual void OnOneSecondTick() override
	{
		if (!appState->states.ruinning) return;
		appState->globals.secondCounter++;
		if (appState->globals.secondCounter % 5 == 0)
		{
			if (appState->states.autoSave)
			{
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
		for (int i = 0; i < MAX_VIEWPORT_COUNT; i++) appState->viewportManagers[i]->Show();
		appState->exportManager->ShowSettings();
		appState->rendererManager->ShowSettings();
		if (appState->windows.styleEditor) ShowStyleEditor(&appState->windows.styleEditor);
		if (appState->windows.textureStore) appState->textureStore->ShowSettings(&appState->windows.textureStore);
		if (appState->windows.osLisc) appState->osLiscences->ShowSettings(&appState->windows.osLisc);
		if (appState->windows.supportersTribute) appState->supportersTribute->ShowSettings(&appState->windows.supportersTribute);

		ImGui::Begin("CustomInspectorTest");
		inspector.Render();
		ImGui::End();

		OnImGuiRenderEnd();
	}

	virtual void OnStart(std::string loadFile) override
	{
		srand((uint32_t)time(NULL));
		SetUpIcon();
		appState = new ApplicationState();
		appState->mainApp = mainApp;
		appState->constants.executableDir = GetExecutableDir(); 
		appState->constants.dataDir = appState->constants.executableDir + PATH_SEPARATOR "Data";
		appState->constants.cacheDir = appState->constants.dataDir + PATH_SEPARATOR "cache";
		appState->constants.texturesDir = appState->constants.dataDir + PATH_SEPARATOR "textures";
		appState->constants.projectsDir = appState->constants.cacheDir + PATH_SEPARATOR "project_data";
		appState->constants.tempDir = appState->constants.dataDir + PATH_SEPARATOR "temp";
		appState->constants.shadersDir = appState->constants.dataDir + PATH_SEPARATOR "shaders";
		appState->constants.fontsDir = appState->constants.dataDir + PATH_SEPARATOR "fonts";
		appState->constants.liscensesDir = appState->constants.dataDir + PATH_SEPARATOR "licenses";
		appState->constants.configsDir = appState->constants.dataDir + PATH_SEPARATOR "configs";
		appState->constants.logsDir = appState->constants.dataDir + PATH_SEPARATOR "logs";
		appState->constants.modelsDir = appState->constants.dataDir + PATH_SEPARATOR "models";
		appState->constants.stylesDir = appState->constants.dataDir + PATH_SEPARATOR "styles";



		inspector.AddStringVariable("Name", "Basic");
		inspector.AddBoolVariable("SquareValue");
		inspector.AddBoolVariable("AbsoluteValue");
		inspector.AddIntegerVariable("SubStyle");
		inspector.AddFloatVariable("Height");
		inspector.AddDropdownWidget("Sub Style", "SubStyle", { "Flat", "Dome", "Slope", "Sine Wave" });
		inspector.AddSliderWidget("Height", "Height");
		inspector.AddSeperatorWidget();
		inspector.AddDragWidget("Height_2", "Height", 0.0f, 100.0f, 5.0f);
		inspector.SetWidgetTooltip("Sub Style", "The style of basic generation to use");
		inspector.SetWidgetTooltip("Height_2", "Maximum height of terrain");
		inspector.AddSeperatorWidget();
		inspector.AddNewLineWidget();


		ImGui::GetStyle().WindowMenuButtonPosition = ImGuiDir_None;
		
		// LoadDefaultStyle();


		GetWindow()->SetShouldCloseCallback([&](int x, int y) -> void { Log("Shutting Down"); appState->mainApp->Close(); });
		glfwSetDropCallback(GetWindow()->GetNativeWindow(), [](GLFWwindow*, int count, const char** paths) {
				for (int i = 0; i < count; i++)
				{
					std::string path = paths[i];
					// if (path.find(".terr3d") != std::string::npos) { appState->serailizer->LoadFile(path); break; }
					// else if (path.find(".terr3dpack") != std::string::npos) { appState->serailizer->LoadPackedProject(path); break; }
				}
		});
		GetWindow()->SetClearColor({ 0.1f, 0.1f, 0.1f });
		appState->mainModel = new Model("Main_Model");
		appState->mainModel->mesh->GeneratePlane(256, 1.0f);
		appState->mainModel->mesh->RecalculateNormals();
		appState->mainModel->SetupMeshOnGPU();
		appState->mainModel->UploadToGPU();

		appState->mainMap.tileCount = 1;
		appState->mainMap.mapResolution = 256;
		appState->mainMap.tileResolution = 256;
		appState->mainMap.tileSize = 1.0f;
		appState->mainMap.tileOffsetX = appState->mainMap.tileOffsetY = 0.0f;
		appState->mainMap.currentTileX = appState->mainMap.currentTileY = 0;
		
		appState->eventManager = new EventManager();
		appState->dashboard = new Dashboard(appState);
		appState->generationManager = new GenerationManager(appState);
		appState->supportersTribute = new SupportersTribute();
		appState->rendererManager = new RendererManager(appState);
		appState->mainMenu = new MainMenu(appState);
		// appState->projectManager = new ProjectManager(appState);
		// appState->serailizer = new Serializer(appState);
		appState->osLiscences = new OSLiscences(appState);
		appState->textureStore = new TextureStore(appState);
		appState->exportManager = new ExportManager(appState);
		appState->styleManager = new Style();
		for (int i = 0; i < MAX_VIEWPORT_COUNT; i++) appState->viewportManagers[i] = new ViewportManager(appState);

		
		appState->styleManager->LoadFromFile(appState->constants.stylesDir + PATH_SEPARATOR "Default.json");
		appState->styleManager->Apply();


		if (loadFile.size() > 0)
		{
			Log("Loading File from " + loadFile);
			// appState->serailizer->LoadFile(loadFile);
		}
		// appState->projectManager->SetId(GenerateId(32));
		float t = 1.0f;
		// Load Fonts
		LoadUIFont("Open-Sans-Regular", 18, appState->constants.fontsDir + PATH_SEPARATOR "OpenSans-Regular.ttf");
		LoadUIFont("OpenSans-Bold", 25, appState->constants.fontsDir + PATH_SEPARATOR "OpenSans-Bold.ttf");
		LoadUIFont("OpenSans-Semi-Bold", 22, appState->constants.fontsDir + PATH_SEPARATOR "OpenSans-Bold.ttf");
		bool tpp = false;
		if (IsNetWorkConnected())
		{
			if (FileExists(appState->constants.configsDir + PATH_SEPARATOR "server.terr3d"))
			{
				bool ttmp = false;
				std::string uid = ReadShaderSourceFile(appState->constants.configsDir + PATH_SEPARATOR "server.terr3d", &ttmp);
				appState->eventManager->RaiseEvent("ConnectedToBackend");
				Log("Connection to Backend : " + FetchURL("https://terraforge3d.maxalpha.repl.co", "/startup/" + uid));
			}
			else
			{
				DownloadFile("https://terraforge3d.maxalpha.repl.co", "/register", appState->constants.configsDir + PATH_SEPARATOR "server.terr3d");
				bool ttmp = false;
				std::string uid = ReadShaderSourceFile(appState->constants.configsDir + PATH_SEPARATOR "server.terr3d", &ttmp);
				Log("Connection to Backend : " + FetchURL("https://terraforge3d.maxalpha.repl.co", "/startup/" + uid));
			}
		}

		Log("Started Up App!");
		appState->eventManager->RaiseEvent("TileResolutionChanged", "256");
		appState->eventManager->RaiseEvent("OnStartUpComplete");


	}

	void OnEnd()
	{
		appState->eventManager->RaiseEvent("OnEnd");
		for (int i = 0; i < MAX_VIEWPORT_COUNT; i++) delete appState->viewportManagers[i];
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
		delete appState->projectManager;
		// delete appState->serailizer;
		delete appState;
	}
};

Application* CreateApplication()
{
	return (mainApp = new MyApp());
}
