#define _CRT_SECURE_NO_WARNINGS
#include "../resource.h"
#include <Base.h>
#include <Base/EntryPoint.h>
#include <ImGuiConsole.h>
#include <AppStyles.h>
#include <ModelImporter.h>
#include <Texture2D.h>
#include <ViewportFramebuffer.h>
#include <AppShaderEditor.h>
#include <OSLiscenses.h>
#include <FoliagePlacement.h>
#include <ElevationNodeEditor.h>
#include <ExportManager.h>
#include <TextureStore.h>
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
static Model sea("Sea");
static Model grid("Grid");

static Application* myApp;
static Shader* shd, * meshNormalsShader, * wireframeShader, * waterShader;
static Camera camera;
static Stats s_Stats;
static ActiveWindows activeWindows;
static std::vector<NoiseLayer> noiseLayers;
static std::vector<NoiseLayer> noiseLayersTmp;
static const char* noiseTypes[] = { "Simplex Perlin", "Random", "Cellular" };
static FastNoiseLite m_NoiseGen;

static glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
static glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
static glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
static glm::vec3 LightPosition = glm::vec3(0.0f);
static float LightColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
static float SeaColor[4] = { 0.4f, 0.4f, 1.0f, 1.0f };
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
static bool showSea = false;
static bool absolute = false;
static bool square = false;
static bool reqTexRfrsh = false;
static bool autoSave = true;
static bool showFoliage = true;
static std::atomic<bool> isRemeshing = false;
static std::atomic<bool> isRuinning = true;

static Texture2D* diffuse, * normal, * gridTex;


static uint32_t vao, vbo, ebo;

static float noiseStrength = 1.0f;
static int resolution = 256;
static float mouseSpeed = 25;
static float scrollSpeed = 0.5f;
static float mouseScrollAmount = 0;
static float seaLevel = 0.0f;
static float scale = 1.0f;
static float textureScale = 1.0f;
static float textureScaleO = 1.0f;
static float viewportMousePosX = 0;
static float viewportMousePosY = 0;
static int numberOfNoiseTypes = 3;
static int secondCounter = 0;

static nlohmann::json appData;

static std::string successMessage = "";
static std::string errorMessage = "";
static std::string savePath = "";

static void ToggleSystemConsole() {
	static bool state = false;
	state = !state;
	ShowWindow(GetConsoleWindow(), state ? SW_SHOW : SW_HIDE);
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
		_gcvt_s(b2, s_Stats.deltaTime * 1000, 6);
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
	ImGui::Begin("Sun Controls");

	ImGui::Text("Sun Position");
	ImGui::DragFloat3("##lightPosition", &LightPosition[0], 0.1f);
	ImGui::Separator();
	ImGui::Separator();
	ImGui::Text("Sun Light Color");
	ImGui::ColorEdit3("##lightColor", LightColor);

	ImGui::End();
}



static float EvaluateNoiseLayer(float x, float y, NoiseLayer& noiseLayer) {
	if (!noiseLayer.enabled)
		return 0.0f;
	float baseNoise = 0;
	try {
		if (strcmp(noiseLayer.noiseType, "Simplex Perlin") == 0) {
			baseNoise = SimplexNoise::noise((x / resolution) * noiseLayer.scale + noiseLayer.offsetX, (y / resolution) * noiseLayer.scale + noiseLayer.offsetY) * noiseLayer.strength;
		}
		else if (strcmp(noiseLayer.noiseType, "Random") == 0) {
			return (rand()) * noiseLayer.strength * 0.0001f;
		}
		else if (strcmp(noiseLayer.noiseType, "Cellular") == 0) {
			m_NoiseGen.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
			baseNoise = m_NoiseGen.GetNoise((x / 1) * noiseLayer.scale + noiseLayer.offsetX, (y / 1) * noiseLayer.scale + noiseLayer.offsetY) * noiseLayer.strength;
		}
	}
	catch (...) {
		baseNoise = 0.0f;
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

static void DeleteBuffers(GLuint a, GLuint b, GLuint c) {
	glDeleteBuffers(1, &a);
	glDeleteBuffers(1, &b);
	glDeleteBuffers(1, &c);
}

static void ResetShader() {
	bool res = false;
	if (shd)
		delete shd;
	if (!wireframeShader)
		wireframeShader = new Shader(GetDefaultVertexShaderSource(), GetDefaultFragmentShaderSource(), GetWireframeGeometryShaderSource());
	if (!waterShader)
		waterShader = new Shader(ReadShaderSourceFile(GetExecutableDir() + "\\Data\\shaders\\water\\vert.glsl", &res), ReadShaderSourceFile(GetExecutableDir() + "\\Data\\shaders\\water\\frag.glsl", &res), ReadShaderSourceFile(GetExecutableDir() + "\\Data\\shaders\\water\\geom.glsl", &res));
	shd = new Shader(GetVertexShaderSource(), GetFragmentShaderSource(), GetGeometryShaderSource());
}

static void FillMeshData() {

	if (terrain.mesh->res != resolution || terrain.mesh->sc != scale || textureScale != textureScaleO) {

		terrain.mesh->GeneratePlane(resolution, scale, textureScale);
		s_Stats.triangles = terrain.mesh->indexCount / 3;
		s_Stats.vertCount = terrain.mesh->vertexCount;
		textureScaleO = textureScale;
	}

	for (int y = 0; y < resolution; y++)
	{
		for (int x = 0; x < resolution; x++)
		{
			if (noiseBased)
				terrain.mesh->SetElevation(noise(x, y), x, y);
			else
				terrain.mesh->SetElevation(GetElevation(x, y), x, y);
		}
	}

	terrain.mesh->RecalculateNormals();

	isRemeshing = false;
}

static void RegenerateMesh() {

	if (isRemeshing)
		return;

	terrain.UploadToGPU();

	isRemeshing = true;

	std::thread worker(FillMeshData);
	worker.detach();


}

static void GenerateMesh()
{
	ResetShader();

	FillMeshData();

	terrain.SetupMeshOnGPU();
	sea.SetupMeshOnGPU();
	sea.mesh->GeneratePlane(256, 120);
	sea.mesh->RecalculateNormals();
	sea.UploadToGPU();

	grid.SetupMeshOnGPU();
	grid.mesh->GeneratePlane(50, 120);
	grid.mesh->RecalculateNormals();
	grid.UploadToGPU();
}

static void DoTheRederThing(float deltaTime) {

	static float time;
	time += deltaTime;
	camera.UpdateCamera(CameraPosition, CameraRotation);
	Shader* shader;
	if (skyboxEnabled)
		RenderSkybox(camera.pv);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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
	float tmp[3];
	tmp[0] = viewportMousePosX;
	tmp[1] = viewportMousePosY;
	tmp[2] = ImGui::GetIO().MouseDown[0];
	shader->SetUniform3f("_MousePos", tmp);
	tmp[0] = 800;
	tmp[1] = 600;
	tmp[2] = 1;
	shader->SetUniform3f("_Resolution", tmp);
	diffuse->Bind(5);
	shader->SetUniformi("_Diffuse", 5);
	terrain.Render();

	if(showFoliage)
		RenderFoliage(shader, camera);

	// For Future
	//gridTex->Bind(5);
	//grid->Render();


	if (showSea) {
		glUseProgram(0);
		waterShader->Bind();
		waterShader->SetTime(&time);
		sea.position.y = seaLevel;
		sea.Update();
		glm::mat4 tmp = glm::translate(camera.pv, glm::vec3(0, seaLevel, 0));
		waterShader->SetUniform3f("_SeaColor", SeaColor);
		waterShader->SetMPV(tmp);
		waterShader->SetLightCol(LightColor);
		waterShader->SetLightPos(LightPosition);
		sea.Render();
	}
}

static void ShowTerrainControls()
{
	static bool exp = false;
	ImGui::Begin("Dashboard");

	ImGui::DragInt("Mesh Resolution", &resolution, 1, 2, 8192);
	ImGui::DragFloat("Mesh Scale", &scale, 0.1f, 1.0f, 5000.0f);
	ImGui::NewLine();
	ImGui::Checkbox("Auto Save", &autoSave);
	ImGui::Checkbox("Auto Update", &autoUpdate);
	ImGui::Checkbox("Wireframe Mode", &wireFrameMode);
	ImGui::Checkbox("Show Skybox", &skyboxEnabled);
	ImGui::Checkbox("Show Foliage", &showFoliage);
	ImGui::NewLine();
	if (ImGui::Button("Update Mesh"))
		RegenerateMesh();

	if (ImGui::Button("Recalculate Normals")) {
		while (isRemeshing);
		terrain.mesh->RecalculateNormals();
	}

	if (ImGui::Button("Refresh Shaders")) {
		ResetShader();
	}

	if (ImGui::Button("Use Custom Shaders")) {
		activeWindows.shaderEditorWindow = true;
	}

	if (ImGui::Button("Texture Settings")) {
		activeWindows.texturEditorWindow = true;
	}

	if (ImGui::Button("Sea Settings")) {
		activeWindows.seaEditor = true;
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
					memset(noiseLayer.noiseType, 0, 1024);
					strcpy(noiseLayer.noiseType, noiseTypes[i]);
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
			noiseLayersTmp.push_back(noiseLayers[id].Clone());
		}
	}
}

static void ShowNoiseSettings() {
	ImGui::Begin("Noise Settings");

	ImGui::DragFloat("Noise Strength", &noiseStrength, 0.005f);
	ImGui::Checkbox("Flatten Base", &flattenBase);
	ImGui::Checkbox("Absoulute Value", &absolute);
	ImGui::Checkbox("Square Value", &square);

	ImGui::Separator();

	ImGui::Text("Noise Layers");
	ImGui::Separator();
	for (unsigned int i = 0; i < noiseLayers.size(); i++) {
		ShowNoiseLayer(noiseLayers[i], i);
		ImGui::Separator();
	}
	if (ImGui::Button("Add Noise Layer")) {
		noiseLayersTmp.push_back(NoiseLayer());
	}
	ImGui::End();
}

static void MouseMoveCallback(float x, float y) {
	float deltaX = x - prevMousePos.x;
	float deltaY = y - prevMousePos.y;

	if (button2) {


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

static void MouseScrollCallback(float amount) {
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
			if (ImGui::GetIO().MouseDown[1]) {
				CameraPosition[0] += -io.MouseDelta.x * 0.005f;
				CameraPosition[1] += io.MouseDelta.y * 0.005f;
			}
			viewportMousePosX = ImGui::GetIO().MousePos.x - viewportOffset.x;
			viewportMousePosY = ImGui::GetIO().MousePos.y - viewportOffset.y;

		}
		ImVec2 wsize = ImGui::GetWindowSize();
		ImGui::Image((ImTextureID)GetViewportFramebufferColorTextureId(), wsize, ImVec2(0, 1), ImVec2(1, 0));
		ImGui::EndChild();
	}
	ImGui::End();
}

static void SaveFile(std::string file = ShowSaveFileDialog()) {
	if (file.size() == 0)
		return;
	if (file.find(".terr3d") == std::string::npos)
		file += ".terr3d";

	savePath = file;

	nlohmann::json data;
	data["type"] = "SAVEFILE";
	data["versionHash"] = MD5File(GetExecutablePath()).ToString();
	data["version"] = "3.0";
	data["name"] = "TerraGen3D v3.0";
	data["EnodeEditor"] = GetElevationNodeEditorSaveData();
	data["styleData"] = GetStyleData();
	data["appData"] = appData;
	data["imguiData"] = std::string(ImGui::SaveIniSettingsToMemory());

	nlohmann::json noiseLayersSave;
	noiseLayersSave["type"] = "NOISE LAYERS";
	std::vector<nlohmann::json> noiseLayersSaveData;
	for (int i = 0; i < noiseLayers.size(); i++) {
		nlohmann::json t = noiseLayers[i].Save();
		t["id"] = i;
		noiseLayersSaveData.push_back(t);
	}
	noiseLayersSave["data"] = noiseLayersSaveData;
	data["noiseLayers"] = noiseLayersSave;
	nlohmann::json tmp;
	tmp["autoUpdate"] = autoUpdate;
	tmp["square"] = square;
	tmp["absolute"] = absolute;
	tmp["flattenBase"] = flattenBase;
	tmp["noiseBased"] = noiseBased ? true : false;
	tmp["wireFrameMode"] = wireFrameMode;
	tmp["skyboxEnabled"] = skyboxEnabled;
	tmp["showSea"] = showSea;
	tmp["vSync"] = vSync;
	tmp["mouseSpeed"] = mouseSpeed;
	tmp["scrollSpeed"] = scrollSpeed;
	tmp["mouseScrollAmount"] = mouseScrollAmount;
	tmp["scale"] = scale;
	tmp["textureScale"] = textureScale;
	tmp["textureScaleO"] = textureScaleO;
	tmp["seaLevel"] = seaLevel;
	tmp["autoSave"] = autoSave;

	tmp["cameraPosX"] = CameraPosition[0];
	tmp["cameraPosY"] = CameraPosition[2];
	tmp["cameraPosZ"] = CameraPosition[1];

	tmp["cameraRotX"] = CameraRotation[0];
	tmp["cameraRotY"] = CameraRotation[1];
	tmp["cameraRotZ"] = CameraRotation[2];

	tmp["lightPosX"] = LightPosition[0];
	tmp["lightPosY"] = LightPosition[2];
	tmp["lightPosZ"] = LightPosition[1];

	if (diffuse)
		tmp["diffuseTexPath"] = diffuse->GetPath();

	data["generals"] = tmp;

	std::ofstream outfile;
	outfile.open(file);

	outfile << data;

	outfile.close();
}

static void OpenSaveFile(std::string file = ShowOpenFileDialog((wchar_t*)".terr3d\0")) {

	// For Now it dows not do anything id any error has occured but in later versions this will be reported to user!


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
	nlohmann::json data;
	try {
		data = nlohmann::json::parse(sdata);
	}
	catch (...) {
		Log("Failed to Parse file : " + file);
	}
	try {
		if (data["versionHash"] != MD5File(GetExecutablePath()).ToString()) {
			Log("The file you are tryng to open was made with a different version of TerraGen3D!");
			return;
		}
	}
	catch (...) {
		Log("The file you are tryng to open was made with a different version of TerraGen3D!");
		return;
	}

	if (data["type"] == "THEME") {
		LoadThemeFromStr(data.dump());
	}


	if (data["type"] != "SAVEFILE")
		return;

	SetElevationNodeEditorSaveData(data["EnodeEditor"]);
	LoadThemeFromStr(data["styleData"]);
	data["imguiData"] = ImGui::SaveIniSettingsToMemory();
	appData = data["appData"];

	// This should be replaced with something better
	while (isRemeshing);
	noiseLayers.clear();

	std::vector<nlohmann::json> noiseLayersSaveData = data["noiseLayers"]["data"];
	for (int i = 0; i < noiseLayersSaveData.size(); i++) {
		noiseLayersTmp.push_back(NoiseLayer());
		noiseLayersTmp.back().Load(noiseLayersSaveData[i]);
	}

	nlohmann::json tmp = data["generals"];
	autoUpdate = tmp["autoUpdate"];
	square = tmp["square"];
	absolute = tmp["absolute"];
	flattenBase = tmp["flattenBase"];
	noiseBased = tmp["noiseBased"];
	skyboxEnabled = tmp["skyboxEnabled"];
	vSync = tmp["vSync"];
	showSea = tmp["showSea"];
	wireFrameMode = tmp["wireFrameMode"];
	mouseSpeed = tmp["mouseSpeed"];
	scrollSpeed = tmp["scrollSpeed"];
	mouseScrollAmount = tmp["mouseScrollAmount"];
	scale = tmp["scale"];
	textureScale = tmp["textureScale"];
	textureScaleO = tmp["textureScaleO"];
	seaLevel = tmp["seaLevel"];
	autoSave = tmp["autoSave"];

	CameraPosition[0] = tmp["cameraPosX"];
	CameraPosition[2] = tmp["cameraPosY"];
	CameraPosition[1] = tmp["cameraPosZ"];

	CameraRotation[0] = tmp["cameraRotX"];
	CameraRotation[1] = tmp["cameraRotY"];
	CameraRotation[2] = tmp["cameraRotZ"];

	LightPosition[0] = tmp["lightPosX"];
	LightPosition[2] = tmp["lightPosY"];
	LightPosition[1] = tmp["lightPosZ"];


	if (diffuse)
		delete diffuse;
	try {
		diffuse = new Texture2D(tmp["diffuseTexPath"]);
	}
	catch (...) {
		Log("Cold not load texture from saved file.");
	}

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
				if (ImGui::MenuItem("Wavefont OBJ")) {
					if (ExportOBJ(terrain.mesh->Clone(), openfilename())) {
						successMessage = "Sucessfully exported mesh!";
						ImGui::BeginPopup("Success Messages");
					}
					else {
						errorMessage = "One Export is already in progress!";
						ImGui::BeginPopup("Error Messages");
					}
				}

				if (ImGui::MenuItem("PNG Heightmap")) {
					if (ExportHeightmapPNG(terrain.mesh->Clone(), openfilename())) {
						successMessage = "Sucessfully exported heightmap!";
						ImGui::BeginPopup("Success Messages");
					}
					else {
						errorMessage = "One Export is already in progress!";
						ImGui::BeginPopup("Error Messages");
					}
				}

				if (ImGui::MenuItem("JPG Heightmap")) {
					if (ExportHeightmapJPG(terrain.mesh->Clone(), openfilename())) {
						successMessage = "Sucessfully exported heightmap!";
						ImGui::BeginPopup("Success Messages");
					}
					else {
						errorMessage = "One Export is already in progress!";
						ImGui::BeginPopup("Error Messages");
					}
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

			if (ImGui::MenuItem("Associate (.terr3d) File Type")) {
				AccocFileType();
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

			ShowWindowMenuItem("Foliage Manager", &activeWindows.foliageManager);

			ShowWindowMenuItem("Elevation Node Editor", &activeWindows.elevationNodeEditorWindow);

			ShowWindowMenuItem("Texture Settings", &activeWindows.texturEditorWindow);

			ShowWindowMenuItem("Texture Store", &activeWindows.textureStore);

			ShowWindowMenuItem("Sea Settings", &activeWindows.seaEditor);

			ShowWindowMenuItem("Contributers", &activeWindows.contribWindow);

			ShowWindowMenuItem("Open Source Liscenses", &activeWindows.osLisc);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Help")) {
			if (ImGui::MenuItem("Contributers"))
				activeWindows.contribWindow = true;

			if (ImGui::MenuItem("Tutorial"))
				ShellExecute(NULL, L"open", L"https://www.youtube.com/playlist?list=PLl3xhxX__M4A74aaTj8fvqApu7vo3cOiZ", NULL, NULL, SW_SHOWNORMAL);

			if (ImGui::MenuItem("Social Handle"))
				ShellExecute(NULL, L"open", L"https://twitter.com/jaysmito101", NULL, NULL, SW_SHOWNORMAL);

			if (ImGui::MenuItem("Discord Server"))
				ShellExecute(NULL, L"open", L"https://discord.gg/AcgRafSfyB", NULL, NULL, SW_SHOWNORMAL);

			if (ImGui::MenuItem("GitHub Page"))
				ShellExecute(NULL, L"open", L"https://github.com/Jaysmito101/TerraGen3D", NULL, NULL, SW_SHOWNORMAL);

			if (ImGui::MenuItem("Open Source Liscenses"))
				activeWindows.osLisc = !activeWindows.osLisc;

			ImGui::EndMenu();

		}
		ImGui::EndMainMenuBar();
	}
}

static void ShowErrorModal() {
	if (ImGui::BeginPopupModal("Error Messages")) {
		ImGui::TextColored(ImVec4(0.7f, 0.2f, 0.2f, 1.0f), errorMessage.c_str());
	}
}

static void ShowSuccessModal() {
	if (ImGui::BeginPopupModal("Success Messages")) {
		ImGui::TextColored(ImVec4(0.2f, 0.7f, 0.2f, 1.0f), successMessage.c_str());
	}
}

static void ShowTextureSettings() {
	ImGui::Begin("Texture Settings", &activeWindows.texturEditorWindow);
	uint32_t id = diffuse ? diffuse->GetRendererID() : 0;
	ImGui::Image((ImTextureID)(id), ImVec2(200, 200));
	if (!diffuse) {
		if (ImGui::Button("Load Texture")) {
			std::string textureFilePath = ShowOpenFileDialog((wchar_t*)L".png\0");
			if (textureFilePath.size() > 1) {
				diffuse = new Texture2D(textureFilePath);
			}
		}
	}
	else {
		if (ImGui::Button("Change Texture")) {
			std::string textureFilePath = ShowOpenFileDialog((wchar_t*)L".png\0");
			if (textureFilePath.size() > 1) {
				delete diffuse;
				diffuse = new Texture2D(textureFilePath);
			}
		}
	}

	ImGui::DragFloat("Texture Scale", &textureScale, 0.1f);


	ImGui::End();
}

static void ShowSeaSettings() {
	ImGui::Begin("Sea Settings", &activeWindows.seaEditor);

	ImGui::Checkbox("Show Sea Level", &showSea);

	ImGui::Text("Sea Color");
	ImGui::ColorEdit3("##seaColor", SeaColor);

	ImGui::DragFloat("Sea Level", &seaLevel, 0.1f);


	ImGui::End();
}

static void ShowContributers() {
	ImGui::Begin("Contributers", &activeWindows.contribWindow);
	ImGui::Text("The contributers as of Version 3.0:");
	ImGui::NewLine();
	ImGui::Text("Jaysmito Mukherjee");
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
		SetWindowConfigPath(GetExecutableDir() + "\\Data\\configs\\windowconfigs.terr3d");
		system((std::string("mkdir \"") + GetExecutableDir() + "\\Data\\cache\\autosave\"").c_str());
		SetupOSLiscences();
		Sleep(1000);
	}

	virtual void OnUpdate(float deltatime) override
	{
		if (!isRuinning)
			return;

		if (!isRemeshing) {
			for (NoiseLayer n : noiseLayersTmp)
				noiseLayers.push_back(n);
			noiseLayersTmp.clear();
		}

		if (reqTexRfrsh) {
			if (diffuse)
				delete diffuse;
			diffuse = new Texture2D(GetTextureStoreSelectedTexture(), true);
			reqTexRfrsh = false;
		}

		// CTRL Shortcuts
		if ((glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_LEFT_CONTROL) || glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_RIGHT_CONTROL))) {

			// Save Shortcut
			if (glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_S)) {
				if (savePath.size() > 3) {
					Log("Saved to " + savePath);
					SaveFile(savePath);
				}
				else {
					SaveFile();
				}
			}

			// Close Shortcut
			if (glfwGetKey(GetWindow()->GetNativeWindow(), GLFW_KEY_W)) {
				if (savePath.size() > 3) {
					Log("CLosed file " + savePath);
					savePath = "";
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
					savePath = "";
					SaveFile();
				}

			}
		}

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
		if (!isRuinning)
			return;
		
		secondCounter++;

		if (secondCounter % 5 == 0) {
			if (autoSave) {
				SaveFile(GetExecutableDir() + "\\Data\\cache\\autosave\\autosave.terr3d");
			}
		}

		GetWindow()->SetVSync(vSync);
		s_Stats.frameRate = 1 / s_Stats.deltaTime;
		ElevationNodeEditorTick();
		SecondlyShaderEditorUpdate();
		UpdateTextureStore();
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
		ShowErrorModal();
		ShowSuccessModal();

		// Optional Windows

		if (activeWindows.statsWindow)
			ShowStats();

		if (activeWindows.texturEditorWindow)
			ShowTextureSettings();

		if (activeWindows.seaEditor)
			ShowSeaSettings();

		if (activeWindows.contribWindow)
			ShowContributers();

		if (activeWindows.styleEditor)
			ShowStyleEditor(&activeWindows.styleEditor);

		if (activeWindows.foliageManager)
			ShowFoliageManager(&activeWindows.foliageManager);

		if (activeWindows.shaderEditorWindow)
			ShowShaderEditor(&activeWindows.shaderEditorWindow);

		if (activeWindows.elevationNodeEditorWindow)
			ShowElevationNodeEditor(&activeWindows.elevationNodeEditorWindow);

		if (activeWindows.textureStore)
			ShowTextureStore(&activeWindows.textureStore);

		if (activeWindows.osLisc)
			ShowOSLiscences(&activeWindows.osLisc);

		OnImGuiRenderEnd();
	}

	virtual void OnStart(std::string loadFile) override
	{
		srand((unsigned int)time(NULL));
		SetUpIcon();
		SetupViewportFrameBuffer();
		SetupCubemap();
		SetupShaderManager();
		SetupElevationManager(&resolution);
		SetupFoliageManager();
		SetupTextureStore(GetExecutableDir(), &reqTexRfrsh);
		diffuse = new Texture2D(GetExecutableDir() + "\\Data\\textures\\white.png");
		gridTex = new Texture2D(GetExecutableDir() + "\\Data\\textures\\grid->png", false, true);
		ImGui::GetStyle().WindowMenuButtonPosition = ImGuiDir_None;
		LoadDefaultStyle();
		m_NoiseGen = FastNoiseLite::FastNoiseLite();
		GetWindow()->SetShouldCloseCallback(OnAppClose);
		glfwSetFramebufferSizeCallback(GetWindow()->GetNativeWindow(), [](GLFWwindow* window, int w, int h) {
			glfwSwapBuffers(window);
			});
		glfwSetScrollCallback(GetWindow()->GetNativeWindow(), [](GLFWwindow*, double x, double y) {
			ImGuiIO& io = ImGui::GetIO();
			io.MouseWheel = (float)y;
			});
		GetWindow()->SetClearColor({ 0.1f, 0.1f, 0.1f });
		GenerateMesh();
		glEnable(GL_DEPTH_TEST);
		LightPosition[1] = -0.3f;
		CameraPosition[1] = 0.2f;
		CameraPosition[2] = 3.1f;
		CameraRotation[1] = 2530.0f;
		autoUpdate = false;
		scale = 1;
		noiseLayersTmp.push_back(NoiseLayer());
		Log("Started Up App!");

		if (loadFile.size() > 0) {
			Log("Loading File from " + loadFile);
			OpenSaveFile(loadFile);
		}

		// For Debug Only
		autoUpdate = true;
	}

	void OnEnd()
	{
		ShutdownElevationNodeEditor();
		delete shd;
		delete wireframeShader;
	}
};

Application* CreateApplication()
{
	myApp = new MyApp();
	return myApp;
}