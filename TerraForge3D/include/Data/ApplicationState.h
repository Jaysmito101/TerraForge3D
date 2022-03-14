#pragma once

#include "Base/Base.h"

#include "Data/ProjectData.h"
#include "Modules/ModuleManager.h"
#include "Sea/SeaManager.h"
#include "Lighting/LightManager.h"
#include "Data/Serializer.h"
#include "Menu/MainMenu.h"
#include "Generators/MeshGeneratorManager.h"
#include "TextureStore/TextureStore.h"
#include "Misc/SupportersTribute.h"
#include "Filters/FiltersManager.h"
#include "Foliage/FoliagePlacement.h"
#include "Sky/SkySettings.h"
#include "Misc/OSLiscences.h"

#include "json/json.hpp"

struct ApplicationStateModels
{
	Model *coreTerrain;
	Model *grid; // For future use
	Model *screenQuad;

	Model *customBase;
	Model *customBaseCopy;

	ApplicationStateModels();
	~ApplicationStateModels();
};

struct ApplicationStateFrameBuffers
{
	FrameBuffer *reflection;
	FrameBuffer *textureExport;
	FrameBuffer *postProcess;
	FrameBuffer *main;
	FrameBuffer *texBakeMain = nullptr;
};

struct ApplicationStateShaders
{
	Shader *terrain = nullptr;
	Shader *wireframe = nullptr;
	Shader *textureBake = nullptr;
	Shader *foliage = nullptr;
	Shader *postProcess = nullptr;

	// For future use
	Shader *meshNormals = nullptr;
};

struct ApplicationStateCameras
{
	Camera main;
	Camera postPorcess;

	nlohmann::json Save();
	void Load(nlohmann::json data);
};

struct ApplicationStateStatistics
{
	double deltatime = 1;
	double frameRate = 1;
	double meshGenerationTime = 0;
	int triangles = 0;
	int vertexCount = 0;
};

struct ApplicationStateWindows
{
	bool styleEditor = false;
	bool statsWindow = false;
	bool shaderEditorWindow = false;
	bool texturEditorWindow = false;
	bool seaEditor = false;
	bool textureStore = false;
	bool osLisc = false;
	bool filtersManager = false;
	bool foliageManager = false;
	bool supportersTribute = false;
	bool skySettings = false;
	bool modulesManager = false;
	bool lightControls = true;
	bool cameraControls = true;


	nlohmann::json Save();
	void Load(nlohmann::json data);
};

struct ApplicationStateModules
{
	ModuleManager *manager;
};

struct ApplicationStateStates
{
	bool usingBase = true;
	bool skyboxEnabled = false;
	bool vSync = true;
	bool autoUpdate = false;
	bool mouseButton1, mouseButton2, mouseButton3;
	bool wireFrameMode = false;
	bool reqTexRfrsh = false;
	bool autoSave = false;
	bool exploreMode = false;
	bool iExploreMode = false;
	bool showFoliage = true;
	bool textureBake = false;
	bool useGPUForNormals = false;
	bool postProcess = false;
	bool autoAspectCalcRatio = true;
	std::atomic<bool> ruinning = true;
	std::atomic<bool> remeshing = false;

	nlohmann::json Save();
	void Load(nlohmann::json data);
};

struct ApplicationStateTextures
{
	Texture2D *grid;
};

struct ApplicationStateGlobals
{
	float mouseSpeed = 25;
	float scrollSpeed = 0.5f;
	float mouseScrollAmount = 0;
	float viewportMousePosX = 0;
	float viewportMousePosY = 0;
	float scale = 1.0f;
	float offset[3];


	int resolution = 256;
	int numberOfNoiseTypes = 3;
	int secondCounter = 0;
	int textureBakeMode = 0;
	int texBakeRes = 1024;

	nlohmann::json appData;

	std::string currentOpenFilePath = "";
	std::string currentBaseModelPath = "";
	std::string kernelsIncludeDir = "";

	float viewportSize[4];
	float hMapC[4];

	nlohmann::json Save();
	void Load(nlohmann::json data);
};

struct ApplicationStateConstants
{
	glm::vec3 UP			 = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 DOWN			 = glm::vec3(0.0f, -1.0f, 0.0f);
	glm::vec3 FRONT			 = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 BACK			 = glm::vec3(0.0f, 1.0f, 1.0f);

	std::string executableDir = "";
	std::string dataDir = "";
	std::string cacheDir = "";
	std::string texturesDir = "";
	std::string projectsDir = "";
	std::string tempDir = "";
	std::string shadersDir = "";
	std::string kernelsDir = "";
	std::string fontsDir = "";
	std::string liscensesDir = "";
	std::string skyboxDir = "";
	std::string modulesDir = "";
	std::string modelsDir = "";
	std::string configsDir = "";
	std::string logsDir = "";

};

enum ApplicationMode
{
	TERRAIN = 0,
	CUSTOM_BASE,
	CUBE_MARCHED
};

struct ApplicationState
{
	Application *mainApp;

	ApplicationStateModels models;
	ApplicationStateFrameBuffers frameBuffers;
	ApplicationStateShaders shaders;
	ApplicationStateCameras cameras;
	ApplicationStateStatistics stats;
	ApplicationStateWindows windows;
	ApplicationStateModules modules;
	ApplicationStateStates states;
	ApplicationStateTextures textures;
	ApplicationStateGlobals globals;
	ApplicationStateConstants constants;

	SeaManager *seaManager = nullptr;
	LightManager *lightManager = nullptr;
	Serializer *serailizer = nullptr;
	MeshGeneratorManager *meshGenerator = nullptr;
	MainMenu *mainMenu = nullptr;
	TextureStore *textureStore = nullptr;
	SupportersTribute *supportersTribute = nullptr;
	FiltersManager *filtersManager = nullptr;
	SkyManager *skyManager = nullptr;
	OSLiscences *osLiscences = nullptr;
	ProjectManager *projectManager = nullptr;
	FoliageManager *foliageManager = nullptr;

	ApplicationMode mode = ApplicationMode::TERRAIN;


	ApplicationState();

	~ApplicationState();
};
