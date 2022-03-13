#include "Application.h"

#include <Windows.h>
#include <filesystem>
#include <ctime>
#include <iostream>
#include <sstream>
#include <fstream>
#include "json.hpp"
#include "zip.h"
#include "dirent.h"
#include <imgui.h>

class MyApp;
void OnAppClose(int x, int y);

static MyApp* myApp;

static const char* moduleLanguages[] = {"C++"};

static char moduleDLLPath[1024 * 4];
static char moduleOutputPath[1024 * 4];
static char moduleResourceFolder[1024 * 4];
static char moduleName[1024 * 4];
static char moduleAuthorName[1024 * 4];
static char moduleVersion[1024 * 4];
static char moduleLiscenseFile[1024 * 4];
static int  moduleVersionInteger;
static int  moduleLanguage;
static bool isFilter, isNode, isNoiseLayer, isUIHelper;
static const char* moduleLanguageStr;

#define MAX(x, y) x > y ? x : y

static void zip_walk(struct zip_t* zip, const char* path, bool isFristLayer = true, std::string prevPath = "") {
	DIR* dir;
	struct dirent* entry;
	char fullpath[MAX_PATH];
	struct stat s;

	memset(fullpath, 0, MAX_PATH);
	dir = opendir(path);
	assert(dir);

	while ((entry = readdir(dir))) {
		// skip "." and ".."
		if (!strcmp(entry->d_name, ".\0") || !strcmp(entry->d_name, "..\0"))
			continue;

		snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);
		stat(fullpath, &s);
		if (S_ISDIR(s.st_mode)) {
			zip_walk(zip, fullpath, false, prevPath + entry->d_name + "\\");
		}
		else {
			zip_entry_open(zip, (prevPath + entry->d_name).c_str());
			zip_entry_fwrite(zip, fullpath);
			zip_entry_close(zip);
		}
	}

	closedir(dir);
}

#ifdef TERR3D_WIN32

std::wstring s2ws(const std::string& s)
{
	int len;
	int slength = (int)s.length() + 1;
	len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
	std::wstring r(buf);
	delete[] buf;
	return r;
}

#endif

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

}

static void OnImGuiRenderEnd() {
	ImGui::End();
}


static std::string ShowSaveFileDialog(std::string ext) {
#ifdef TERR3D_WIN32
	OPENFILENAME ofn;
	WCHAR fileName[MAX_PATH];
	ZeroMemory(fileName, MAX_PATH);
	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = NULL;
	ofn.lpstrFilter = s2ws(ext).c_str();
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
#else
	char filename[PATH_MAX];
	FILE* f = popen("zenity --file-selection --save", "r");
	fgets(filename, PATH_MAX, f);
	pclose(f);
	return std::string(fileName);
#endif
}

static std::string ShowOpenFileDialog(std::string ext, bool folder = false) {
#ifdef TERR3D_WIN32
	OPENFILENAME ofn;
	WCHAR fileName[MAX_PATH];
	ZeroMemory(fileName, MAX_PATH);
	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = NULL;
	ofn.lpstrFilter = s2ws(ext).c_str();
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
#else
	char filename[PATH_MAX];
	FILE* f = popen("zenity --file-selection", "r");
	fgets(filename, PATH_MAX, f);
	pclose(f);
	return std::string(fileName);
#endif
}

static void SetupStyle() 
{
	ImGuiStyle* style = &ImGui::GetStyle();

	style->WindowPadding = ImVec2(15, 15);
	style->WindowRounding = 5.0f;
	style->FramePadding = ImVec2(5, 5);
	style->FrameRounding = 4.0f;
	style->ItemSpacing = ImVec2(12, 8);
	style->ItemInnerSpacing = ImVec2(8, 6);
	style->IndentSpacing = 25.0f;
	style->ScrollbarSize = 15.0f;
	style->ScrollbarRounding = 9.0f;
	style->GrabMinSize = 5.0f;
	style->GrabRounding = 3.0f;
	style->WindowBorderSize = 0;
	style->ChildBorderSize = 0;
	style->PopupBorderSize = 0;
	style->FrameBorderSize = 0;
	style->TabBorderSize = 0;

	style->Colors[ImGuiCol_Text] = ImVec4(0.80f, 0.80f, 0.83f, 1.00f);
	style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
	style->Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	style->Colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
	style->Colors[ImGuiCol_Border] = ImVec4(0.80f, 0.80f, 0.83f, 0.88f);
	style->Colors[ImGuiCol_BorderShadow] = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
	style->Colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
	style->Colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style->Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 0.98f, 0.95f, 0.75f);
	style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
	style->Colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
	style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	style->Colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
	style->Colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
	style->Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	style->Colors[ImGuiCol_Button] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
	style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style->Colors[ImGuiCol_Header] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style->Colors[ImGuiCol_HeaderHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style->Colors[ImGuiCol_HeaderActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	style->Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	style->Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style->Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	style->Colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
	style->Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
	style->Colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
	style->Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
	style->Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);

	ImVec4* colors = ImGui::GetStyle().Colors;

	colors[ImGuiCol_Tab] = ImVec4(0.146f, 0.113f, 0.146f, 0.86f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.364f, 0.205f, 0.366f, 0.80f);
	colors[ImGuiCol_TabActive] = ImVec4(51.0f / 255, 31.0f / 255, 49.0f / 255, 0.97f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(51.0f / 255, 31.0f / 255, 49.0f / 255, 0.57f);
	colors[ImGuiCol_Header] = ImVec4(0.61f, 0.61f, 0.62f, 0.22f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.61f, 0.62f, 0.62f, 0.51f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.61f, 0.62f, 0.62f, 0.83f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(43.0f / 255, 17.0f / 255, 43.0f / 255, 0.97f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.202f, 0.116f, 0.196f, 0.57f);
}

static void ShowError(std::string message)
{
	MessageBox(NULL, s2ws(message).c_str(), L"Error", 0);
}

static void* LoadDLL(std::string path)
{
	if (path.size() == 0)
	{
		ShowError("Invalid DLL Path!");
		return nullptr;
	}
	void* dll = LoadLibrary(s2ws(path).c_str());
	if (!dll)
		ShowError("Failed to Load DLL!");
	return dll;
}

static bool VerifyDLLFunc(void* dll, std::string functionName)
{
	void* func = GetProcAddress((HMODULE)dll, (functionName).c_str());
	if (!func)
		ShowError(functionName + " not found in DLL!");
	if (func && functionName == "GetModuleName")
	{
		char* name = ((char*(*)(void))func)();
		memcpy(moduleName, name, MAX(1024 * 4 - 1, strlen(name)));
	}
	return func;
}

static void CloseDLL(void* dll)
{
	FreeLibrary((HMODULE)dll);
}

static bool VerifyNodeDLL(std::string path) {
	void* dll = LoadDLL(path);
	if (!dll)
		return false;
	bool data = true;
	data = data && VerifyDLLFunc(dll, "GetModuleName");
	data = data && VerifyDLLFunc(dll, "GetModuleVersion");
	data = data && VerifyDLLFunc(dll, "VerifyUpdate");
	data = data && VerifyDLLFunc(dll, "LoadModule");
	data = data && VerifyDLLFunc(dll, "UnloadModule");
	data = data && VerifyDLLFunc(dll, "LoadNode");
	data = data && VerifyDLLFunc(dll, "RenderNode");
	data = data && VerifyDLLFunc(dll, "EvaluateNode");
	data = data && VerifyDLLFunc(dll, "GetNodeName");
	data = data && VerifyDLLFunc(dll, "SaveNodeData");
	data = data && VerifyDLLFunc(dll, "LoadNodeData");
	CloseDLL(dll);
	return data;
}

static bool VerifyNoiseLayerDLL(std::string path) {
	void* dll = LoadDLL(path);
	if (!dll)
		return false;
	bool data = true;
	data = data && VerifyDLLFunc(dll, "GetModuleName");
	data = data && VerifyDLLFunc(dll, "GetModuleVersion");
	data = data && VerifyDLLFunc(dll, "VerifyUpdate");
	data = data && VerifyDLLFunc(dll, "LoadModule");
	data = data && VerifyDLLFunc(dll, "UnloadModule");
	data = data && VerifyDLLFunc(dll, "LoadNoiseLayer");
	data = data && VerifyDLLFunc(dll, "RenderNoiseLayer");
	data = data && VerifyDLLFunc(dll, "EvaluateNoiseLayer");
	data = data && VerifyDLLFunc(dll, "SaveNoiseLayerData");
	data = data && VerifyDLLFunc(dll, "LoadNoiseLayerData");
	data = data && VerifyDLLFunc(dll, "GetNoiseLayerName");
	CloseDLL(dll);
	return data;
}

static bool VerifyUIDLL(std::string path) {
	void* dll = LoadDLL(path);
	if (!dll)
		return false;
	bool data = true;
	data = data && VerifyDLLFunc(dll, "GetModuleName");
	data = data && VerifyDLLFunc(dll, "GetModuleVersion");
	data = data && VerifyDLLFunc(dll, "VerifyUpdate");
	data = data && VerifyDLLFunc(dll, "LoadModule");
	data = data && VerifyDLLFunc(dll, "UnloadModule");
	data = data && VerifyDLLFunc(dll, "GetWindowName");
	data = data && VerifyDLLFunc(dll, "Render");
	CloseDLL(dll);
	return data;
}

static void CopyFileC(std::string from, std::string to)
{
	std::ifstream  src(from, std::ios::binary);
	std::ofstream  dst(to, std::ios::binary);
	dst << src.rdbuf();
	src.close();
	dst.close();
}

static void CopyFolderC(std::string from, std::string to)
{
	std::filesystem::copy(from, to);
}


static void DeleteFolderC(std::string from)
{
}

static void MakeModule()
{
	int moduleTypeC = 0;
	std::string dllPath = moduleDLLPath;
	if (dllPath.size() == 0)
	{
		ShowError("DLL Path not specified!");
		return;
	}

	std::string moduleNameS = moduleName;
	if (moduleNameS.size() == 0)
	{
		ShowError("Module Name not specified!");
		return;
	}

	std::string authorName = moduleAuthorName;
	if (authorName.size() == 0)
	{
		ShowError("Anuthor Name not specified!");
		return;
	}

	std::string liscense = moduleLiscenseFile;
	if (liscense.size() == 0)
	{
		ShowError("Liscense not specified!");
		return;
	}


	std::string rescFolder = moduleResourceFolder;
	if (rescFolder.size() == 0)
	{
		rescFolder = "";
	}

	std::string version = moduleVersion;
	if (version.size() == 0)
	{
		ShowError("Version not specified!");
		return;
	}

	std::string outputFile = moduleOutputPath;
	if (outputFile.size() == 0)
	{
		outputFile = "module.terr3dmodule";
	}

	if (isNoiseLayer)
		moduleTypeC++;

	if (isNode)
		moduleTypeC++;

	if (isUIHelper)
		moduleTypeC++;

	if (moduleTypeC == 0)
	{
		ShowError("Module type not selected!");
		return;
	}

	nlohmann::json data;
	data["type"] = "Terr3dModule";
	data["name"] = moduleNameS;
	data["author"] = authorName;
	data["filter"] = isFilter;
	data["node"] = isNode;
	data["noiselayer"] = isNoiseLayer;
	data["uihelper"] = isUIHelper;
	std::ifstream infile(liscense);
	std::stringstream liscD;
	liscD << infile.rdbuf();
	data["liscense"] = liscD.str();
	data["lang"] = 0;
	data["version"] = version;
	data["vid"] = moduleVersionInteger;
	nlohmann::json timeD;
	std::time_t time = std::time(0);
	timeD["dd"] = std::localtime(&time)->tm_mday;
	timeD["mm"] = std::localtime(&time)->tm_mon;
	timeD["yyyy"] = std::localtime(&time)->tm_year;
	timeD["ss"] = std::localtime(&time)->tm_sec;
	timeD["mm"] = std::localtime(&time)->tm_min;
	timeD["hh"] = std::localtime(&time)->tm_hour;
	data["creation_time"] = timeD;

	std::string info = data.dump(4);


	system("mkdir temp");

	std::ofstream ofile("temp\\data.terr3d");
	ofile << info;
	ofile.close();

	CopyFileC(dllPath, "temp\\module.dll");
	CopyFileC(liscense, "temp\\LISCENSE");
	if(rescFolder.size() > 0)
		CopyFolderC(rescFolder, "temp");

	zip_t* packed = zip_open(outputFile.c_str(), 9, 'w');
	zip_walk(packed, "temp");
	zip_close(packed);

	DeleteFolderC("temp");	
}

class MyApp : public Application
{
public:
	virtual void OnPreload() override {
		SetTitle("TerraForge3D Module Maker - Jaysmito Mukherjee");
		SetWindowConfigPath("windowconfigs.terr3d");
		Sleep(1000);
	}

	virtual void OnUpdate(float deltatime) override
	{
		RenderImGui();
	}

	virtual void OnOneSecondTick() override
	{
		
	}

	virtual void OnImGuiRender() override
	{
		OnBeforeImGuiRender();
		ImGui::Begin("Module Maker");
	    ImGui::Text("TerraForge3D Module Maker");
		ImGui::NewLine();

		ImGui::Text("Module Name :");
		ImGui::InputText("##Module Name", moduleName, sizeof(moduleName));
		ImGui::NewLine();
		
		ImGui::Text("Module Author Name :");
		ImGui::InputText("##Module Author Name", moduleAuthorName, sizeof(moduleAuthorName));
		ImGui::NewLine();

		ImGui::Text("Module Version :");
		ImGui::InputText("##Module Version", moduleVersion, sizeof(moduleVersion));
		ImGui::NewLine();

		ImGui::Text("Module Version Integer :");
		ImGui::InputInt("##Module Version Integer", &moduleVersionInteger);
		ImGui::NewLine();

		ImGui::Text("Module DLL File :");
		ImGui::InputText("##Module DLL", moduleDLLPath, sizeof(moduleDLLPath));
		ImGui::SameLine();
		if (ImGui::Button("Browse##0"))
		{
			std::string moduleDLLPathS = ShowOpenFileDialog(".dll");
			memcpy(moduleDLLPath, moduleDLLPathS.data(), moduleDLLPathS.size());
		}
		ImGui::NewLine();

		ImGui::Text("Module Resource Folder :");
		ImGui::InputText("##Module Res", moduleResourceFolder, sizeof(moduleResourceFolder));
		ImGui::SameLine();
		if (ImGui::Button("Browse##1"))
		{
			std::string moduleResourceFolderS = ShowOpenFileDialog("**.**");
			memcpy(moduleResourceFolder, moduleResourceFolderS.data(), moduleResourceFolderS.size());
		}
		ImGui::NewLine();

		ImGui::Text("Module Liscense File :");
		ImGui::InputText("##Module Lisc", moduleLiscenseFile, sizeof(moduleLiscenseFile));
		ImGui::SameLine();
		if (ImGui::Button("Browse##2"))
		{
			std::string moduleLiscenseFileS = ShowOpenFileDialog(".txt\0.md");
			memcpy(moduleLiscenseFile, moduleLiscenseFileS.data(), moduleLiscenseFileS.size());
		}
		ImGui::NewLine();

		ImGui::Text("Module Type");
		if (ImGui::Checkbox("Node Addon Module", &isNode))
			isNode = VerifyNodeDLL(std::string(moduleDLLPath));
		if(ImGui::Checkbox("Noise Layer Module", &isNoiseLayer))
			isNoiseLayer = VerifyNoiseLayerDLL(std::string(moduleDLLPath));
		if(ImGui::Checkbox("UI Module", &isUIHelper))
			isUIHelper = VerifyUIDLL(std::string(moduleDLLPath));
		ImGui::NewLine();

		ImGui::Text("Module Output File :");
		ImGui::InputText("##Module Output", moduleOutputPath, sizeof(moduleOutputPath));
		ImGui::SameLine();
		if (ImGui::Button("Browse##4"))
		{
			std::string moduleOutputPathS = ShowSaveFileDialog(".terr3dmodule");
			memcpy(moduleOutputPath, moduleOutputPathS.data(), moduleOutputPathS.size());
		}
		ImGui::NewLine();
		
		ImGui::NewLine();

		if (ImGui::Button("Make"))
			MakeModule();

		ImGui::End();
		OnImGuiRenderEnd();
	}

	virtual void OnStart(std::string loadFile) override
	{
		srand((unsigned int)time(NULL));
		GetWindow()->SetShouldCloseCallback(OnAppClose);
		SetupStyle();

		moduleDLLPath[0] = '\0';
		moduleOutputPath[0] = '\0';
		moduleResourceFolder[0] = '\0';
		moduleName[0] = '\0';
		moduleAuthorName[0] = '\0';
		moduleVersion[0] = '\0';
		moduleLiscenseFile[0] = '\0';
		moduleVersionInteger = 0;
		moduleLanguage = 0;
		moduleLanguageStr = moduleLanguages[moduleLanguage];
		isFilter = isNode = isNoiseLayer = isUIHelper = false;
	}

	void OnEnd()
	{
		
	}
};

void OnAppClose(int x, int y)
{
	myApp->Close();
}


Application* CreateApplication()
{
	myApp = new MyApp();
	return myApp;
}