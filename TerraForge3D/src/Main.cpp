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
#include "Foliage/FoliagePlacement.h"
#include "Exporters/ExportManager.h"
#include "Misc/OSLiscences.h"
#include "Sky/SkySettings.h"
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


static void ShowStats()
{
	ImGui::Begin("Statistics", &appState->windows.statsWindow);
	ImGui::Text(("Framerate       :" + std::to_string(appState->stats.frameRate)).c_str());
	ImGui::End();
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

static void ResetShader()
{
	bool res = false;

	if (appState->shaders.foliage)
	{
		delete appState->shaders.foliage;
	}

	if (!appState->shaders.wireframe)
	{
		appState->shaders.wireframe = new Shader(ReadShaderSourceFile(appState->constants.shadersDir + PATH_SEPARATOR "default" PATH_SEPARATOR "vert.glsl", &res),
			ReadShaderSourceFile(appState->constants.shadersDir + PATH_SEPARATOR "default" PATH_SEPARATOR "frag.glsl", &res),
			ReadShaderSourceFile(appState->constants.shadersDir + PATH_SEPARATOR "default" PATH_SEPARATOR "geom.glsl", &res));
	}

	// appState->shaders.textureBake = new Shader(ReadShaderSourceFile(appState->constants.shadersDir + PATH_SEPARATOR "texture_bake" PATH_SEPARATOR "vert.glsl", &res),
	//        ReadShaderSourceFile(appState->constants.shadersDir + PATH_SEPARATOR "texture_bake" PATH_SEPARATOR "frag.glsl", &res),
	//        ReadShaderSourceFile(appState->constants.shadersDir + PATH_SEPARATOR "texture_bake" PATH_SEPARATOR "geom.glsl", &res));

	// appState->shaders.terrain = new Shader(ReadShaderSourceFile(appState->constants.shadersDir + PATH_SEPARATOR "default" PATH_SEPARATOR "vert.glsl", &res),
	//                                     ReadShaderSourceFile(appState->constants.shadersDir + PATH_SEPARATOR "default" PATH_SEPARATOR "frag.glsl", &res),
	//                                       ReadShaderSourceFile(appState->constants.shadersDir + PATH_SEPARATOR "default" PATH_SEPARATOR "geom.glsl", &res));

	appState->shaders.foliage = new Shader(ReadShaderSourceFile(appState->constants.shadersDir + PATH_SEPARATOR "foliage" PATH_SEPARATOR "vert.glsl", &res),
		ReadShaderSourceFile(appState->constants.shadersDir + PATH_SEPARATOR "foliage" PATH_SEPARATOR "frag.glsl", &res),
		ReadShaderSourceFile(appState->constants.shadersDir + PATH_SEPARATOR "foliage" PATH_SEPARATOR "geom.glsl", &res));
}


static void DoTheRederThing(float deltaTime, bool renderWater = false, bool bakeTexture = false, bool bakeTextureFinal = false)
{
	static float time;
	time += deltaTime;
	appState->cameras.main.UpdateCamera();
	Shader* shader = nullptr;

	if (appState->states.skyboxEnabled) appState->skyManager->RenderSky(appState->cameras.main.view, appState->cameras.main.pers);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	shader = appState->shaders.terrain;

	shader->SetUniformf("_TextureBake", 0.0f);
	shader->Bind();
	shader->SetTime(&time);
	shader->SetMPV(appState->cameras.main.pv);
	appState->models.mainModel->Update();
	shader->SetUniformMat4("_Model", appState->models.mainModel->modelMatrix);
	shader->SetLightCol(appState->lightManager->color);
	shader->SetLightPos(appState->lightManager->position);
	shader->SetUniformf("_LightStrength", appState->lightManager->strength);
	float tmp[3];
	tmp[0] = appState->globals.viewportMousePosX;
	tmp[1] = appState->globals.viewportMousePosY;
	tmp[2] = ImGui::GetIO().MouseDown[0];
	shader->SetUniform3f("_MousePos", tmp);
	tmp[0] = appState->frameBuffers.main->GetWidth();
	tmp[1] = appState->frameBuffers.main->GetHeight();
	tmp[2] = 1;
	shader->SetUniform3f("_Resolution", tmp);
	shader->SetUniformf("_SeaLevel", appState->seaManager->level);
	shader->SetUniform3f("_CameraPos", appState->cameras.main.position);
	shader->SetUniformf("_MapTileResolution", appState->mainMap.tileResolution);
	shader->SetUniformf("_CameraNear", appState->cameras.main.cNear);
	shader->SetUniformf("_CameraFar", appState->cameras.main.cFar);
	shader->SetUniformf("_FlatShade", appState->states.useGPUForNormals ? 1.0f : 0.0f);
	appState->shadingManager->UpdateShaders();

	for (int i = 0; i < 6; i++)
	{
		appState->mainMap.currentTileDataLayers[i]->Bind(7 + i);
		shader->SetUniformi(std::string("_MapDataLayers[") + std::to_string(i) + "]", 7 + i);
	}

	appState->models.mainModel->Render();

	if (appState->states.showFoliage)
	{
		shader = appState->shaders.foliage;
		shader->Bind();
		shader->SetTime(&time);
		shader->SetMPV(appState->cameras.main.pv);
		shader->SetLightCol(appState->lightManager->color);
		shader->SetLightPos(appState->lightManager->position);
		shader->SetUniformf("_LightStrength", appState->lightManager->strength);
		float tmp[3];
		tmp[0] = appState->globals.viewportMousePosX;
		tmp[1] = appState->globals.viewportMousePosY;
		tmp[2] = ImGui::GetIO().MouseDown[0];
		shader->SetUniform3f("_MousePos", tmp);
		tmp[0] = appState->frameBuffers.main->GetWidth();
		tmp[1] = appState->frameBuffers.main->GetHeight();
		tmp[2] = 1;
		shader->SetUniform3f("_Resolution", tmp);
		shader->SetUniformf("_SeaLevel", appState->seaManager->level);
		shader->SetUniform3f("_CameraPos", appState->cameras.main.position);
		shader->SetUniformf("_CameraNear", appState->cameras.main.cNear);
		shader->SetUniformf("_CameraFar", appState->cameras.main.cFar);
		appState->foliageManager->RenderFoliage(appState->cameras.main);
	}

	// For Future
	//gridTex->Bind(5);
	//grid->Render();

	//if (appState->seaManager->enabled && renderWater)
	{
		//	appState->seaManager->Render(appState->cameras.main, appState->lightManager, appState->frameBuffers.reflection, time);
	}

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
				if (tmp) { delete appState->models.mainModel; appState->models.mainModel = tmp; tmp->mesh->RecalculateNormals(); tmp->SetupMeshOnGPU(); tmp->UploadToGPU(); }
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
			if (ImGui::Button("Generate Plane##GenBasePlane"))
			{
				appState->models.mainModel->mesh->GeneratePlane(resolution, scale);
				appState->models.mainModel->mesh->RecalculateNormals();
				appState->models.mainModel->SetupMeshOnGPU();
				appState->models.mainModel->UploadToGPU();
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
				appState->models.mainModel->mesh->GenerateSphere(resolution, scale);
				appState->models.mainModel->mesh->RecalculateNormals();
				appState->models.mainModel->SetupMeshOnGPU();
				appState->models.mainModel->UploadToGPU();
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


//static void GenerateMap()
//{
//	float step = appState->mainMap.tileSize/ appState->mainMap.tileResolution;
//	for (int i = 0; i < appState->mainMap.tileResolution; i++)
//	{
//		for (int j = 0; j < appState->mainMap.tileResolution; j++)
//		{
//			float y = (float)i * step + appState->mainMap.tileOffsetY;
//			float x = (float)j * step + appState->mainMap.tileOffsetX;
//			appState->mainMap.currentTileDataLayers[0]->SetPixelI(j, i, (sin(x) + cos(y)) * x, 0.0f, 0.0f, 0.0f);
//		}
//	}
//}


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
	ImGui::Checkbox("Show Skybox", &appState->states.skyboxEnabled);
	ImGui::Checkbox("Show Sea", &appState->seaManager->enabled);
	ImGui::Checkbox("Show Foliage", &appState->states.showFoliage);
	ImGui::NewLine();

	if (ImGui::Button("Regenerate")) appState->states.requireRemesh = true;
	if (ImGui::Button("Refresh Shaders")) ResetShader();
	if (ImGui::Button("Export Frame")) ExportTexture(appState->frameBuffers.main->GetRendererID(), ShowSaveFileDialog(".png"), appState->frameBuffers.main->GetWidth(), appState->frameBuffers.main->GetHeight());

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

static void MouseMoveCallback(float x, float y)
{
	static glm::vec2 prevMousePos; // This is temporary
	float deltaX = x - prevMousePos.x;
	float deltaY = y - prevMousePos.y;
	if (appState->states.mouseButton2)
	{
		deltaX = std::clamp(deltaX, -200.0f, 200.0f);
		deltaY = std::clamp(deltaY, -200.0f, 200.0f);
		appState->cameras.main.rotation[0] += deltaX * appState->globals.mouseSpeed;
		appState->cameras.main.rotation[1] += deltaY * appState->globals.mouseSpeed;
	}
	prevMousePos.x = x;
	prevMousePos.y = y;
}

static void MouseScrollCallback(float amount)
{
	appState->globals.mouseScrollAmount = amount;
	glm::vec3 pPos = glm::vec3(appState->cameras.main.position[0], appState->cameras.main.position[1], appState->cameras.main.position[2]);
	pPos += appState->constants.FRONT * amount * appState->globals.scrollSpeed;
	appState->cameras.main.position[0] = pPos.x;
	appState->cameras.main.position[1] = pPos.y;
	appState->cameras.main.position[2] = pPos.z;
}


static void ShowMainScene()
{
	auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
	auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
	auto viewportOffset = ImGui::GetWindowPos();
	ImGui::Begin("Viewport");
	{
		ImGui::BeginChild("MainRender");

		if (ImGui::IsWindowHovered())
		{
			ImGuiIO io = ImGui::GetIO();
			MouseMoveCallback(io.MousePos.x, io.MousePos.y);
			MouseScrollCallback(io.MouseWheel);
			appState->states.mouseButton1 = io.MouseDown[0];
			appState->states.mouseButton2 = io.MouseDown[2];
			appState->states.mouseButton3 = io.MouseDown[1];

			if (ImGui::GetIO().MouseDown[1])
			{
				appState->cameras.main.position[0] += -io.MouseDelta.x * 0.005f * glm::distance(glm::vec3(0.0f), glm::vec3(appState->cameras.main.position[0], appState->cameras.main.position[1], appState->cameras.main.position[2]));
				appState->cameras.main.position[1] += io.MouseDelta.y * 0.005f * glm::distance(glm::vec3(0.0f), glm::vec3(appState->cameras.main.position[0], appState->cameras.main.position[1], appState->cameras.main.position[2]));
			}

			appState->globals.viewportMousePosX = ImGui::GetIO().MousePos.x - viewportOffset.x;
			appState->globals.viewportMousePosY = ImGui::GetIO().MousePos.y - viewportOffset.y;
		}

		ImVec2 wsize = ImGui::GetWindowSize();
		appState->globals.viewportSize[0] = wsize.x;
		appState->globals.viewportSize[1] = wsize.y;

		ImGui::Image((ImTextureID)appState->frameBuffers.main->GetColorTexture(), wsize, ImVec2(0, 1), ImVec2(1, 0));

		ImGui::EndChild();
	}
	ImGui::End();
}

static void SaveFile(std::string file = ShowSaveFileDialog())
{
	appState->serailizer->SaveFile(file);
}

static void OpenSaveFile(std::string file = ShowOpenFileDialog(".terr3d"))
{
	appState->serailizer->LoadFile(file);
}


static void PackProject(std::string path = ShowSaveFileDialog())
{
	appState->serailizer->PackProject(path);
}

static void LoadPackedProject(std::string path = ShowOpenFileDialog())
{
	appState->serailizer->LoadPackedProject(path);
}


static void ShowModuleManager()
{
	appState->modules.manager->ShowSettings(&appState->windows.modulesManager);
}

static void OnBeforeImGuiRender()
{
	static bool dockspaceOpen = true;
	static bool opt_fullscreen_persistant = true;
	bool opt_fullscreen = opt_fullscreen_persistant;
	static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

	if (opt_fullscreen)
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.1f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	}

	if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
	{
		window_flags |= ImGuiWindowFlags_NoBackground;
	}

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockSpace", &dockspaceOpen, window_flags);
	ImGui::PopStyleVar();

	if (opt_fullscreen)
	{
		ImGui::PopStyleVar(2);
	}

	// DockSpace
	ImGuiIO& io = ImGui::GetIO();
	ImGuiStyle& style = ImGui::GetStyle();
	float minWinSizeX = style.WindowMinSize.x;
	style.WindowMinSize.x = 370.0f;

	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
	{
		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
	}

	style.WindowMinSize.x = minWinSizeX;
}

static void OnImGuiRenderEnd()
{
	ImGui::End();
}

static void SetUpIcon()
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
		if (!appState->states.ruinning)
		{
			return;
		}

		appState->meshGenerator->Generate();

		// CTRL Shortcuts
		if ((glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_LEFT_CONTROL) || glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_RIGHT_CONTROL)))
		{
			// Open Shortcut
			if (glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_O)) OpenSaveFile();

			// Exit Shortcut
			if (glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_Q)) exit(0);

			// Save Shortcut
			if (glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_S))
			{
				if (appState->globals.currentOpenFilePath.size() > 3)
				{
					Log("Saved to " + appState->globals.currentOpenFilePath);
					SaveFile(appState->globals.currentOpenFilePath);
				}
				else SaveFile();
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
					SaveFile();
				}
			}
		}

		appState->stats.deltatime = deltatime;

		// Render
		{
			glBindFramebuffer(GL_FRAMEBUFFER, appState->frameBuffers.main->GetRendererID());
			glViewport(0, 0, appState->frameBuffers.main->GetWidth(), appState->frameBuffers.main->GetHeight());
			GetWindow()->Clear();
			DoTheRederThing(deltatime, true);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		RenderImGui();

		appState->exportManager->Update();
		appState->modules.manager->UpdateModules();
	}

	virtual void OnOneSecondTick() override
	{
		if (!appState->states.ruinning)
		{
			return;
		}

		appState->globals.secondCounter++;

		if (appState->globals.secondCounter % 5 == 0)
		{
			appState->states.requireRemesh = true;
			if (appState->states.autoSave)
			{
				SaveFile(appState->constants.cacheDir + PATH_SEPARATOR "autosave" PATH_SEPARATOR "autosave.terr3d");

				if (appState->globals.currentOpenFilePath.size() > 3)
				{
					SaveFile(appState->globals.currentOpenFilePath);
				}
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

		if (appState->windows.cameraControls)
		{
			ImGui::Begin("Camera Controls", &appState->windows.cameraControls);
			appState->cameras.main.ShowSettings();
			ImGui::Checkbox("Auto Calculate Aspect Ratio", &appState->states.autoAspectCalcRatio);
			ImGui::End();
		}

		if (appState->states.autoAspectCalcRatio && (appState->globals.viewportSize[1] != 0 && appState->globals.viewportSize[0] != 0))
		{
			appState->cameras.main.aspect = appState->globals.viewportSize[0] / appState->globals.viewportSize[1];
		}

		appState->lightManager->ShowSettings(true, &appState->windows.lightControls);
		appState->meshGenerator->ShowSettings();
		ShowTerrainControls();
		ShowMainScene();

		// Optional Windows

		if (appState->windows.statsWindow) ShowStats();
		appState->seaManager->ShowSettings(&appState->windows.seaEditor);
		if (appState->windows.modulesManager) ShowModuleManager();
		if (appState->windows.styleEditor) ShowStyleEditor(&appState->windows.styleEditor);
		if (appState->windows.foliageManager) appState->foliageManager->ShowSettings(&appState->windows.foliageManager);
		if (appState->windows.textureStore) appState->textureStore->ShowSettings(&appState->windows.textureStore);
		if (appState->windows.shadingManager) appState->shadingManager->ShowSettings(&appState->windows.shadingManager);
		if (appState->windows.osLisc) appState->osLiscences->ShowSettings(&appState->windows.osLisc);
		if (appState->windows.supportersTribute) appState->supportersTribute->ShowSettings(&appState->windows.supportersTribute);
		if (appState->windows.skySettings) appState->skyManager->ShowSettings(&appState->windows.skySettings);
		if (appState->windows.exportManager) appState->exportManager->ShowSettings();

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

		appState->models.mainModel->mesh->GeneratePlane(256, 1.0f);
		appState->models.mainModel->mesh->RecalculateNormals();
		appState->models.mainModel->SetupMeshOnGPU();
		appState->models.mainModel->UploadToGPU();

		appState->mainMap.tileCount = 1;
		appState->mainMap.mapResolution = 256;
		appState->mainMap.tileResolution = 256;
		appState->mainMap.tileSize = 1.0f;
		appState->mainMap.tileOffsetX = appState->mainMap.tileOffsetY = 0.0f;
		appState->mainMap.currentTileX = appState->mainMap.currentTileY = 0;
		for (int i = 0; i < 6; i++) { appState->mainMap.currentTileDataLayers[i] = new DataTexture(); appState->mainMap.currentTileDataLayers[i]->Resize(256); appState->mainMap.currentTileDataLayers[i]->UploadToGPU(); }
		appState->globals.cpuWorkerThreadsActive = std::thread::hardware_concurrency();
		appState->states.requireRemesh = true;


		appState->globals.kernelsIncludeDir = "\"" + appState->constants.kernelsDir + "\"";
		appState->modules.manager = new ModuleManager(appState);
		appState->meshGenerator = new MeshGeneratorManager(appState);
		appState->mainMenu = new MainMenu(appState);
		appState->seaManager = new SeaManager(appState);
		appState->lightManager = new LightManager();
		appState->skyManager = new SkyManager(appState);
		appState->foliageManager = new FoliageManager(appState);
		appState->projectManager = new ProjectManager(appState);
		appState->serailizer = new Serializer(appState);
		appState->osLiscences = new OSLiscences(appState);
		appState->textureStore = new TextureStore(appState);
		appState->shadingManager = new ShadingManager(appState);
		appState->exportManager = new ExportManager(appState);


		ResetShader();

		//appState->meshGenerator->GenerateSync();

		appState->models.screenQuad->SetupMeshOnGPU();
		appState->models.screenQuad->mesh->GenerateScreenQuad();
		appState->models.screenQuad->mesh->RecalculateNormals();
		appState->models.screenQuad->UploadToGPU();
		glEnable(GL_DEPTH_TEST);

		if (loadFile.size() > 0)
		{
			Log("Loading File from " + loadFile);
			OpenSaveFile(loadFile);
		}

		appState->projectManager->SetId(GenerateId(32));

		float t = 1.0f;
		// Load Fonts
		LoadUIFont("Open-Sans-Regular", 18, appState->constants.fontsDir + PATH_SEPARATOR "OpenSans-Regular.ttf");
		LoadUIFont("OpenSans-Bold", 25, appState->constants.fontsDir + PATH_SEPARATOR "OpenSans-Bold.ttf");
		LoadUIFont("OpenSans-Semi-Bold", 22, appState->constants.fontsDir + PATH_SEPARATOR "OpenSans-Bold.ttf");
		appState->frameBuffers.main = new FrameBuffer(1280, 720);
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

		appState->windows.shadingManager = true; // TEMPORARY FOR DEBUG ONLY
		Log("Started Up App!");
	}

	void OnEnd()
	{
		while (appState->states.remeshing);

		using namespace std::chrono_literals;
		std::this_thread::sleep_for(500ms);
		
		for (int i = 0; i < 6; i++) delete appState->mainMap.currentTileDataLayers[i];

		delete appState->shaders.terrain;
		delete appState->shaders.foliage;
		delete appState->shaders.wireframe;
		delete appState->supportersTribute;
		delete appState->mainMenu;
		delete appState->skyManager;
		delete appState->frameBuffers.main;
		delete appState->osLiscences;
		delete appState->exportManager;
		delete appState->projectManager;
		delete appState->foliageManager;
		delete appState->shadingManager;
		//		delete appState->seaManager;
		delete appState->lightManager;
		delete appState->serailizer;
		delete appState;
	}
};

Application* CreateApplication()
{
	mainApp = new MyApp();
	return mainApp;
}
