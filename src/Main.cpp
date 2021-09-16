#define _CRT_SECURE_NO_WARNINGS
#include "../resource.h"
#include <Base.h>
#include <Base/EntryPoint.h>
#include <ImGuiConsole.h>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <windows.h>
#include <string.h>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_relational.hpp>
#include <glm/ext/vector_relational.hpp>
#include <glm/ext/scalar_relational.hpp>
#include <SimplexNoise.h>
#include <FastNoiseLite.h>
#include <thread>
#include <time.h>
#include <mutex>
#include <json.hpp>
#undef near
#undef far

struct Vec2 {
	float x = 0;
	float y = 0;
};

struct Vec3 {
	float x = 0;
	float y = 0;
	float z = 0;
};

struct Vec4 {
	float x = 0;
	float y = 0;
	float z = 0;
	float w = 0;
};

struct Vert {
	glm::vec3 position;
	glm::vec3 normal;
};

struct Mesh {
	Vert* vert;
	int* indices;
	int vertexCount;
	int indexCount;
};

struct Camera {
	glm::mat4 view;
	glm::mat4 pers;
	glm::mat4 pv;
	glm::vec3 position;
	glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	glm::vec3 rotation;
	float pitch;
	float yaw;
	float roll;
	float fov;
	float far;
	float aspect;
	float near;

	Camera() {
		fov = 45;
		near = 0.01f;
		far = 100.0f;
		aspect = 16.0f / 9.0f;
		pitch = yaw = roll = 0;
		view = glm::mat4(1.0f);
		pv = glm::mat4(1.0f);
		pers = glm::perspective(fov, aspect, near, far);
		position = glm::vec3(0.0f, 0.0f, 3.0f);
		rotation = glm::vec3(1.0f);
	}

	void UpdateRotation()
	{
		pitch = rotation.x;
		yaw = rotation.y;
		roll = rotation.z;
	}

	void UpdateCamera(float* pos, float* rot)
	{
		position.x = pos[0];
		position.y = pos[1];
		position.z = pos[2];
		rotation.x = glm::radians(rot[0]);
		rotation.y = glm::radians(rot[1]);
		rotation.z = glm::radians(rot[2]);
		UpdateRotation();
		view = glm::lookAt(position, position + cameraFront, cameraUp);
		view = glm::rotate(view, glm::radians(rotation.y), glm::vec3(1.0f, 0.0f, 0.0f));
		view = glm::rotate(view, glm::radians(rotation.x), glm::vec3(0.0f, 1.0f, 0.0f));
		pers = glm::perspective(fov, aspect, near, far);
		pv = pers * view;
	}
};

struct Stats
{
	float deltaTime = 1;
	float frameRate = 1;
	int triangles = 0;
	int vertCount = 0;
};

struct NoiseLayer {

	NoiseLayer() {
		noiseType = "Simplex Perlin";
		strcpy(name, "Noise Layer");
		strength = 0.0f;
		enabled = true;
		active = false;
		scale = 1;
		offsetX = 0;
		offsetY = 0;
	}

	const char* noiseType;
	char name[256];
	float strength;
	float offsetX, offsetY;
	float scale;
	bool enabled;
	bool active;
};

static std::string vs = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNorm;

uniform mat4 _PV;

out vec3 FragPos;
out vec3 Normal;

void main()
{
    gl_Position = _PV * vec4(aPos.x, aPos.y, aPos.z, 1.0);
	FragPos = aPos;
	Normal = aNorm;
}
)";

static std::string fs = R"(

#version 330 core
out vec4 FragColor;

uniform vec3 _LightPosition;
uniform vec3 _LightColor;

in vec3 FragPos;
in vec3 Normal;

void main()
{	
	//vec3 objectColor = vec3(0.34f, 0.49f, 0.27f);
	vec3 objectColor = vec3(1, 1, 1);
	vec3 norm = normalize(Normal);
	vec3 lightDir = normalize(_LightPosition - FragPos);
	float diff = max(dot(norm, lightDir), 0.0f);
	vec3 diffuse = diff * _LightColor;
	vec3 result = (vec3(0.2, 0.2, 0.2) + diffuse) * objectColor;
	FragColor = vec4(result, 1.0);
} 
)";
static Application* myApp;
static glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
static glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
static glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
static bool vSync = true;
static Stats s_Stats;
static char b1[4096], b2[4096];
static uint32_t vao, vbo, ebo, fbo;
static uint32_t depthTex, colorTex;
static Shader* shd;
static Camera camera;
static Mesh mesh;
static glm::vec3 LightPosition = glm::vec3(0.0f);
static std::string fileName = "";
static bool autoUpdate = false;
static float CameraPosition[3];
static float CameraRotation[3];
static float noiseScale = 0.01f;
static float noiseStrength = 1.0f;
static bool absolute = false;
static bool square = false;
static int resolution = 256;
static float fadeOFf = 0.1f;
static float LightColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
static Vec2 prevMousePos;
static bool button1, button2, button3;
static float mouseSpeed = 25;
static float scrollSpeed = 0.5f;
static float mouseScrollAmount = 0;
static float scale = 1.0f;
static bool flattenBase = false;
static std::string vertPath = "./vert.glsl";
static std::string fragPath= "./frag.glsl";
static std::vector<NoiseLayer> noiseLayers;
static const char* noiseTypes[] = {"Simplex Perlin", "Random", "Cellular"};
static int numberOfNoiseTypes = 3;
static FastNoiseLite m_NoiseGen;
static bool isRemeshing = false;
std::mutex m_mutex;

static void ToggleSystemConsole() {
	static bool state = true;
	state = !state;
	ShowWindow(GetConsoleWindow(), state?SW_SHOW:SW_HIDE);
}

static void Log(const char* log)
{
	std::cout << log << std::endl;
};

void OnAppClose(int x, int y)
{
	Log("Close App");
	myApp->Close();
}

static std::string ReadShaderSourceFile(std::string path, bool* result) {
	std::fstream newfile;
	newfile.open(path.c_str(), std::ios::in);
	if (newfile.is_open()) {
		std::string tp;
		std::string res = "";
		getline(newfile, res, '\0');
		newfile.close();
		return res;
	}else{
		*result = false;
	}
	return std::string("");
}

static void ShowStats() 
{
	ImGui::Begin("Statistics");

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
		delete mesh.vert;
		delete mesh.indices;
		exit(0);
	}

	ImGui::End();
}

static void ShowConsole() 
{
}

static void InitGlad() {
	if (!gladLoadGL()) {
		Log("Failed to Initialize GLAD!");
		exit(-1);
	}
}

static void ShowCameraControls() {
	ImGui::Begin("Camera Controls");

	ImGui::Text("Camera Position");
	ImGui::DragFloat3("##cameraPosition", CameraPosition, 0.1);
	ImGui::Separator();
	ImGui::Separator();
	ImGui::Text("Camera Rotation");
	ImGui::DragFloat3("##cameraRotation", CameraRotation, 10);
	ImGui::Separator();
	ImGui::Separator();
	ImGui::Text("Projection Settings");
	ImGui::Separator();
	ImGui::DragFloat("FOV", &(camera.fov), 0.01);
	ImGui::DragFloat("Aspect Ratio", &(camera.aspect), 0.01);
	ImGui::DragFloat("Near Clipping", &(camera.near), 0.01);
	ImGui::DragFloat("Far Clipping", &(camera.far), 0.01);

	ImGui::End();
}

static void ShowLightingControls() {
	ImGui::Begin("Light Controls");

	ImGui::Text("Light Position");
	ImGui::DragFloat3("##lightPosition", &LightPosition[0], 0.1);
	ImGui::Separator();
	ImGui::Separator();
	ImGui::Text("Light Color");
	ImGui::ColorPicker4("##lightColor", LightColor);

	ImGui::End();
}

std::string openfilename(HWND owner = NULL) {
	OPENFILENAME ofn;
	WCHAR fileName[MAX_PATH];
	ZeroMemory(fileName, MAX_PATH);
	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = owner;
	ofn.lpstrFilter = L"*.obj\0";
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = (LPWSTR)"";

	std::string fileNameStr;

	if (GetSaveFileName(&ofn)) {
		std::wstring ws(ofn.lpstrFile);
		// your new String
		std::string str(ws.begin(), ws.end());
		return str;
	}
	return std::string("");
}

std::string ShowOpenFileDialog(HWND owner = NULL) {
	OPENFILENAME ofn;
	WCHAR fileName[MAX_PATH];
	ZeroMemory(fileName, MAX_PATH);
	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = owner;
	ofn.lpstrFilter = L"*.glsl\0*.*\0";
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = (LPWSTR)"";

	std::string fileNameStr;

	if (GetOpenFileName(&ofn)) {
		std::wstring ws(ofn.lpstrFile);
		// your new String
		std::string str(ws.begin(), ws.end());
		return str;
	}
	return std::string("");
}

static std::string ExportOBJ() {
	std::string fileName = openfilename();

	std::ofstream outfile;
	outfile.open(fileName + ".obj");


	outfile << "# TerraGenV1.0 OBJ" << std::endl << std::endl;;

	for (int i = 0; i < mesh.vertexCount; i++) 
	{
		outfile << "v " << mesh.vert[i].position.x << " " << mesh.vert[i].position.y << " " << mesh.vert[i].position.z << " "  << std::endl;
	}

	outfile << std::endl;
	outfile << std::endl;

	for (int i = 0; i < mesh.indexCount; i += 3)
	{
		outfile << "f " << mesh.indices[i] + 1 << " " << mesh.indices[i + 1] + 1 << " " << mesh.indices[i + 2] + 1 << " " << std::endl;
	}

	outfile.close();
	return fileName;

}

static void RecalculateNormals(Mesh& mesh) 
{
	for (int i = 0; i < mesh.indexCount; i += 3)
	{
		const int ia = mesh.indices[i];
		const int ib = mesh.indices[i + 1];
		const int ic = mesh.indices[i + 2];

		const glm::vec3 e1 = mesh.vert[ia].position - mesh.vert[ib].position;
		const glm::vec3 e2 = mesh.vert[ic].position - mesh.vert[ib].position;
		const glm::vec3 no = cross(e1, e2);

		mesh.vert[ia].normal += no;
		mesh.vert[ib].normal += no;
		mesh.vert[ic].normal += no;
	}

	for (int i = 0; i < mesh.vertexCount; i++) mesh.vert[i].normal = glm::normalize(mesh.vert[i].normal);
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
		noise = max(0, noise);
	}
	if (absolute) {
		noise = abs(noise);
	}
	if (square) {
		noise *= noise;
	}
	return noise * noiseStrength;
}

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

			vertices[i].position.x = pointOnPlane.x;
			vertices[i].position.y = pointOnPlane.y + noise(x, y);
			vertices[i].position.z = pointOnPlane.z;
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

static void SetupFrameBuffer() {
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glGenTextures(1, &colorTex);
	glBindTexture(GL_TEXTURE_2D, colorTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 800, 600, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex, 0);
	glGenTextures(1, &depthTex);
	glBindTexture(GL_TEXTURE_2D, depthTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 800, 600, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void DeleteBuffers(GLuint a, GLuint b, GLuint c) {
	glDeleteBuffers(1, &a);
	glDeleteBuffers(1, &b);
	glDeleteBuffers(1, &c);
}

static void ResetShader() {
	if (shd)
		delete shd;

	bool res = true;
	std::string vertSrc = ReadShaderSourceFile(vertPath, &res);
	if (!res) {
		vertSrc = vs;
		Log((std::string("Could not load ") + vertPath + "! Using default Vertex shader!").c_str());
		vertPath = "Internal";
	}
	else {
		Log((std::string("Loaded Vertex shader from ") + vertPath).c_str());
	}
	res = true;
	std::string fragSrc = ReadShaderSourceFile(fragPath, &res);
	if (!res) {
		fragSrc = fs;
		Log((std::string("Could not load ") + fragPath + "! Using default Fragment shader!").c_str());
		fragPath = "Internal";
	}
	else {
		Log((std::string("Loaded Fragment shader from ") + fragPath).c_str());
	}

	shd = new Shader(vertSrc, fragSrc);
}

static void GenerateMesh() 
{
	GenerateVertices();

	ResetShader();

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vert) * mesh.vertexCount, mesh.vert, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * mesh.indexCount, mesh.indices, GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), (void*)offsetof(Vert, position));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), (void*)offsetof(Vert, normal));
	glEnableVertexAttribArray(1);

}


static void RegenerateMesh(){
	
	GenerateVertices();

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vert) * mesh.vertexCount, mesh.vert, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * mesh.indexCount, mesh.indices, GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), (void*)offsetof(Vert, position));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), (void*)offsetof(Vert, normal));
	glEnableVertexAttribArray(1);
	
}

static void DoTheRederThing(float deltaTime) {
	static float time;
	time += deltaTime;
	camera.UpdateCamera(CameraPosition, CameraRotation);
	shd->SetTime(&time);
	shd->SetMPV(camera.pv);
	shd->SetLightCol(LightColor);
	shd->SetLightPos(LightPosition);
	shd->Bind();
	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

static void ShowTerrainControls()
{
	static bool exp = false;
	ImGui::Begin("Terrain Controls");


	ImGui::DragInt("Mesh Resolution", &resolution, 1, 2, 8192);
	ImGui::DragFloat("Mesh Scale", &scale, 0.1f, 1.0f, 5000.0f);
	//ImGui::DragFloat("Noise Scale", &noiseScale, 0.005f);
	ImGui::DragFloat("Noise Strength", &noiseStrength, 0.005f);
	ImGui::NewLine();
	ImGui::Checkbox("Auto Update", &autoUpdate);
	ImGui::Checkbox("Flatten Base", &flattenBase);
	ImGui::Checkbox("Absoulute Value", &absolute);
	ImGui::Checkbox("Square Value", &square);
	ImGui::NewLine();
	if(ImGui::Button("Update Mesh"))
		RegenerateMesh();

	if (ImGui::Button("Refresh Shaders")) {
		ResetShader();
	}
	ImGui::Separator();
	ImGui::Text("Current: %s", vertPath.c_str());
	if (ImGui::Button("Use Custom Vertex Shader")) {
		vertPath = ShowOpenFileDialog();
		ResetShader();
	}
	ImGui::Separator();
	ImGui::Text("Current: %s", fragPath.c_str());

	if (ImGui::Button("Use Custom Fragment Shader")) {
		fragPath = ShowOpenFileDialog();
		ResetShader();
	}

	ImGui::Separator();
	if (ImGui::Button("Export as OBJ")) {
		fileName = ExportOBJ();
		exp = true;
	}
	if (exp) {
		ImGui::TextColored(ImVec4(0.0f, 0.7f, 0.0f, 1.0f), "Exported mesh as OBJ! (%s)", fileName);
		ImGui::NewLine();
		ImGui::TextColored(ImVec4(0.0f, 0.7f, 0.0f, 1.0f), "(%s)", fileName);
	}

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
	ImGui::Text("Noise Layers");
	ImGui::Separator();
	for (int i = 0; i < noiseLayers.size();i++) {
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
		ImGui::Image((ImTextureID)colorTex, wsize, ImVec2(0, 1), ImVec2(1, 0));
		ImGui::EndChild();
	}
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
	if (opt_fullscreen)
		ImGui::PopStyleVar(2);
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

	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::Button("Exit")) {
				exit(0);
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Options"))
		{
			if (ImGui::Button("Toggle System Console")) {
				ToggleSystemConsole();
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
}

static void OnImGuiRenderEnd() {
	ImGui::End();
}

static void ResizeTexture(uint32_t tex, int w, int h) {
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
}

void SetDarkMode()
{
	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
	colors[ImGuiCol_Border] = ImVec4(0.19f, 0.19f, 0.19f, 0.29f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
	colors[ImGuiCol_Button] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
	colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
	colors[ImGuiCol_Tab] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.20f, 0.20f, 0.36f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_DockingPreview] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
	colors[ImGuiCol_DockingEmptyBg] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_TableHeaderBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
	colors[ImGuiCol_TableBorderStrong] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
	colors[ImGuiCol_TableBorderLight] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
	colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
	colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.35f);

	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowPadding = ImVec2(8.00f, 8.00f);
	style.FramePadding = ImVec2(5.00f, 2.00f);
	style.CellPadding = ImVec2(6.00f, 6.00f);
	style.ItemSpacing = ImVec2(6.00f, 6.00f);
	style.ItemInnerSpacing = ImVec2(6.00f, 6.00f);
	style.TouchExtraPadding = ImVec2(0.00f, 0.00f);
	style.IndentSpacing = 25;
	style.ScrollbarSize = 15;
	style.GrabMinSize = 10;
	style.WindowBorderSize = 1;
	style.ChildBorderSize = 1;
	style.PopupBorderSize = 1;
	style.FrameBorderSize = 1;
	style.TabBorderSize = 1;
	style.WindowRounding = 7;
	style.ChildRounding = 4;
	style.FrameRounding = 3;
	style.PopupRounding = 4;
	style.ScrollbarRounding = 9;
	style.GrabRounding = 3;
	style.LogSliderDeadzone = 4;
	style.TabRounding = 4;
}

class MyApp : public Application 
{
public:
	virtual void OnUpdate(float deltatime) override
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glViewport(0, 0, 800, 600);
		GetWindow()->Clear();
		s_Stats.deltaTime = deltatime;
		DoTheRederThing(deltatime);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		RenderImGui();
		if (autoUpdate)
			RegenerateMesh();
	}

	virtual void OnOneSecondTick() override 
	{
		GetWindow()->SetVSync(vSync);
		s_Stats.frameRate = 1 / s_Stats.deltaTime;
	}

	virtual void OnImGuiRender() override
	{
		OnBeforeImGuiRender();
		ShowGeneralControls();
		ShowStats();
		ShowConsole();
		ShowCameraControls();
		ShowTerrainControls();
		ShowLightingControls();
		ShowNoiseSettings();
		ShowMainScene();
		OnImGuiRenderEnd();
	}

	virtual void OnStart() override
	{
		Log("Started Up App!");
		srand(time(NULL));
		SetDarkMode();
		m_NoiseGen = FastNoiseLite::FastNoiseLite();
		SetupFrameBuffer();
		GetWindow()->SetShouldCloseCallback(OnAppClose);
		glfwSetFramebufferSizeCallback(GetWindow()->GetNativeWindow(), [] (GLFWwindow* window, int w, int h){
			glfwSwapBuffers(window);
			});
		glfwSetScrollCallback(GetWindow()->GetNativeWindow(), [](GLFWwindow*, double x, double y) {
			ImGuiIO& io = ImGui::GetIO();
			io.MouseWheel = y;
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
	}

	void OnEnd() 
	{
		delete shd;
	}
};

Application* CreateApplication()
{
	myApp = new MyApp();
	return myApp;
}