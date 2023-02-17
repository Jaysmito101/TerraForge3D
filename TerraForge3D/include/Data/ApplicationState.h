#pragma once

#include "Base/Base.h"

#include "Data/ProjectData.h"
#include "Data/Serializer.h"
#include "Menu/MainMenu.h"
#include "Generators/MeshGeneratorManager.h"
#include "TextureStore/TextureStore.h"
#include "Misc/SupportersTribute.h"
#include "Misc/OSLiscences.h"
#include "Exporters/ExportManager.h"
#include "Misc/ViewportManager.h"
#include "Renderer/RendererManager.h"
#include "Platform.h"

#include "json/json.hpp"

#ifndef MAX_VIEWPORT_COUNT
#define MAX_VIEWPORT_COUNT 8
#endif

struct ApplicationStateStatistics
{
	double deltatime = 1;
	double frameRate = 1;
};

struct ApplicationStateWindows
{
	bool styleEditor = false;
	bool textureStore = false;
	bool osLisc = false;
	bool supportersTribute = false;

	nlohmann::json Save();
	void Load(nlohmann::json data);
};

struct ApplicationStateStates
{
	bool usingBase = true;
	bool skyboxEnabled = false;
	bool vSync = true;
	bool mouseButton1, mouseButton2, mouseButton3;
	bool autoSave = false;
	bool showFoliage = true;
	bool useGPUForNormals = false;
	bool postProcess = false;
	bool autoAspectCalcRatio = true;
	std::atomic<bool> ruinning = true;
	std::atomic<bool> remeshing = false;
	std::atomic<bool> pauseUpdation = false;
	std::atomic<bool> requireRemesh = false;

	nlohmann::json Save();
	void Load(nlohmann::json data);
};

struct ApplicationStateGlobals
{
	float mouseSpeed = 25;
	float scrollSpeed = 0.5f;
	float mouseScrollAmount = 0;
	float viewportMousePosX = 0;
	float viewportMousePosY = 0;

	int numberOfNoiseTypes = 3;
	int secondCounter = 0;
	int cpuWorkerThreadsActive = 1;

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

class ApplicationState
{
public:
	Application *mainApp;

	ApplicationStateStatistics stats;
	ApplicationStateWindows windows;
	ApplicationStateStates states;
	ApplicationStateGlobals globals;
	ApplicationStateConstants constants;

	Serializer *serailizer = nullptr;
	MeshGeneratorManager *meshGenerator = nullptr;
	MainMenu *mainMenu = nullptr;
	TextureStore *textureStore = nullptr;
	SupportersTribute *supportersTribute = nullptr;
	OSLiscences *osLiscences = nullptr;
	ProjectManager *projectManager = nullptr;
	ExportManager* exportManager = nullptr;
	Model* mainModel;
	ViewportManager* viewportManagers[MAX_VIEWPORT_COUNT];
	RendererManager* rendererManager;

	struct
	{
		DataTexture* currentTileDataLayers[6];
		int mapResolution, tileResolution, tileCount, currentTileX, currentTileY;
		float tileSize, tileOffsetX, tileOffsetY;
	} mainMap;


	ApplicationState();
	~ApplicationState();
};
