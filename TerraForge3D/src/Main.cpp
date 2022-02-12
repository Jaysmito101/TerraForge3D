#include "../resource.h"

#include <Utils.h>
#include <AppStyles.h>
#include <ExplorerControls.h>
#include <Data/VersionInfo.h>
#include <TextureSettings.h>
#include <ViewportFramebuffer.h>
#include <Modules/ModuleManager.h>
#include <AppShaderEditor.h>
#include <OSLiscenses.h>
#include <FiltersManager.h>
#include <FoliagePlacement.h>
#include <SkySettings.h>
#include <SupportersTribute.h>
#include <ExportManager.h>
#include <TextureStore.h>

// TerraForge3D Base


#define GLFW_EXPOSE_NATIVE_WIN32 // For Windows
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "Base/Base.h"
#include "Base/EntryPoint.h"

// TerraForge3D Application
#include "Data/ProjectData.h"
#include "Data/ApplicationState.h"
#include "Generators/MeshGeneratorManager.h"

#undef cNear
#undef cFar

#include <FastNoiseLite.h>


#include <json.hpp>
#include <zip.h>
#include <sys/stat.h>
#include <dirent/dirent.h>





#include "NoiseLayers/LayeredNoiseManager.h"

static ApplicationState* appState;
static Application* mainApp;


static LayeredNoiseManager* noiseGen;




void OnAppClose(int x, int y)
{
	Log("Close App");
	appState->mainApp->Close();
}


static void ShowStats()
{
	ImGui::Begin("Statistics", &appState->windows.statsWindow);

	ImGui::Text(("Vertex Count    :" + std::to_string(appState->stats.vertexCount)).c_str());
	ImGui::Text(("Triangles Count :" + std::to_string(appState->stats.triangles)).c_str());
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

	if (ImGui::Button("Exit")) {
		exit(0);
	}

	ImGui::End();
}

static void ResetShader() {
	bool res = false;
	if (appState->shaders.terrain)
		delete appState->shaders.terrain;
	if (appState->shaders.textureBake)
		delete appState->shaders.textureBake;
	if(appState->shaders.foliage)
		delete appState->shaders.foliage;

	if (!appState->shaders.wireframe)
		appState->shaders.wireframe = new Shader(GetDefaultVertexShaderSource(), GetDefaultFragmentShaderSource(), GetWireframeGeometryShaderSource());
	
	appState->shaders.textureBake = new Shader(ReadShaderSourceFile(GetExecutableDir() + "\\Data\\shaders\\texture_bake\\vert.glsl", &res), ReadShaderSourceFile(GetExecutableDir() + "\\Data\\shaders\\texture_bake\\frag.glsl", &res), ReadShaderSourceFile(GetExecutableDir() + "\\Data\\shaders\\texture_bake\\geom.glsl", &res));
	appState->shaders.terrain = new Shader(GetVertexShaderSource(), GetFragmentShaderSource(), GetGeometryShaderSource());
	appState->shaders.foliage = new Shader(ReadShaderSourceFile(GetExecutableDir() + "\\Data\\shaders\\foliage\\vert.glsl", &res), ReadShaderSourceFile(GetExecutableDir() + "\\Data\\shaders\\foliage\\frag.glsl", &res), ReadShaderSourceFile(GetExecutableDir() + "\\Data\\shaders\\foliage\\geom.glsl", &res));
}

static void RegenerateMesh() 
{
	appState->meshGenerator->Generate();
}


static void DoTheRederThing(float deltaTime, bool renderWater = false, bool bakeTexture = false) {

	static float time;
	time += deltaTime;
	appState->cameras.main.UpdateCamera();

	Shader* shader;
	
	// Texture Bake
	if (appState->states.textureBake)
	{
		Camera cam;
		float CameraR[3] = { 5185.0f, 0.0f, 0.0f };
		cam.cFar = 1000;
		cam.aspect = 1;
		
		cam.position[0] = 0.0f;
		cam.position[1] = 0.0f;
		cam.position[2] = appState->cameras.main.position[2];

		cam.rotation[0] = 5185.0f;
		cam.rotation[1] = 0.0f;
		cam.rotation[2] = 0.0f;

		cam.UpdateCamera();

		shader = appState->shaders.textureBake;
		shader->Bind();
		shader->SetTime(&time);
		shader->SetMPV(cam.pv);
		shader->SetUniformMat4("_Model", appState->models.coreTerrain->modelMatrix);
		shader->SetLightCol(appState->lightManager->color);
		shader->SetLightPos(appState->lightManager->position);
		float tmp[3];
		tmp[0] = 1024;
		tmp[1] = 1024;
		tmp[2] = 1;
		shader->SetUniform3f("_Resolution", tmp);
		shader->SetUniform3f("_CameraPos", appState->cameras.main.position);
		shader->SetUniform3f("_HMapMinMax", appState->globals.hMapC);
		shader->SetUniformf("_SeaLevel", appState->seaManager->level);
		shader->SetUniformf("_CameraNear", appState->cameras.main.cNear);
		shader->SetUniformf("_CameraFar", appState->cameras.main.cFar);
		shader->SetUniformf("_Mode", appState->globals.textureBakeMode);
		UpdateDiffuseTexturesUBO(shader->GetNativeShader(), "_DiffuseTextures");
		appState->models.coreTerrain->Render();
	}
	else {
		if (appState->states.skyboxEnabled)
			RenderSky(appState->cameras.main.view, appState->cameras.main.pers);


		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		if (appState->states.wireFrameMode)
			shader = appState->shaders.wireframe;
		else
			shader = appState->shaders.terrain;

		shader->Bind();
		shader->SetTime(&time);
		shader->SetMPV(appState->cameras.main.pv);
		appState->models.coreTerrain->Update();
		shader->SetUniformMat4("_Model", appState->models.coreTerrain->modelMatrix);
		shader->SetLightCol(appState->lightManager->color);
		shader->SetLightPos(appState->lightManager->position);
		float tmp[3];
		tmp[0] = appState->globals.viewportMousePosX;
		tmp[1] = appState->globals.viewportMousePosY;
		tmp[2] = ImGui::GetIO().MouseDown[0];
		shader->SetUniform3f("_MousePos", tmp);
		tmp[0] = 800;
		tmp[1] = 600;
		tmp[2] = 1;
		shader->SetUniform3f("_Resolution", tmp);
		shader->SetUniformf("_SeaLevel", appState->seaManager->level);
		shader->SetUniform3f("_CameraPos", appState->cameras.main.position);
		shader->SetUniformf("_CameraNear", appState->cameras.main.cNear);
		shader->SetUniformf("_CameraFar", appState->cameras.main.cFar);
		shader->SetUniformf("_FlatShade", appState->states.useGPUForNormals ? 1.0f : 0.0f);

		UpdateDiffuseTexturesUBO(shader->GetNativeShader(), "_DiffuseTextures");
		if(appState->mode == ApplicationMode::TERRAIN)
			appState->models.coreTerrain->Render();
		else if (appState->mode == ApplicationMode::CUSTOM_BASE)
			appState->models.customBase->Render();


		if (appState->states.showFoliage)
		{
			shader = appState->shaders.foliage;
			shader->Bind();
			shader->SetTime(&time);
			shader->SetMPV(appState->cameras.main.pv);
			shader->SetUniformMat4("_Model", appState->models.coreTerrain->modelMatrix);
			shader->SetLightCol(appState->lightManager->color);
			shader->SetLightPos(appState->lightManager->position);
			float tmp[3];
			tmp[0] = appState->globals.viewportMousePosX;
			tmp[1] = appState->globals.viewportMousePosY;
			tmp[2] = ImGui::GetIO().MouseDown[0];
			shader->SetUniform3f("_MousePos", tmp);
			tmp[0] = 800;
			tmp[1] = 600;
			tmp[2] = 1;
			shader->SetUniform3f("_Resolution", tmp);
			shader->SetUniformf("_SeaLevel", appState->seaManager->level);
			shader->SetUniform3f("_CameraPos", appState->cameras.main.position);
			shader->SetUniformf("_CameraNear", appState->cameras.main.cNear);
			shader->SetUniformf("_CameraFar", appState->cameras.main.cFar);
			RenderFoliage(shader, appState->cameras.main);
		}



		// For Future
		//gridTex->Bind(5);
		//grid->Render();


		if (appState->seaManager->enabled && renderWater) {
			appState->seaManager->Render(appState->cameras.main, appState->lightManager, appState->frameBuffers.relflection, time);
		}
	}


	 
}

void PostProcess(float deltatime)
{
	float pos[3] = { 0, 0, 1.8 };
	float rot[3] = {0, 0, 0};
	appState->cameras.postPorcess.aspect = 1;
	
	appState->cameras.postPorcess.position[0] = 0.0f;
	appState->cameras.postPorcess.position[0] = 0.0f;
	appState->cameras.postPorcess.position[0] = 1.8f;

	appState->cameras.postPorcess.rotation[0] = 0.0f;
	appState->cameras.postPorcess.rotation[1] = 0.0f;
	appState->cameras.postPorcess.rotation[2] = 0.0f;

	appState->cameras.postPorcess.UpdateCamera();

	float viewDir[3] = { glm::value_ptr(appState->cameras.main.pv)[2], glm::value_ptr(appState->cameras.main.pv)[6], glm::value_ptr(appState->cameras.main.pv)[10] };

	appState->shaders.postProcess->Bind(); 

	appState->shaders.postProcess->SetUniform3f("_CameraPosition", appState->cameras.main.position);
	appState->shaders.postProcess->SetUniform3f("_ViewDir", viewDir);

	float cloudsBoxMinMax[3] = {0.0f}; // Temporary

	appState->shaders.postProcess->SetUniform3f("cloudsBoxBoundsMin", cloudsBoxMinMax);
	appState->shaders.postProcess->SetUniform3f("cloudsBoxBoundsMax", cloudsBoxMinMax);
	 
	appState->shaders.postProcess->SetMPV(appState->cameras.postPorcess.pv);

	glActiveTexture(GL_TEXTURE5);	
	glBindTexture(GL_TEXTURE_2D, GetViewportFramebufferColorTextureId());
	appState->shaders.postProcess->SetUniformi("_ColorTexture", 5);

	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, GetViewportFramebufferDepthTextureId()); 
	appState->shaders.postProcess->SetUniformi("_DepthTexture", 6);

	appState->models.screenQuad->Render();
} 
 
static void ChangeCustomModel(std::string mdstr = ShowOpenFileDialog("*.obj"))
{
	while (appState->states.remeshing);
	if(!appState->states.usingBase)
	{
		delete appState->models.customBase;
		delete appState->models.customBaseCopy;
	}
	if(mdstr.size() > 3)
	{
		appState->globals.currentBaseModelPath = mdstr;
		appState->states.usingBase = false;	
		appState->models.customBase = LoadModel(appState->globals.currentBaseModelPath);
		appState->models.customBaseCopy = LoadModel(appState->globals.currentBaseModelPath); // Will Be Replaced with something efficient
		appState->mode = ApplicationMode::CUSTOM_BASE;
	}
}

static void ShowChooseBaseModelPopup()
{
	// TEMP
	static std::shared_ptr<Texture2D> plane = std::make_shared<Texture2D>(GetExecutableDir() + "\\Data\\textures\\ui_thumbs\\plane.png", false, false);
	static std::shared_ptr<Texture2D> sphere = std::make_shared<Texture2D>(GetExecutableDir() + "\\Data\\textures\\ui_thumbs\\sphere.png", false, false);
	static std::shared_ptr<Texture2D> cube = std::make_shared<Texture2D>(GetExecutableDir() + "\\Data\\textures\\ui_thumbs\\cube.png", false, false);
	static std::shared_ptr<Texture2D> cone = std::make_shared<Texture2D>(GetExecutableDir() + "\\Data\\textures\\ui_thumbs\\cone.png", false, false);
	static std::shared_ptr<Texture2D> cyllinder = std::make_shared<Texture2D>(GetExecutableDir() + "\\Data\\textures\\ui_thumbs\\cylinder.png", false, false);
	static std::shared_ptr<Texture2D> torus = std::make_shared<Texture2D>(GetExecutableDir() + "\\Data\\textures\\ui_thumbs\\torus.png", false, false);
	// TEMP

	if (ImGui::BeginPopup("Choose Base Model"))
	{
		if (ImGui::Button("Open From File")) 
		{
			ChangeCustomModel();
			ImGui::CloseCurrentPopup();
		}

		if (ImGui::Button("Close"))
			ImGui::CloseCurrentPopup();

		ImGui::BeginChild("Choose Base Model##Child", ImVec2(650, 450));

		if (ImGui::ImageButton((ImTextureID)plane->GetRendererID(), ImVec2(200, 200)) )
		{
			while (appState->states.remeshing);
			delete appState->models.customBase;
			delete appState->models.customBaseCopy;
			appState->states.usingBase = true;
			appState->mode = ApplicationMode::TERRAIN;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::ImageButton((ImTextureID)sphere->GetRendererID(), ImVec2(200, 200)))
		{
			ChangeCustomModel(GetExecutableDir() + "\\Data\\models\\sphere.obj");
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::ImageButton((ImTextureID)cube->GetRendererID(), ImVec2(200, 200)))
		{
			ChangeCustomModel(GetExecutableDir() + "\\Data\\models\\cube.obj");
			ImGui::CloseCurrentPopup();
		}


		if (ImGui::ImageButton((ImTextureID)torus->GetRendererID(), ImVec2(200, 200)))
		{
			ChangeCustomModel(GetExecutableDir() + "\\Data\\models\\torus.obj");
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::ImageButton((ImTextureID)cone->GetRendererID(), ImVec2(200, 200)))
		{
			ChangeCustomModel(GetExecutableDir() + "\\Data\\models\\cone.obj");
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::ImageButton((ImTextureID)cyllinder->GetRendererID(), ImVec2(200, 200)))
		{
			ChangeCustomModel(GetExecutableDir() + "\\Data\\models\\cylinder.obj");
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndChild();

		ImGui::EndPopup();
	}
}

static void ShowTerrainControls()
{
	static bool exp = false;
	ImGui::Begin("Dashboard");

	ShowChooseBaseModelPopup();
	
	if(appState->mode != ApplicationMode::TERRAIN)
	{
		ImGui::Text(("Current Base Model : " + appState->globals.currentBaseModelPath).c_str());
		if(ImGui::Button("Change Current Base"))
		{
			ImGui::OpenPopup("Choose Base Model");
		}
		if(ImGui::Button("Switch to Plane"))
		{
			while(appState->states.remeshing);
			delete appState->models.customBase;
			delete appState->models.customBaseCopy;
			appState->states.usingBase = true;
			appState->mode = ApplicationMode::TERRAIN;
		}
	}
	else
	{
		ImGui::Text("Current Base Model : Default Plane");
		if (ImGui::Button("Change Current Base"))
		{
			ImGui::OpenPopup("Choose Base Model");
		}
	} 


	if(appState->mode == ApplicationMode::TERRAIN)
		ImGui::DragInt("Mesh Resolution", &appState->globals.resolution, 1, 2, 16 * 4096);

	if(appState->mode == ApplicationMode::TERRAIN || appState->mode == ApplicationMode::CUSTOM_BASE)
		ImGui::DragFloat("Mesh Scale", &appState->globals.scale, 0.1f, 1.0f, 5000.0f);


	static const char* tbmi[] = {"Diffuse Map", "Heigt Map", "Normal Map"};
	if (ImGui::BeginCombo("Texture Bake Mode##combo", tbmi[appState->globals.textureBakeMode]))
	{
	    for (int n = 0; n < IM_ARRAYSIZE(tbmi); n++)
	    {
        	bool is_selected = (appState->globals.textureBakeMode == n);
	        if (ImGui::Selectable(tbmi[n], is_selected))
        	    appState->globals.textureBakeMode = n;
	        if (is_selected)
        	    ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
	    }	
	    ImGui::EndCombo();
	}

	if(appState->globals.textureBakeMode == 1)
	{
		ImGui::DragFloat2("Height Map Min Max", appState->globals.hMapC, 0.01f);
	}


	ImGui::NewLine();

	ImGui::Checkbox("Auto Update", &appState->states.autoUpdate);
	ImGui::Checkbox("Post Processing", &appState->states.postProcess);
	ImGui::Checkbox("Auto Save", &appState->states.autoSave);
	ImGui::Checkbox("Wireframe Mode", &appState->states.wireFrameMode);
	ImGui::Checkbox("Show Skybox", &appState->states.skyboxEnabled);
	ImGui::Checkbox("Show Sea", &appState->seaManager->enabled);
	ImGui::Checkbox("Show Foliage", &appState->states.showFoliage);
	ImGui::Checkbox("Infinite Explorer Mode", &appState->states.iExploreMode);
	ImGui::Checkbox("Explorer Mode", &appState->states.exploreMode);
	ImGui::Checkbox("Texture Bake Mode", &appState->states.textureBake);
	ImGui::NewLine();
	if (ImGui::Button("Update Mesh"))
		RegenerateMesh();

	if (ImGui::Button("Recalculate Normals")) {

		while (appState->states.remeshing);
		if (appState->mode == ApplicationMode::TERRAIN)
		{
			appState->models.coreTerrain->mesh->RecalculateNormals();
			appState->models.coreTerrain->UploadToGPU();
		}
		else if (appState->mode == ApplicationMode::CUSTOM_BASE) 
		{
			appState->models.customBase->mesh->RecalculateNormals();
			appState->models.customBase->UploadToGPU();
		}
		else
		{
			Log("Unsupported App Mode!");
		}
	}

	if (ImGui::Button("Refresh Shaders")) {
		ResetShader();
	}

	if (ImGui::Button("Use Custom Shaders")) {
		appState->windows.shaderEditorWindow = true;
	}

	if (ImGui::Button("Texture Settings")) {
		appState->windows.texturEditorWindow = true;
	}

	if (ImGui::Button("Sea Settings")) {
		appState->windows.seaEditor = true;
	}
	if (ImGui::Button("Filter Settings")) {
		appState->windows.filtersManager = true;
	}

	if (ImGui::Button("Export Frame")) {
		if(appState->states.textureBake)
			ExportTexture(appState->frameBuffers.textureExport->GetRendererID() , ShowSaveFileDialog(".png"), 1024, 1024);
		else
			ExportTexture(GetViewportFramebufferId(), ShowSaveFileDialog(".png"), 800, 600);
	}

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

static void MouseMoveCallback(float x, float y) {
	static glm::vec2 prevMousePos; // This is temporary
	float deltaX = x - prevMousePos.x;
	float deltaY = y - prevMousePos.y;

	if (appState->states.mouseButton2) {


		if (deltaX > 200) {
			deltaX = 0;
		}

		if (deltaY > 200) {
			deltaY = 0;
		}

		appState->cameras.main.rotation[0] += deltaX * appState->globals.mouseSpeed;
		appState->cameras.main.rotation[1] += deltaY * appState->globals.mouseSpeed;
	}


	prevMousePos.x = x;
	prevMousePos.y = y;

}

static void MouseScrollCallback(float amount) {
	appState->globals.mouseScrollAmount = amount;
	glm::vec3 pPos = glm::vec3(appState->cameras.main.position[0], appState->cameras.main.position[1], appState->cameras.main.position[2]);
	pPos += appState->constants.FRONT * amount * appState->globals.scrollSpeed;
	appState->cameras.main.position[0] = pPos.x;
	appState->cameras.main.position[1] = pPos.y;
	appState->cameras.main.position[2] = pPos.z;
}


static void ShowMainScene() {
	auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
	auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
	auto viewportOffset = ImGui::GetWindowPos();

	ImGui::Begin("Viewport");
	{
		ImGui::BeginChild("MainRender");
		if (ImGui::IsWindowHovered()) {
			ImGuiIO io = ImGui::GetIO();
			MouseMoveCallback(io.MousePos.x, io.MousePos.y);
			MouseScrollCallback(io.MouseWheel);
			appState->states.mouseButton1 = io.MouseDown[0];
			appState->states.mouseButton2 = io.MouseDown[2];
			appState->states.mouseButton3 = io.MouseDown[1];
			if (ImGui::GetIO().MouseDown[1]) {
				appState->cameras.main.position[0] += -io.MouseDelta.x * 0.005f * glm::distance(glm::vec3(0.0f), glm::vec3(appState->cameras.main.position[0], appState->cameras.main.position[1], appState->cameras.main.position[2]));
				appState->cameras.main.position[1] += io.MouseDelta.y * 0.005f * glm::distance(glm::vec3(0.0f), glm::vec3(appState->cameras.main.position[0], appState->cameras.main.position[1], appState->cameras.main.position[2]));
			}
			appState->globals.viewportMousePosX = ImGui::GetIO().MousePos.x - viewportOffset.x;
			appState->globals.viewportMousePosY = ImGui::GetIO().MousePos.y - viewportOffset.y;

		}
		ImVec2 wsize = ImGui::GetWindowSize();
		appState->globals.viewportSize[0] = wsize.x;
		appState->globals.viewportSize[1] = wsize.y;
		if (appState->states.textureBake)
			ImGui::Image((ImTextureID)appState->frameBuffers.textureExport->GetColorTexture(), wsize, ImVec2(0, 1), ImVec2(1, 0));
		else if (appState->states.postProcess)
			ImGui::Image((ImTextureID)appState->frameBuffers.postProcess->GetColorTexture(), wsize, ImVec2(0, 1), ImVec2(1, 0));
		else
			ImGui::Image((ImTextureID)GetViewportFramebufferColorTextureId(), wsize, ImVec2(0, 1), ImVec2(1, 0));
		ImGui::EndChild();
	}
	ImGui::End();
}

static void SaveFile(std::string file = ShowSaveFileDialog()) {
	appState->serailizer->SaveFile(file);

	//data["EnodeEditor"] = GetMeshNodeEditorSaveData();

//	data["noiseLayers"] = noiseGen->Save();
	
}

static void OpenSaveFile(std::string file = ShowOpenFileDialog(".terr3d")) {

	appState->serailizer->LoadFile(file);

	//noiseGen->Load(data["noiseLayers"]);
	//SetMeshNodeEditorSaveData(data["EnodeEditor"]);

}


static void PackProject(std::string path = ShowSaveFileDialog()) {
	appState->serailizer->PackProject(path);
}

static void LoadPackedProject(std::string path = ShowOpenFileDialog()) {
	appState->serailizer->LoadPackedProject(path);
}


static void ShowModuleManager() {
	ImGui::Begin("Module Manager", &appState->windows.modulesManager);
	appState->modules.manager->Render();
	ImGui::End();
}

static void OnBeforeImGuiRender() {

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
		window_flags |= ImGuiWindowFlags_NoBackground;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockSpace", &dockspaceOpen, window_flags);
	ImGui::PopStyleVar();
	if (opt_fullscreen) {
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

static void OnImGuiRenderEnd() {
	ImGui::End();
}

static void SetUpIcon() {
#ifdef TERR3D_WIN32
	HWND hwnd = glfwGetWin32Window(mainApp->GetWindow()->GetNativeWindow());
	HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
	if (hIcon) {
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
	virtual void OnPreload() override {
		SetTitle("TerraForge3D - Jaysmito Mukherjee");
		MkDir(GetExecutableDir() + "\\Data\\logs");
		SetLogsDir(GetExecutableDir() + "\\Data\\logs");
		SetWindowConfigPath(GetExecutableDir() + "\\Data\\configs\\windowconfigs.terr3d");
		MkDir(GetExecutableDir() + "\\Data\\cache\\autosave\"");
		SetupOSLiscences();
	}

	virtual void OnUpdate(float deltatime) override
	{
		if (!appState->states.ruinning)
			return;

		if (!appState->states.exploreMode) {
			
			// CTRL Shortcuts
			if ((glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_LEFT_CONTROL) || glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_RIGHT_CONTROL))) {

				// Open Shortcut
				if (glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_O)) {
					OpenSaveFile();
				}

				// Exit Shortcut
				if (glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_Q)) {
					exit(0);
				}

				// Save Shortcut
				if (glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_S)) {
					if (appState->globals.currentOpenFilePath.size() > 3) {
						Log("Saved to " + appState->globals.currentOpenFilePath);
						SaveFile(appState->globals.currentOpenFilePath);
					}
					else {
						SaveFile();
					}
				}

				// Close Shortcut
				if (glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_W)) {
					if (appState->globals.currentOpenFilePath.size() > 3) {
						Log("CLosed file " + appState->globals.currentOpenFilePath);
						appState->globals.currentOpenFilePath = "";
					}
					else {
						Log("Shutting Down");
						exit(0);
					}
				}

				// CTRL + SHIFT Shortcuts
				if ((glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_LEFT_SHIFT) || glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_RIGHT_SHIFT))) {// Save Shortcut

					// Save As Shortcuts
					if (glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_S)) {
						appState->globals.currentOpenFilePath = "";
						SaveFile();
					}

					// Explorer Mode Shortcut
					if (glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_X)) {
						Log("Toggle Explorer Mode");
						appState->states.exploreMode = true;
					}

				}
			}

			appState->stats.deltatime = deltatime;

			if (appState->states.textureBake) {
				appState->frameBuffers.textureExport->Begin();
				glViewport(0, 0, 1024, 1024); // This should be 4096 instead of 1024 but my computer is too weak to go for 4096
				GetWindow()->Clear();
				DoTheRederThing(deltatime, false, true);
			}
			else {
				appState->frameBuffers.relflection->Begin();
				glViewport(0, 0, 800, 600);
				GetWindow()->Clear();
				DoTheRederThing(deltatime);

				glBindFramebuffer(GL_FRAMEBUFFER, GetViewportFramebufferId());
				glViewport(0, 0, 800, 600);
				GetWindow()->Clear();
				DoTheRederThing(deltatime, true);

				if (appState->states.postProcess)
				{
					appState->frameBuffers.postProcess->Begin();
					GetWindow()->Clear();
					PostProcess(deltatime);
				}
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			RenderImGui();

			if (appState->states.autoUpdate)
				RegenerateMesh();

		}
		else {
			static bool expH = false;
			if (!expH) {
				GetWindow()->SetFullScreen(true);
				glfwSetInputMode(GetWindow()->GetNativeWindow(), GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
				ExPSaveCamera(appState->cameras.main.position, appState->cameras.main.rotation);
			}


			expH = appState->states.exploreMode;
			appState->stats.deltatime = deltatime;

			appState->frameBuffers.relflection->Begin();
			glViewport(0, 0, 800, 600);
			GetWindow()->Clear();
			DoTheRederThing(deltatime);
			appState->frameBuffers.relflection->End();

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			int w, h;
			glfwGetWindowSize(GetWindow()->GetNativeWindow(), &w, &h);
			glViewport(0, 0, w, h);
			GetWindow()->Clear();
			UpdateExplorerControls(appState->cameras.main.position, appState->cameras.main.rotation, appState->states.iExploreMode, &appState->globals.offset[0], &appState->globals.offset[1]);
			DoTheRederThing(deltatime, true);

			if (appState->states.postProcess)
			{
				appState->frameBuffers.postProcess->Begin();
				PostProcess(deltatime);
			}

			if ((glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_ESCAPE))) {
				appState->states.exploreMode = false;
				expH = false;
				glfwSetInputMode(GetWindow()->GetNativeWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				GetWindow()->SetFullScreen(false);
				ExPRestoreCamera(appState->cameras.main.position, appState->cameras.main.rotation);
			}


			if (appState->states.autoUpdate)
				RegenerateMesh();

		}


		if (glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_I))
			appState->globals.offset[0] -= 0.01f;

		if (glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_K))
			appState->globals.offset[0] += 0.01f;

		if (glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_L))
			appState->globals.offset[1] -= 0.01f;

		if (glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_J))
			appState->globals.offset[1] += 0.01f;


		if (ReqRefresh())
			ResetShader();
	}

	virtual void OnOneSecondTick() override
	{
		if (!appState->states.ruinning)
			return;

		appState->globals.secondCounter++;

		if (appState->globals.secondCounter % 5 == 0) {
			if (appState->states.autoSave) {
				SaveFile(GetExecutableDir() + "\\Data\\cache\\autosave\\autosave.terr3d");

				if (appState->globals.currentOpenFilePath.size() > 3) {
					SaveFile(appState->globals.currentOpenFilePath);
				}
			}
		}

		GetWindow()->SetVSync(appState->states.vSync);
		appState->stats.frameRate = 1 / appState->stats.deltatime;
		SecondlyShaderEditorUpdate();
		UpdateTextureStore();
		TextureSettingsTick();
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
		
		if (appState->states.autoAspectCalcRatio)
		{
			appState->cameras.main.aspect = appState->globals.viewportSize[0] / appState->globals.viewportSize[1];
		}

		appState->lightManager->ShowSettings(true, &appState->windows.lightControls);

		appState->meshGenerator->ShowSettings();
		
		ShowTerrainControls();



		ShowMainScene();

		// Optional Windows

		if (appState->windows.statsWindow)
			ShowStats();

		if (appState->windows.texturEditorWindow)
			ShowTextureSettings(&appState->windows.texturEditorWindow);

		appState->seaManager->ShowSettings(&appState->windows.seaEditor);

		if (appState->windows.modulesManager)
			ShowModuleManager();

		if (appState->windows.styleEditor)
			ShowStyleEditor(&appState->windows.styleEditor);

		if (appState->windows.foliageManager)
			ShowFoliageManager(&appState->windows.foliageManager);

		if (appState->windows.shaderEditorWindow)
			ShowShaderEditor(&appState->windows.shaderEditorWindow);

		if (appState->windows.textureStore)
			ShowTextureStore(&appState->windows.textureStore);

		if (appState->windows.filtersManager)
			ShowFiltersMamager(&appState->windows.filtersManager);

		if (appState->windows.osLisc)
			ShowOSLiscences(&appState->windows.osLisc);

		if (appState->windows.supportersTribute)
			ShowSupportersTribute(&appState->windows.supportersTribute);

		if (appState->windows.skySettings)
			ShowSkySettings(&appState->windows.skySettings);

		for (UIModule* mod : appState->modules.manager->uiModules)
		{
			if (mod->active)
			{
				ImGui::Begin(mod->windowName.c_str(), &mod->active);
				mod->Render();
				ImGui::End();
			}
		}

		for (NoiseLayerModule* mod : appState->modules.manager->nlModules)
		{
			if (mod->active)
			{
				ImGui::Begin(mod->name.c_str());
				mod->Render();
				ImGui::End();
			}
		}

		OnImGuiRenderEnd();
	}

	virtual void OnStart(std::string loadFile) override
	{
		// Set random generator seed from current time
		srand((unsigned int)time(NULL));


		// Setup custom icon for the Main Window
		SetUpIcon();
		
		SetUpApplicationState();
		appState = GetApplicationState();
		appState->mainApp = mainApp;

		SetupViewportFrameBuffer();
		SetupShaderManager(GetExecutableDir());
		SetupFoliageManager();
		SetupSupportersTribute();
		SetupExplorerControls();
		SetupTextureStore(GetExecutableDir(), &appState->states.reqTexRfrsh);

		ImGui::GetStyle().WindowMenuButtonPosition = ImGuiDir_None;
		
		LoadDefaultStyle();
		
		GetWindow()->SetShouldCloseCallback(OnAppClose);
		glfwSetFramebufferSizeCallback(GetWindow()->GetNativeWindow(), [](GLFWwindow* window, int w, int h) {
			glfwSwapBuffers(window);
			});
		glfwSetScrollCallback(GetWindow()->GetNativeWindow(), [](GLFWwindow*, double x, double y) {
			ImGuiIO& io = ImGui::GetIO();
			io.MouseWheel = (float)y;
			});
		
		GetWindow()->SetClearColor({ 0.1f, 0.1f, 0.1f });



		appState->globals.kernelsIncludeDir = "\"" + GetExecutableDir() + "\\Data\\kernels" + "\"";

		appState->modules.manager = new ModuleManager();
		appState->meshGenerator = new MeshGeneratorManager(appState);
		appState->mainMenu = new MainMenu(appState);

		appState->seaManager = new SeaManager();
		appState->lightManager = new LightManager();
		appState->serailizer = new Serializer(appState);

		ResetShader();
		appState->meshGenerator->GenerateSync();

		appState->models.coreTerrain->SetupMeshOnGPU();

		appState->models.screenQuad->SetupMeshOnGPU();
		appState->models.screenQuad->mesh->GenerateScreenQuad();
		appState->models.screenQuad->mesh->RecalculateNormals();
		appState->models.screenQuad->UploadToGPU();

		glEnable(GL_DEPTH_TEST);
		appState->globals.scale = 1;

		if (loadFile.size() > 0) {
			Log("Loading File from " + loadFile);
			OpenSaveFile(loadFile);
		}

		SetProjectId(GenerateId(32));

		SetupFiltersManager(&appState->states.autoUpdate, appState->models.coreTerrain);

		float t = 1.0f;
		SetupTextureSettings(&appState->states.reqTexRfrsh, &t, appState->models.coreTerrain);
		SetupSky();

		// Load Fonts

		LoadUIFont("Open-Sans-Regular", 18, GetExecutableDir() + "\\Data\\fonts\\OpenSans-Regular.ttf");
		LoadUIFont("OpenSans-Bold", 25, GetExecutableDir() + "\\Data\\fonts\\OpenSans-Bold.ttf");


		appState->frameBuffers.relflection = new FrameBuffer();
		appState->frameBuffers.textureExport = new FrameBuffer(1024, 1024); // This should be 4096 instead of 1024 but my computer is too weak to go for 4096
		LoadTextureThumbs();

		bool tpp = false;

		appState->frameBuffers.postProcess = new FrameBuffer(800, 600);
		appState->shaders.postProcess = new Shader(ReadShaderSourceFile(GetExecutableDir() + "\\Data\\shaders\\post_processing\\vert.glsl", &tpp), ReadShaderSourceFile(GetExecutableDir() + "\\Data\\shaders\\post_processing\\frag.glsl", &tpp));

		// For Debug Only
		appState->states.autoUpdate = true;


		Log("Started Up App!");
	}

	void OnEnd()
	{
		while (appState->states.remeshing);

		using namespace std::chrono_literals;
		std::this_thread::sleep_for(500ms);
		delete appState->shaders.terrain;
		delete appState->shaders.foliage;
		delete appState->shaders.textureBake;
		delete appState->shaders.wireframe;
		delete appState->shaders.postProcess;

		delete appState->mainMenu;

//		delete appState->frameBuffers.main;
		delete appState->frameBuffers.postProcess;
		delete appState->frameBuffers.relflection;
		delete appState->frameBuffers.textureExport;

//		delete appState->seaManager;
		delete appState->lightManager;
		delete appState->serailizer;
		DeleteApplicationState();

//		exit(0); // Temporary
	}
};

Application* CreateApplication()
{
	mainApp = new MyApp();
	return mainApp;
}