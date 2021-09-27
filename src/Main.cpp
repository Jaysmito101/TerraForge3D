#define _CRT_SECURE_NO_WARNINGS
#include "../resource.h"
#include <Base.h>
#include <Base/EntryPoint.h>
#include <ImGuiConsole.h>
#include <AppStyles.h>
#include <ViewportFramebuffer.h>
#include <AppShaderEditor.h>
#include <ElevationNodeEditor.h>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <windows.h>
#include <string>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_relational.hpp>
#include <glm/ext/vector_relational.hpp>
#include <glm/ext/scalar_relational.hpp>
#include <SimplexNoise.h>
#include <FastNoiseLite.h>
#include <thread>
#include <CubeMap.h>
#include <time.h>
#include <mutex>
#include <json.hpp>
#include <atomic>

#include <Utils.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>



#include <AppStructs.h>

static Model terrain("Terrain");

static Application *myApp;
static Shader *shd, *meshNormalsShader, *wireframeShader;
static Camera camera;
static Stats s_Stats;
static ActiveWindows activeWindows;
static std::vector<NoiseLayer> noiseLayers;
static const char* noiseTypes[] = {"Simplex Perlin", "Random", "Cellular"};
static FastNoiseLite m_NoiseGen;

static glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
static glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
static glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
static glm::vec3 LightPosition = glm::vec3(0.0f);
static float LightColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
static float CameraPosition[3];
static float CameraRotation[3];
static Vec2 prevMousePos;
static char b1[4096], b2[4096];

static bool skyboxEnabled = false;
static bool vSync = true;
static bool autoUpdate = false;
static bool flattenBase = false;
static bool button1, button2, button3;
static std::atomic<bool> noiseBased = false;
static bool wireFrameMode = false;
static bool absolute = false;
static bool square = false;
static std::atomic<bool> isRemeshing = false;



static uint32_t vao, vbo, ebo;

static float noiseStrength = 1.0f;
static int resolution = 256;
static float mouseSpeed = 25;
static float scrollSpeed = 0.5f;
static float mouseScrollAmount = 0;
static float scale = 1.0f;
static int numberOfNoiseTypes = 3;
std::string fileName = "";
static nlohmann::json appData;


static void ToggleSystemConsole() {
	static bool state = false;
	state = !state;
	ShowWindow(GetConsoleWindow(), state?SW_SHOW:SW_HIDE);
}



void OnAppClose(int x, int y)
{
	Log("Close App");
	myApp->Close();
}


static void ShowStats() 
{
	ImGui::Begin("Statistics", &activeWindows.statsWindow);

	// Frame Rate
	{
		if (s_Stats.deltaTime == 0)
			s_Stats.deltaTime = 1;
		memset(b1, 0, 100);
		memset(b2, 0, 100);
		_itoa_s((int)s_Stats.frameRate, b2, 10);
		strcat_s(b1, "FPS         : ");
		strcat_s(b1, b2);
		ImGui::Text(b1);
		
		memset(b1, 0, 100);
		memset(b2, 0, 100);
		_gcvt_s(b2, s_Stats.deltaTime*1000, 6);
		strcat_s(b1, "Frame Time  : ");
		strcat_s(b1, b2);
		strcat_s(b1, "ms");
		ImGui::Text(b1);

		memset(b1, 0, 100);
		memset(b2, 0, 100);
		_itoa_s(s_Stats.triangles, b2, 10);
		strcat_s(b1, "Triangles  : ");
		strcat_s(b1, b2);
		ImGui::Text(b1);

		memset(b1, 0, 100);
		memset(b2, 0, 100);
		_itoa_s(s_Stats.vertCount, b2, 10);
		strcat_s(b1, "Vertices  : ");
		strcat_s(b1, b2);
		ImGui::Text(b1);
	}

	ImGui::End();
}

static void ShowGeneralControls() 
{
	ImGui::Begin("General Controls");

	{
		ImGui::Checkbox("VSync ", &vSync);
	}

	ImGui::DragFloat("Mouse Speed", &mouseSpeed);
	ImGui::DragFloat("Zoom Speed", &scrollSpeed);

	if (ImGui::Button("Exit")) {
		exit(0);
	}

	ImGui::End();
}

static void ShowCameraControls() {
	ImGui::Begin("Camera Controls");

	ImGui::Text("Camera Position");
	ImGui::DragFloat3("##cameraPosition", CameraPosition, 0.1f);
	ImGui::Separator();
	ImGui::Separator();
	ImGui::Text("Camera Rotation");
	ImGui::DragFloat3("##cameraRotation", CameraRotation, 10);
	ImGui::Separator();
	ImGui::Separator();
	ImGui::Text("Projection Settings");
	ImGui::Separator();
	ImGui::DragFloat("FOV", &(camera.fov), 0.01f);
	ImGui::DragFloat("Aspect Ratio", &(camera.aspect), 0.01f);
	ImGui::DragFloat("Near Clipping", &(camera.near), 0.01f);
	ImGui::DragFloat("Far Clipping", &(camera.far), 0.01f);

	ImGui::End();
}

static void ShowLightingControls() {
	ImGui::Begin("Light Controls");

	ImGui::Text("Light Position");
	ImGui::DragFloat3("##lightPosition", &LightPosition[0], 0.1f);
	ImGui::Separator();
	ImGui::Separator();
	ImGui::Text("Light Color");
	ImGui::ColorPicker4("##lightColor", LightColor);

	ImGui::End();
}


static std::string ExportOBJ() {
	std::string fileName = openfilename();

	std::ofstream outfile;

	if (fileName.find(".obj") == std::string::npos)
		fileName += ".obj";
	
	outfile.open(fileName);


	outfile << "# TerraGenV1.0 OBJ" << std::endl << std::endl;;

	for (int i = 0; i < terrain.mesh.vertexCount; i++) 
	{
		outfile << "v " << terrain.mesh.vert[i].position.x << " " << terrain.mesh.vert[i].position.y << " " << terrain.mesh.vert[i].position.z << " "  << std::endl;
	}

	outfile << std::endl;
	outfile << std::endl;

	for (int i = 0; i < terrain.mesh.indexCount; i += 3)
	{
		outfile << "f " << terrain.mesh.indices[i] + 1 << " " << terrain.mesh.indices[i + 1] + 1 << " " << terrain.mesh.indices[i + 2] + 1 << " " << std::endl;
	}

	outfile.close();

	Log((std::string("Exported Mesh to ") + fileName).c_str());

	return fileName;

}

static float EvaluateNoiseLayer(float x, float y, NoiseLayer& noiseLayer) {
	if (!noiseLayer.enabled)
		return 0.0f;
	float baseNoise = 0;
	if (strcmp(noiseLayer.noiseType, "Simplex Perlin") == 0) {
		baseNoise = SimplexNoise::noise((x/resolution) * noiseLayer.scale + noiseLayer.offsetX, (y/ resolution) * noiseLayer.scale + noiseLayer.offsetY) * noiseLayer.strength;
	}
	else if (strcmp(noiseLayer.noiseType, "Random") == 0) {		
		return (rand())*noiseLayer.strength*0.0001f;
	}
	else if (strcmp(noiseLayer.noiseType, "Cellular") == 0) {
		m_NoiseGen.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
		baseNoise = m_NoiseGen.GetNoise((x / 1) * noiseLayer.scale + noiseLayer.offsetX, (y / 1)* noiseLayer.scale + noiseLayer.offsetY) * noiseLayer.strength;
	}
	return baseNoise;
}

static float noise(float x, float y) {
	float noise = 0;
	for (NoiseLayer& nl : noiseLayers) {
		noise += EvaluateNoiseLayer(x, y, nl);
	}
	if (flattenBase) {
		noise = MAX(0, noise);
	}
	if (absolute) {
		noise = abs(noise);
	}
	if (square) {
		noise *= noise;
	}
	return noise * noiseStrength;
}

/*
static void GenerateVertices()
{
	Vert* vertices = new Vert[resolution * resolution];
	int* inds = new int[(resolution-1) * (resolution-1) * 6];
	int triIndex = 0;

	for (int y = 0; y < resolution; y++)
	{
		for (int x = 0; x < resolution; x++)
		{
			int i = x + y * resolution;
			glm::vec2 percent = glm::vec2(x, y) / ((float)resolution - 1);
			glm::vec3 pointOnPlane = (percent.x - .5f) * 2 * right + (percent.y - .5f) * 2 * front;
			pointOnPlane *= scale;
			vertices[i] = Vert();
			vertices[i].position = glm::vec3(0.0f);

			vertices[i].position.x = (float)pointOnPlane.x;
			if(noiseBased)
				vertices[i].position.y = (float)(pointOnPlane.y + noise((float)x, (float)y));
			else
				vertices[i].position.y = (float)(pointOnPlane.y + GetElevation((float)x, (float)y));
			vertices[i].position.z = (float)pointOnPlane.z;
			vertices[i].normal = glm::vec3(0.0f);
			if (x != resolution - 1 && y != resolution - 1)
			{
				inds[triIndex] = i;
				inds[triIndex + 1] = i + resolution + 1;
				inds[triIndex + 2] = i + resolution;

				inds[triIndex + 3] = i;
				inds[triIndex + 4] = i + 1;
				inds[triIndex + 5] = i + resolution + 1;
				triIndex += 6;
			}
		}
	}
	
	if (mesh.vert)
		delete mesh.vert;
	if (mesh.indices)
		delete mesh.indices;
	mesh.vertexCount = resolution * resolution;
	mesh.vert = new Vert[resolution * resolution];
	memset(mesh.vert, 0, sizeof(Vert) * mesh.vertexCount);
	memcpy(mesh.vert, vertices, sizeof(Vert)*mesh.vertexCount);
	mesh.indexCount = (resolution - 1) * (resolution - 1) * 6;
	mesh.indices = new int[(resolution - 1) * (resolution - 1) * 6];
	memset(mesh.indices, 0, mesh.indexCount);
	memcpy(mesh.indices, inds, sizeof(int) * mesh.indexCount);
	delete vertices;
	delete inds;
	RecalculateNormals(mesh);
	s_Stats.vertCount = resolution * resolution;
	s_Stats.triangles = mesh.indexCount / 3;
}
*/

static void DeleteBuffers(GLuint a, GLuint b, GLuint c) {
	glDeleteBuffers(1, &a);
	glDeleteBuffers(1, &b);
	glDeleteBuffers(1, &c);
}

static void ResetShader() {
	if (shd)
		delete shd;
	if(!wireframeShader)
		wireframeShader = new Shader(GetVertexShaderSource(), GetFragmentShaderSource(), GetWireframeGeometryShaderSource());
	shd = new Shader(GetVertexShaderSource(), GetFragmentShaderSource(), GetGeometryShaderSource());
}

static void FillMeshData() {

	if (terrain.mesh.res == resolution && terrain.mesh.sc == scale) {
		for (int y = 0; y < resolution; y++)
		{
			for (int x = 0; x < resolution; x++)
			{
				if(noiseBased)
					terrain.mesh.SetElevation(noise(x, y), x, y);
				else
					terrain.mesh.SetElevation(GetElevation(x, y), x, y);
			}
		}
		terrain.mesh.RecalculateNormals();
	}
	else {
		terrain.mesh.GeneratePlane(resolution, scale);
		s_Stats.triangles = terrain.mesh.indexCount / 3;
		s_Stats.vertCount = terrain.mesh.vertexCount;
		for (int y = 0; y < resolution; y++)
		{
			for (int x = 0; x < resolution; x++)
			{
				terrain.mesh.SetElevation(noise(x, y), x, y);
			}
		}
		terrain.mesh.RecalculateNormals();
	}
	isRemeshing = false;
}

static void UploadMeshToGPU() {
	if (!terrain.mesh.IsValid())
		return;

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vert) * terrain.mesh.vertexCount, terrain.mesh.vert, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * terrain.mesh.indexCount, terrain.mesh.indices, GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), (void*)offsetof(Vert, position));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), (void*)offsetof(Vert, normal));
	glEnableVertexAttribArray(1);

}

static void RegenerateMesh(){

	if (isRemeshing)
		return;

	UploadMeshToGPU();

	isRemeshing = true;
	
	std::thread worker(FillMeshData);
	worker.detach();


}

static void GenerateMesh() 
{
	FillMeshData();

	ResetShader();

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vert) * terrain.mesh.vertexCount, terrain.mesh.vert, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * terrain.mesh.indexCount, terrain.mesh.indices, GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), (void*)offsetof(Vert, position));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), (void*)offsetof(Vert, normal));
	glEnableVertexAttribArray(1);

}

static void DoTheRederThing(float deltaTime) {
	static float time;
	time += deltaTime;
	camera.UpdateCamera(CameraPosition, CameraRotation);
	Shader* shader;
	if(skyboxEnabled)
		RenderSkybox(camera.pv);
	if (wireFrameMode)
		shader = wireframeShader;
	else
		shader = shd;
	shader->Bind();
	shader->SetTime(&time);
	shader->SetMPV(camera.pv);
	shader->SetUniformMAt4("_Model", terrain.modelMatrix);
	shader->SetLightCol(LightColor);
	shader->SetLightPos(LightPosition);
	glBindVertexArray(vao);
	if (wireFrameMode) {
		glDrawElements(GL_LINE_STRIP, terrain.mesh.indexCount, GL_UNSIGNED_INT, 0);
	}
	else {
		glDrawElements(GL_TRIANGLES, terrain.mesh.indexCount, GL_UNSIGNED_INT, 0);
	}
	glBindVertexArray(0);
}

static void ShowTerrainControls()
{
	static bool exp = false;
	ImGui::Begin("Dashboard");


	ImGui::DragInt("Mesh Resolution", &resolution, 1, 2, 8192);
	ImGui::DragFloat("Mesh Scale", &scale, 0.1f, 1.0f, 5000.0f);
	ImGui::NewLine();
	ImGui::Checkbox("Auto Update", &autoUpdate);
	ImGui::Checkbox("Wireframe Mode", &wireFrameMode);
	ImGui::Checkbox("Use Skybox", &skyboxEnabled);
	ImGui::NewLine();
	if(ImGui::Button("Update Mesh"))
		RegenerateMesh();

	if (ImGui::Button("Refresh Shaders")) {
		ResetShader();
	}
	ImGui::Separator();
	
	if (ImGui::Button("Use Custom Shaders")) {
		activeWindows.shaderEditorWindow = true;
	}

	ImGui::Separator();

	if (ImGui::Button("Change Mode##4584")) {
		noiseBased = !noiseBased;
	}

	if (noiseBased) {
		ImGui::Text("Using Layer Based Workflow");
	}
	else {
		ImGui::Text("Using Node Based Workflow");
	}

	ImGui::Separator();
	ImGui::End();
}

static void ShowNoiseLayer(NoiseLayer& noiseLayer, int id) {
	bool state = ImGui::CollapsingHeader((std::string("##noiseLayerName") + std::to_string(id)).c_str());
	ImGui::SameLine();
	ImGui::Text(noiseLayer.name);
	if (state) {
		ImGui::InputText((std::string("##") + std::to_string(id)).c_str(), (noiseLayer.name), 256);
			ImGui::Checkbox((std::string("Enabled##") + std::to_string(id)).c_str(), &(noiseLayer.enabled));
			ImGui::Text("Noise Type");
			if (ImGui::BeginCombo((std::string("##noiseType") + std::to_string(id)).c_str(), noiseLayer.noiseType))
			{
				for (int i = 0; i < numberOfNoiseTypes; i++)
				{
					bool is_selected = (noiseLayer.noiseType == noiseTypes[i]);
						if (ImGui::Selectable(noiseTypes[i], is_selected)) {
							noiseLayer.noiseType = noiseTypes[i];
								if (is_selected)
									ImGui::SetItemDefaultFocus();
						}
				}
				ImGui::EndCombo();
			}
		ImGui::DragFloat((std::string("Scale##") + std::to_string(id)).c_str(), &(noiseLayer.scale), 0.01f, -200.0f, 200.0f);
		ImGui::DragFloat((std::string("Strength##") + std::to_string(id)).c_str(), &(noiseLayer.strength), 0.001f);
		ImGui::DragFloat((std::string("Offset X##") + std::to_string(id)).c_str(), &(noiseLayer.offsetX), 0.01f);
		ImGui::DragFloat((std::string("Offset Y##") + std::to_string(id)).c_str(), &(noiseLayer.offsetY), 0.01f);
		if (ImGui::Button((std::string("Remove") + std::string("##noiseLayerName") + std::to_string(id)).c_str())) {
			noiseLayers.erase(noiseLayers.begin() + id);
		}
		ImGui::SameLine();
		if (ImGui::Button((std::string("Duplicate") + std::string("##noiseLayerName") + std::to_string(id)).c_str())) {
			Log(std::to_string(id).c_str());
			noiseLayers.push_back(noiseLayers[id]);
		}
	}
}

static void ShowNoiseSettings(){
	ImGui::Begin("Noise Settings");

	ImGui::DragFloat("Noise Strength", &noiseStrength, 0.005f);
	ImGui::Checkbox("Flatten Base", &flattenBase);
	ImGui::Checkbox("Absoulute Value", &absolute);
	ImGui::Checkbox("Square Value", &square);

	ImGui::Separator();

	ImGui::Text("Noise Layers");
	ImGui::Separator();
	for (unsigned int i = 0; i < noiseLayers.size();i++) {
		ShowNoiseLayer(noiseLayers[i], i);
		ImGui::Separator();
	}
	if (ImGui::Button("Add Noise Layer")) {
		noiseLayers.push_back(NoiseLayer());
	}
	ImGui::End();
}

static void MouseMoveCallback(float x, float y) {
	if (button2) {

		float deltaX = x - prevMousePos.x;
		float deltaY = y - prevMousePos.y;

		if (deltaX > 200) {
			deltaX = 0;
		}

		if (deltaY > 200) {
			deltaY = 0;
		}

		CameraRotation[0] += deltaX * mouseSpeed;
		CameraRotation[1] += deltaY * mouseSpeed;
	}

	prevMousePos.x = x;
	prevMousePos.y = y;
	
}

static void MouseScrollCallback(float amount){
	mouseScrollAmount = amount;
	glm::vec3 pPos = glm::vec3(CameraPosition[0], CameraPosition[1], CameraPosition[2]);
	pPos += front * amount * scrollSpeed;
	CameraPosition[0] = pPos.x;
	CameraPosition[1] = pPos.y;
	CameraPosition[2] = pPos.z;
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
			button1 = io.MouseDown[0];
			button2 = io.MouseDown[2];
			button3 = io.MouseDown[1];
		}
		ImVec2 wsize = ImGui::GetWindowSize();
		ImGui::Image((ImTextureID)GetViewportFramebufferColorTextureId(), wsize, ImVec2(0, 1), ImVec2(1, 0));
		ImGui::EndChild();
	}
	ImGui::End();
}

static void SaveFile() {
	std::string file = ShowSaveFileDialog();
	if (file.size() == 0)
		return;
	if (file.find(".terr3d") == std::string::npos)
		file += ".terr3d";

	nlohmann::json data;
	data["type"] = "SAVEFILE";
	data["version"] = "3.0";
	data["name"] = "TerraGen3D v3.0";
	data["EnodeEditor"] = GetElevationNodeEditorSaveData();
	data["styleData"] = GetStyleData();
	data["appData"] = appData;
	data["imguiData"] = std::string(ImGui::SaveIniSettingsToMemory());

	nlohmann::json tmp;
	tmp["autoUpdate"] = autoUpdate;
	tmp["square"] = square;
	tmp["absolute"] = absolute;
	tmp["flattenBase"] = flattenBase;
	tmp["noiseBased"] = noiseBased?true:false;
	tmp["wireFrameMode"] = wireFrameMode;
	tmp["skyboxEnabled"] = skyboxEnabled;
	tmp["vSync"] = vSync;
	tmp["mouseSpeed"] = mouseSpeed;
	tmp["scrollSpeed"] = scrollSpeed;
	tmp["mouseScrollAmount"] = mouseScrollAmount;
	tmp["scale"] = scale;

	data["generals"] = tmp;

	std::ofstream outfile;
	outfile.open(file);

	outfile << data;

	outfile.close();
}

static void OpenSaveFile() {

	// For Now it dows not do anything id any error has occured but in later versions this will be reported to user!

	std::string file = ShowOpenFileDialog(".terr3d\0");
	if (file.size() == 0)
		return;
	if (file.find(".terr3d") == std::string::npos)
		file += ".terr3d";
	bool flagRd = true;
	std::string sdata = ReadShaderSourceFile(file, &flagRd);
	if (!flagRd)
		return;
	if (sdata.size() == 0)
		return;
	nlohmann::json data = nlohmann::json::parse(sdata);
	if (data["type"] != "SAVEFILE")
		return;
	
	SetElevationNodeEditorSaveData(data["EnodeEditor"]);
	LoadThemeFromStr(data["styleData"]);
	data["imguiData"] = ImGui::SaveIniSettingsToMemory();
	appData = data["appData"];
	nlohmann::json tmp = data["generals"];
	autoUpdate = tmp["autoUpdate"];
	square = tmp["square"];
	absolute = tmp["absolute"];
	flattenBase = tmp["flattenBase"];
	noiseBased = tmp["noiseBased"];
	skyboxEnabled = tmp["skyboxEnabled"];
	vSync = tmp["vSync"];
	wireFrameMode = tmp["wireFrameMode"];
	mouseSpeed = tmp["mouseSpeed"];
	scrollSpeed = tmp["scrollSpeed"];
	mouseScrollAmount = tmp["mouseScrollAmount"];
	scale = tmp["scale"];


	// For Future
	// ImGui::LoadIniSettingsFromMemory(data["imguiData"].dump().c_str(), data["imguiData"].dump().size());
	
	std::ofstream outfile;
	outfile.open(file);

	outfile << data;

	outfile.close();
}

static void ShowWindowMenuItem(const char* title, bool* val) {
	ImGui::Checkbox(title, val);
}

static void ShowMenu() {
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Open")) {
				OpenSaveFile();
			}

			if (ImGui::MenuItem("Save")) {
				SaveFile();
			}


			if (ImGui::BeginMenu("Export As")) {
				if (ImGui::MenuItem("Export as OBJ")) {
					fileName = ExportOBJ();
				}
				ImGui::EndMenu();
			}

			if (ImGui::MenuItem("Exit")) {
				exit(0);
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Options"))
		{
			if (ImGui::MenuItem("Toggle System Console")) {
				ToggleSystemConsole();
			}

			if (ImGui::BeginMenu("Themes")) {
				if (ImGui::MenuItem("Default")) {
					LoadDefaultStyle();
				}

				if (ImGui::MenuItem("Black & White")) {
					LoadBlackAndWhite();
				}

				if (ImGui::MenuItem("Cool Dark")) {
					LoadDarkCoolStyle();
				}

				if (ImGui::MenuItem("Light Orange")) {
					LoadLightOrngeStyle();
				}

				if (ImGui::MenuItem("Load Theme From File")) {
					LoadThemeFromFile(openfilename());
				}
				ImGui::EndMenu();
			}

			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Widnows"))
		{
			ShowWindowMenuItem("Statistics", &activeWindows.statsWindow);

			ShowWindowMenuItem("Theme Editor", &activeWindows.styleEditor);

			ShowWindowMenuItem("Shader Editor", &activeWindows.shaderEditorWindow);

			ShowWindowMenuItem("Elevation Node Editor", &activeWindows.elevationNodeEditorWindow);
			
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
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
	ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
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

	ShowMenu();
}

static void OnImGuiRenderEnd() {
	ImGui::End();
}

static void ResizeTexture(uint32_t tex, int w, int h) {
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
}

static void SetUpIcon() {
	HWND hwnd = glfwGetWin32Window(myApp->GetWindow()->GetNativeWindow());
	HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
	if (hIcon) {
		//Change both icons to the same icon handle.
		SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

		//This will ensure that the application icon gets changed too.
		SendMessage(GetWindow(hwnd, GW_OWNER), WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		SendMessage(GetWindow(hwnd, GW_OWNER), WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	}
}


class MyApp : public Application 
{
public:
	virtual void OnPreload() override {
		SetTitle("TerraGen3D - Jaysmito Mukherjee");
		
		Sleep(1000);
	}

	virtual void OnUpdate(float deltatime) override
	{
		glBindFramebuffer(GL_FRAMEBUFFER, GetViewportFramebufferId());
		glViewport(0, 0, 800, 600);
		GetWindow()->Clear();
		s_Stats.deltaTime = deltatime;
		DoTheRederThing(deltatime);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		RenderImGui();
		if (autoUpdate)
			RegenerateMesh();

		if (ReqRefresh())
			ResetShader();
	}

	virtual void OnOneSecondTick() override 
	{
		GetWindow()->SetVSync(vSync);
		s_Stats.frameRate = 1 / s_Stats.deltaTime;
		ElevationNodeEditorTick();
		SecondlyShaderEditorUpdate();
	}

	virtual void OnImGuiRender() override
	{
		OnBeforeImGuiRender();
		ShowGeneralControls();
		ShowCameraControls();
		ShowTerrainControls();
		ShowLightingControls();
		ShowNoiseSettings();
		ShowMainScene();

		// Optional Windows

		if (activeWindows.statsWindow)
			ShowStats();


		if (activeWindows.styleEditor)
			ShowStyleEditor(&activeWindows.styleEditor);

		if (activeWindows.shaderEditorWindow)
			ShowShaderEditor(&activeWindows.shaderEditorWindow);

		if (activeWindows.elevationNodeEditorWindow)
			ShowElevationNodeEditor(&activeWindows.elevationNodeEditorWindow);


		OnImGuiRenderEnd();
	}

	virtual void OnStart() override
	{
		SetUpIcon();
		srand((unsigned int)time(NULL));
		SetupViewportFrameBuffer();
		SetupCubemap();
		SetupShaderManager();
		SetupElevationManager(&resolution);
		ImGui::GetStyle().WindowMenuButtonPosition = ImGuiDir_None;
		LoadDefaultStyle();
		m_NoiseGen = FastNoiseLite::FastNoiseLite();
		GetWindow()->SetShouldCloseCallback(OnAppClose);
		glfwSetFramebufferSizeCallback(GetWindow()->GetNativeWindow(), [] (GLFWwindow* window, int w, int h){
			glfwSwapBuffers(window);
			});
		glfwSetScrollCallback(GetWindow()->GetNativeWindow(), [](GLFWwindow*, double x, double y) {
			ImGuiIO& io = ImGui::GetIO();
			io.MouseWheel = (float)y;
			});
		GetWindow()->SetClearColor({0.1f, 0.1f, 0.1f});
		GenerateMesh();
		glEnable(GL_DEPTH_TEST);
		LightPosition[1] = -0.3f;
		CameraPosition[1] = 0.2f;
		CameraPosition[2] = 3.1f;
		CameraRotation[1] = 2530.0f;
		autoUpdate = false;
		scale = 1;
		noiseLayers.push_back(NoiseLayer());
		Log("Started Up App!");

		// For Debug Only
		autoUpdate = true;
	}

	void OnEnd() 
	{
		ShutdownElevationNodeEditor();
		delete shd;
	}
};

Application* CreateApplication()
{
	myApp = new MyApp();
	return myApp;
}