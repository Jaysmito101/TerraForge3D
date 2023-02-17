#include "resource.h"

#include "Platform.h"



// TerraForge3D Base

#include <glad/glad.h>

#ifdef TERR3D_WIN32
#define GLFW_EXPOSE_NATIVE_WIN32 // For Windows
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#endif

#include "Base/Base.h"
#include "Base/EntryPoint.h"

// TerraForge3D Application
#include "Data/ProjectData.h"
#include "Data/ApplicationState.h"
#include "Data/VersionInfo.h"
#include "Generators/MeshGeneratorManager.h"
#include "TextureStore/TextureStore.h"
#include "Exporters/ExportManager.h"
#include "Misc/OSLiscences.h"
#include "Misc/AppStyles.h"
#include "Misc/SupportersTribute.h"

#include "Platform.h"
#include "Utils/Utils.h"

#undef cNear
#undef cFar

#include "json/json.hpp"
#include <zip.h>
#include <sys/stat.h>

#ifdef TERR3D_WIN32
#include <dirent/dirent.h>
#else
#include <dirent.h>
#endif





#include "NoiseLayers/LayeredNoiseManager.h"

static ApplicationState* appState;
static Application* mainApp;


void OnAppClose(int x, int y)
{
	Log("Shutting Down");
	appState->mainApp->Close();
}


static void ShowGeneralControls()
{
	ImGui::Begin("General Controls");
	{
		ImGui::Checkbox("VSync ", &appState->states.vSync);
	}
	ImGui::DragFloat("Mouse Speed", &appState->globals.mouseSpeed);
	ImGui::DragFloat("Zoom Speed", &appState->globals.scrollSpeed);

	if (ImGui::Button("Exit"))
	{
		exit(0);
	}

	ImGui::End();
}

static void ShowChooseBaseModelPopup()
{
	if (ImGui::BeginPopup("Choose Base Model##Main"))
	{
		if (ImGui::Button("Open From File"))
		{
			std::string path = ShowOpenFileDialog("*.*");
			if (FileExists(path))
			{
				Model* tmp = LoadModel(path);
				if (tmp) { delete appState->mainModel; appState->mainModel = tmp; tmp->mesh->RecalculateNormals(); tmp->SetupMeshOnGPU(); tmp->UploadToGPU(); }
			}
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Close")) ImGui::CloseCurrentPopup();

		static const char* default_base_models[] = { "Plane", "Sphere", "Torus" };
		static int selected_item = 0;
		if (ImGui::BeginCombo("Gnerate Base Model##Choose Base Model Main", default_base_models[selected_item]))
		{
			for (int i = 0; i < 3; i++)
			{
				bool is_selected = (selected_item == i);
				if (ImGui::Selectable(default_base_models[i], is_selected)) selected_item = i;
				if (is_selected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		if (selected_item == 0)
		{
			// plane
			static int resolution = 256;
			static float scale = 1.0f;
			ImGui::DragFloat("Scale##GenBasePlane", &scale, 0.01f, 0.05f, 10000.0f);
			ImGui::DragInt("Resolution##GenBasePlane", &resolution, 0.1f, 2, 100000);
			resolution = std::clamp(resolution, 2, 1024 * 128);
			if (ImGui::Button("Generate Plane##GenBasePlane"))
			{
				appState->mainModel->mesh->GeneratePlane(resolution, scale);
				appState->mainModel->mesh->RecalculateNormals();
				appState->mainModel->SetupMeshOnGPU();
				appState->mainModel->UploadToGPU();
			}
		}
		else if (selected_item == 1)
		{
			// sphere
			static int resolution = 256;
			static float scale = 1.0f;
			ImGui::DragFloat("Radius##GenBaseSphere", &scale, 0.01f, 0.05f, 10000.0f);
			ImGui::DragInt("Resolutione##GenBaseSphere", &resolution, 0.1f, 2, 100000);
			if (ImGui::Button("Generate Sphere##GenBaseSphere"))
			{
				appState->mainModel->mesh->GenerateSphere(resolution, scale);
				appState->mainModel->mesh->RecalculateNormals();
				appState->mainModel->SetupMeshOnGPU();
				appState->mainModel->UploadToGPU();
			}
		}
		else
		{
			ImGui::Text("Not Yet Implemented");
		}
		ImGui::EndPopup();
	}
}

static void CalculateTileSizeAndOffset()
{
	appState->mainMap.tileCount = std::clamp(appState->mainMap.tileCount, 1, 1000000);
	appState->mainMap.tileResolution = appState->mainMap.mapResolution / appState->mainMap.tileCount;
	appState->mainMap.tileSize = (1.0f / appState->mainMap.tileCount);
	appState->mainMap.currentTileX = std::clamp(appState->mainMap.currentTileX, 0, appState->mainMap.tileCount - 1);
	appState->mainMap.currentTileY = std::clamp(appState->mainMap.currentTileY, 0, appState->mainMap.tileCount - 1);
	appState->mainMap.tileOffsetX = appState->mainMap.currentTileX * appState->mainMap.tileSize;
	appState->mainMap.tileOffsetY = appState->mainMap.currentTileY * appState->mainMap.tileSize;
}



static void ShowTerrainControls()
{

	ImGui::Begin("Dashboard");
	ShowChooseBaseModelPopup();
	if (ImGui::Button("Change Base Model")) ImGui::OpenPopup("Choose Base Model##Main");

	if (ImGui::CollapsingHeader("Generator Resolution Settings"))
	{
		bool changed = false;
		changed = PowerOfTwoDropDown("Map Resolution##MainMapGen", &appState->mainMap.mapResolution, 4, 20) || changed;
		changed = ImGui::DragInt("Tile Count##MainMapGen", &appState->mainMap.tileCount, 0.01f, 0, 1000000) || changed;

		if (changed)
		{
			while (appState->states.remeshing);
			appState->mainMap.currentTileX = appState->mainMap.currentTileY = 0;
			CalculateTileSizeAndOffset();
			for (int i = 0; i < 6; i++) appState->mainMap.currentTileDataLayers[i]->Resize(appState->mainMap.tileResolution);
		}
		ImGui::DragInt("Current Tile X##MainMapGen", &appState->mainMap.currentTileX, 0.01f, 0, appState->mainMap.tileCount);
		ImGui::DragInt("Current Tile Y##MainMapGen", &appState->mainMap.currentTileY, 0.01f, 0, appState->mainMap.tileCount);
		ImGui::Text("Tile Resolution: %d", appState->mainMap.tileResolution);
		ImGui::Text("Tile Size: %f", appState->mainMap.tileSize);
		ImGui::Text("Tile Offset: %f %f", appState->mainMap.tileOffsetX, appState->mainMap.tileOffsetY);
		CalculateTileSizeAndOffset();
	}

	ImGui::NewLine();
	ImGui::Checkbox("Auto Save", &appState->states.autoSave);
	ImGui::NewLine();

	if (ImGui::Button("Regenerate")) appState->states.requireRemesh = true;
	
	ImGui::Separator();
	ImGui::NewLine();
	//if(appState->modules.manager->uiModules.size() > 0)
	//	ImGui::Text("UI Modules");
	//int i = 0;
	//for (UIModule* mod : appState->modules.manager->uiModules)
	//ImGui::Checkbox(MAKE_IMGUI_LABEL(i++, mod->windowName), &mod->active);
	ImGui::Separator();
	ImGui::End();
}

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
		appState->meshGenerator->Generate();
		appState->exportManager->Update(); 
		for (int i = 0; i < MAX_VIEWPORT_COUNT; i++) appState->viewportManagers[i]->Update();

		// CTRL Shortcuts
		if ((glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_LEFT_CONTROL) || glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_RIGHT_CONTROL)))
		{
			// Open Shortcut
			if (glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_O)) appState->serailizer->LoadFile(ShowOpenFileDialog("*.terr3d"));

			// Exit Shortcut
			if (glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_Q)) exit(0);

			// Save Shortcut
			if (glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_S))
			{
				if (appState->globals.currentOpenFilePath.size() > 3)
				{
					Log("Saved to " + appState->globals.currentOpenFilePath);
					appState->serailizer->SaveFile(appState->globals.currentOpenFilePath);
				}
				else appState->serailizer->SaveFile(ShowSaveFileDialog("*.terr3d"));
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
					appState->serailizer->SaveFile(ShowSaveFileDialog("*.terr3d"));
				}
			}
		}

		appState->stats.deltatime = deltatime;

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		RenderImGui();
	}

	virtual void OnOneSecondTick() override
	{
		if (!appState->states.ruinning) return;
		appState->globals.secondCounter++;
		if (appState->globals.secondCounter % 5 == 0)
		{
			appState->states.requireRemesh = true;
			if (appState->states.autoSave)
			{
				appState->serailizer->SaveFile(appState->constants.cacheDir + PATH_SEPARATOR "autosave" PATH_SEPARATOR "autosave.terr3d");
				if (appState->globals.currentOpenFilePath.size() > 3) appState->serailizer->SaveFile(appState->globals.currentOpenFilePath);
			}
		}

		GetWindow()->SetVSync(appState->states.vSync);
		appState->stats.frameRate = 1 / appState->stats.deltatime;
	}

	virtual void OnImGuiRender() override
	{
		OnBeforeImGuiRender();
		appState->mainMenu->ShowMainMenu();
		ShowGeneralControls();

		appState->meshGenerator->ShowSettings();
		ShowTerrainControls();
		// Optional Windows

		for (int i = 0; i < MAX_VIEWPORT_COUNT; i++) appState->viewportManagers[i]->Show();
		appState->exportManager->ShowSettings();
		appState->rendererManager->ShowSettings();
		if (appState->windows.styleEditor) ShowStyleEditor(&appState->windows.styleEditor);
		if (appState->windows.textureStore) appState->textureStore->ShowSettings(&appState->windows.textureStore);
		if (appState->windows.osLisc) appState->osLiscences->ShowSettings(&appState->windows.osLisc);
		if (appState->windows.supportersTribute) appState->supportersTribute->ShowSettings(&appState->windows.supportersTribute);

		OnImGuiRenderEnd();
	}

	virtual void OnStart(std::string loadFile) override
	{
		// Set random generator seed from current time
		srand((unsigned int)time(NULL));
		// Setup custom icon for the Main Window
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
		appState->constants.kernelsDir = appState->constants.dataDir + PATH_SEPARATOR "kernels";
		appState->constants.fontsDir = appState->constants.dataDir + PATH_SEPARATOR "fonts";
		appState->constants.liscensesDir = appState->constants.dataDir + PATH_SEPARATOR "licenses";
		appState->constants.skyboxDir = appState->constants.dataDir + PATH_SEPARATOR "skybox";
		appState->constants.modulesDir = appState->constants.dataDir + PATH_SEPARATOR "modules";
		appState->constants.configsDir = appState->constants.dataDir + PATH_SEPARATOR "configs";
		appState->constants.logsDir = appState->constants.dataDir + PATH_SEPARATOR "logs";
		appState->constants.modelsDir = appState->constants.dataDir + PATH_SEPARATOR "models";

		appState->supportersTribute = new SupportersTribute();
		ImGui::GetStyle().WindowMenuButtonPosition = ImGuiDir_None;

		LoadDefaultStyle();

		GetWindow()->SetShouldCloseCallback(OnAppClose);

		glfwSetFramebufferSizeCallback(GetWindow()->GetNativeWindow(), [](GLFWwindow* window, int w, int h) {glfwSwapBuffers(window); });

		glfwSetScrollCallback(GetWindow()->GetNativeWindow(), [](GLFWwindow*, double x, double y) {ImGuiIO& io = ImGui::GetIO(); io.MouseWheel = (float)y; });

		glfwSetDropCallback(GetWindow()->GetNativeWindow(), [](GLFWwindow*, int count, const char** paths)
			{
				for (int i = 0; i < count; i++)
				{
					std::string path = paths[i];
					if (path.find(".terr3d") != std::string::npos)
					{
						appState->serailizer->LoadFile(path);
						break;
					}
					else if (path.find(".terr3dpack") != std::string::npos)
					{
						appState->serailizer->LoadPackedProject(path);
						break;
					}
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
		for (int i = 0; i < 6; i++) { appState->mainMap.currentTileDataLayers[i] = new DataTexture(i); appState->mainMap.currentTileDataLayers[i]->Resize(256); appState->mainMap.currentTileDataLayers[i]->UploadToGPU(); }
		appState->globals.cpuWorkerThreadsActive = std::thread::hardware_concurrency();
		appState->states.requireRemesh = true;


		appState->globals.kernelsIncludeDir = "\"" + appState->constants.kernelsDir + "\"";
		appState->rendererManager = new RendererManager(appState);
		appState->meshGenerator = new MeshGeneratorManager(appState);
		appState->mainMenu = new MainMenu(appState);
		appState->projectManager = new ProjectManager(appState);
		appState->serailizer = new Serializer(appState);
		appState->osLiscences = new OSLiscences(appState);
		appState->textureStore = new TextureStore(appState);
		appState->exportManager = new ExportManager(appState);
		for (int i = 0; i < MAX_VIEWPORT_COUNT; i++) appState->viewportManagers[i] = new ViewportManager(appState);


		glEnable(GL_DEPTH_TEST);

		if (loadFile.size() > 0)
		{
			Log("Loading File from " + loadFile);
			appState->serailizer->LoadFile(loadFile);
		}

		appState->projectManager->SetId(GenerateId(32));

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
	}

	void OnEnd()
	{
		while (appState->states.remeshing);

		using namespace std::chrono_literals;
		std::this_thread::sleep_for(500ms);
		
		for (int i = 0; i < 6; i++) delete appState->mainMap.currentTileDataLayers[i];
		for (int i = 0; i < MAX_VIEWPORT_COUNT; i++) delete appState->viewportManagers[i];

		delete appState->rendererManager;
		delete appState->meshGenerator;
		delete appState->mainModel;
		delete appState->supportersTribute;
		delete appState->mainMenu;
		delete appState->osLiscences;
		delete appState->exportManager;
		delete appState->projectManager;
		delete appState->serailizer;
		delete appState;
	}
};

Application* CreateApplication()
{
	return (mainApp = new MyApp());
}
