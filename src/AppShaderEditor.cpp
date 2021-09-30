#define _CRT_SECURE_NO_WARNINGS

#include <text-editor/TextEditor.h>
#include "AppShaderEditor.h"
#include <AppStructs.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>
#include <Utils.h>

static std::string defaultBaseVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNorm;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 _PV;
uniform mat4 _Model;

out DATA
{

    vec3 FragPos;
    vec3 Normal;
	vec2 TexCoord;
	mat4 PV;
} data_out; 

void main()
{
    gl_Position =  vec4(aPos.x, aPos.y, aPos.z, 1.0);
	data_out.FragPos = aPos;
	data_out.Normal = aNorm;
	data_out.TexCoord = aTexCoord;
	data_out.PV = _PV;
}
)";

static std::string defaultBaseGeometryShader = R"(
#version 330 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

in DATA
{

    vec3 FragPos;
    vec3 Normal;
	vec2 TexCoord;
	mat4 PV;
} data_in[]; 

void main()
{	
	gl_Position = data_in[0].PV * gl_in[0].gl_Position;
	Normal = data_in[0].Normal;
	TexCoord = data_in[0].TexCoord;
	FragPos = data_in[0].FragPos;
	EmitVertex();

	gl_Position = data_in[0].PV * gl_in[1].gl_Position;
	Normal = data_in[1].Normal;
	TexCoord = data_in[0].TexCoord;
	FragPos = data_in[1].FragPos;
	EmitVertex();

	gl_Position =  data_in[0].PV * gl_in[2].gl_Position;
	Normal = data_in[2].Normal;
	TexCoord = data_in[0].TexCoord;
	FragPos = data_in[2].FragPos;
	EmitVertex();

	EndPrimitive();
} 
)";

static std::string defaultBaseFragmentShader = R"(
#version 330 core
out vec4 FragColor;

uniform vec3 _LightPosition;
uniform vec3 _LightColor;

uniform sampler2D _Diffuse;
//unifrom float _UseTexutres;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

void main()
{	
	//vec3 objectColor = vec3(0.34f, 0.49f, 0.27f);
	vec3 objectColor = vec3(1, 1, 1);
	objectColor = texture(_Diffuse, TexCoord).xyz;
	vec3 norm = normalize(Normal);
	vec3 lightDir = normalize(_LightPosition - FragPos);
	float diff = max(dot(norm, lightDir), 0.0f);
	vec3 diffuse = diff * _LightColor;
	vec3 result = (vec3(0.2, 0.2, 0.2) + diffuse) * objectColor;
	FragColor = vec4(result, 1.0);
} 
)";


enum ShaderType {
	Vertex = 0,
	Fragment = 1,
	Geometry = 2
};

struct ShaderFile {
	char* shaderSource;
	std::string filePath;
	int sourceSize;
	ShaderType shaderType;

	ShaderFile(ShaderType type = ShaderType::Vertex, int size = 1024*1024*10, bool initialize = true) 
	{
		sourceSize = size;
		shaderType = type;
		shaderSource = (char*)malloc(size);
		filePath = (shaderType == ShaderType::Vertex ? "vert.glsl":(shaderType == ShaderType::Geometry?"geom.glsl": "frag.glsl"));
		if (initialize) {
			std::string src = (shaderType == ShaderType::Vertex ? defaultBaseVertexShader : (shaderType == ShaderType::Geometry ? defaultBaseGeometryShader : defaultBaseFragmentShader));
			memset(shaderSource, 0, size);
			strcat(shaderSource, src.c_str());
		}
	}

	void SetSource(std::string source) {
		memset(shaderSource, 0, sourceSize);
		strcat(shaderSource, source.c_str());
	}

	std::string GetSource() {
		if (shaderSource)
			return std::string(shaderSource);
		else
			return "";
	}

	~ShaderFile() {
		delete shaderSource;
	}
};


static TextEditor::ErrorMarkers markers;
static TextEditor Veditor, Feditor, Geditor;
bool isCurrVertexShader = true;
static std::vector<ShaderFile> shaderFiles;
static ShaderFile vertShaderFile(ShaderType::Vertex);
static ShaderFile fragShaderFile(ShaderType::Fragment);
static ShaderFile geomShaderFile(ShaderType::Geometry);
ShaderType current;
static bool reqRfrsh = false;
static void ApplyShaders();
static bool isVs = false;
static bool isFs = false;
static bool isGs = false;

static void Log(const char* log)
{
	std::cout << log << std::endl;
};

bool ReqRefresh() {
	bool t = reqRfrsh;
	reqRfrsh = false;
	return t;
}

static std::string ShowSaveFileDialogSh(HWND owner = NULL) {
	OPENFILENAME ofn;
	WCHAR fileName[MAX_PATH];
	ZeroMemory(fileName, MAX_PATH);
	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = owner;
	ofn.lpstrFilter = L"*.glsl\0";
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

static std::string ShowOpenFileDialogSh(HWND owner = NULL) {
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


void SetupShaderManager() {
	auto lang = TextEditor::LanguageDefinition::GLSL();
	Veditor.SetLanguageDefinition(lang);
	Veditor.SetText(vertShaderFile.GetSource());
	Feditor.SetLanguageDefinition(lang);
	Feditor.SetText(fragShaderFile.GetSource());
	Geditor.SetLanguageDefinition(lang);
	Geditor.SetText(geomShaderFile.GetSource());
	current = ShaderType::Vertex;
}

std::string GetVertexShaderSource()
{
	return vertShaderFile.GetSource();
}

std::string GetFragmentShaderSource()
{
	return fragShaderFile.GetSource();
}

std::string GetGeometryShaderSource()
{
	return geomShaderFile.GetSource();
}
std::string GetDefaultFragmentShaderSource() 
{
	return defaultBaseFragmentShader;
}

std::string GetDefaultVertexShaderSource() 
{
	return defaultBaseVertexShader;
}



// This will be updated later
std::string GetMeshNormalsGeometryShaderSource()
{
	return std::regex_replace(defaultBaseGeometryShader, std::regex("triangle_strip"), "line_strip");
}

std::string GetWireframeGeometryShaderSource()
{
	return std::regex_replace(defaultBaseGeometryShader, std::regex("triangle_strip"), "line_strip");
}

static void CreateShader(ShaderType type) {
	std::string fileName = ShowSaveFileDialogSh();
	std::ofstream outfile;
	if (fileName.find(".glsl") == std::string::npos)
		fileName += ".glsl";
	outfile.open(fileName);

	if (type == ShaderType::Vertex) {
		vertShaderFile.filePath = fileName;
		outfile << vertShaderFile.GetSource();
		Veditor.SetText(vertShaderFile.GetSource());
	}
	else if (type == ShaderType::Fragment) {
		outfile << fragShaderFile.GetSource();
		fragShaderFile.filePath = fileName;
		Feditor.SetText(fragShaderFile.GetSource());
	}
	else if (type == ShaderType::Geometry) {
		outfile << geomShaderFile.GetSource();
		geomShaderFile.filePath = fileName;
		Geditor.SetText(geomShaderFile.GetSource());
	}

	outfile.close();
}

static void OpenShader(ShaderType type) {
	std::string fileName = ShowOpenFileDialogSh();

	bool res = true;

	std::string ss = ReadShaderSourceFile(fileName, &res);
	if (!res)
		ss = "Failed to Load " + fileName;

	if (type == ShaderType::Vertex) {
		vertShaderFile.filePath = fileName;
		vertShaderFile.SetSource(ss);
		Veditor.SetText(vertShaderFile.GetSource());
	}
	else if (type == ShaderType::Fragment) {
		fragShaderFile.filePath = fileName;
		fragShaderFile.SetSource(ss);
		Feditor.SetText(fragShaderFile.GetSource());
	}
	else if (type == ShaderType::Geometry) {
		geomShaderFile.filePath = fileName;
		geomShaderFile.SetSource(ss);
		Geditor.SetText(geomShaderFile.GetSource());
	}
}

static void ApplyShaders() {
	reqRfrsh = true;
	vertShaderFile.SetSource(Veditor.GetText());
	fragShaderFile.SetSource(Feditor.GetText());
	geomShaderFile.SetSource(Geditor.GetText());
}

static void SaveShaders() {
	isVs = true;
	isFs = true;
	std::string fileName = (current == ShaderType::Vertex ? vertShaderFile : (current == ShaderType::Geometry ? geomShaderFile : fragShaderFile)).filePath;
	std::ofstream outfile;
	outfile.open(fileName);
	if (current == ShaderType::Vertex)
		outfile << Veditor.GetText();;
	if (current == ShaderType::Fragment)
		outfile << Feditor.GetText();;
	if (current == ShaderType::Geometry)
		outfile << Geditor.GetText();;
	outfile.close();
}

void SecondlyShaderEditorUpdate() 
{
	if (current == ShaderType::Vertex) 
	{
		if (vertShaderFile.GetSource() != Veditor.GetText()) {
			isVs = false;
		}
	}
	else if (current == ShaderType::Fragment) 
	{
		if (fragShaderFile.GetSource() != Feditor.GetText()) {
			isFs = false;
		}
	}
	else if (current == ShaderType::Geometry)
	{
		if (geomShaderFile.GetSource() != Geditor.GetText()) {
			isGs = false;
		}
	}
}


void ShowShaderEditor(bool* pOpen)
{
	static bool showCreateShaderPopup = false;

	ImGui::Begin("Shader Editor Window (Beta)", pOpen);

	if (ImGui::Button("Create Shader")) {
		CreateShader(current);
	}
	ImGui::SameLine();

	if (ImGui::Button("Open Shader")) {
		OpenShader(current);
	}
	ImGui::SameLine();

	if (ImGui::Button("Apply##shaderEditor")) {
		ApplyShaders();
	}
	ImGui::SameLine();

	if (ImGui::Button("Save & Apply##shaderEditor")) {
		ApplyShaders();
		SaveShaders();
	}
	ImGui::NewLine();

	if ((current == ShaderType::Vertex && !isVs) || (current == ShaderType::Fragment && !isFs) || (current == ShaderType::Geometry && !isGs))
		ImGui::Text("[UNSAVED]");
	else
		ImGui::Text("[SAVED]");

	if (ImGui::BeginTabBar("##shaderEditorTabs", ImGuiTabBarFlags_None))
	{
		bool rout = ImGui::BeginTabItem("Vertex Shader");
		if (rout)
		{
			current = ShaderType::Vertex;
			Veditor.Render("Vertex Shader Editor");
			ImGui::EndTabItem();
		}

		rout = ImGui::BeginTabItem("Geometry Shader");
		if (rout)
		{
			current = ShaderType::Geometry;
			Geditor.Render("Geometry Shader Editor");
			ImGui::EndTabItem();
		}

		rout = ImGui::BeginTabItem("Fragment Shader");
		if (rout)
		{
			current = ShaderType::Fragment;
			Feditor.Render("Fragment Shader Editor");
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}
	ImGui::End();
}
