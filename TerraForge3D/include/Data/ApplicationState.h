#pragma once

#include "Base/Base.h"

#include "Menu/MainMenu.h"
#include "TextureStore/TextureStore.h"
#include "Misc/SupportersTribute.h"
#include "Misc/OSLiscences.h"
#include "Exporters/ExportManager.h"
#include "Misc/ViewportManager.h"
#include "Renderer/RendererManager.h"
#include "Misc/Dashboard.h"
#include "Misc/Style.h"
#include "Platform.h"
#include "Job/Job.h"
#include "Job/JobSystem.h"
#include "Job/JobManager.h"


#include "Generators/GeneratorData.h"
#include "Generators/GenerationManager.h"


#include "json.hpp"

#ifndef MAX_VIEWPORT_COUNT
#define MAX_VIEWPORT_COUNT 8
#endif

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
	bool autoSave = false;
	std::atomic<bool> ruinning = true;
	nlohmann::json Save();
	void Load(nlohmann::json data);
};

struct ApplicationStateGlobals
{
	int secondCounter = 0;
	nlohmann::json appData;
	std::string currentOpenFilePath = "";
	std::string currentBaseModelPath = "";
	std::string kernelsIncludeDir = "";
	
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
	std::string fontsDir = "";
	std::string liscensesDir = "";
	std::string modelsDir = "";
	std::string configsDir = "";
	std::string logsDir = "";
	std::string stylesDir = "";

	int gpuWorkgroupSize = 16;
};

class ApplicationState
{
public:
	Application *mainApp;

	ApplicationStateWindows windows;
	ApplicationStateStates states;
	ApplicationStateGlobals globals;
	ApplicationStateConstants constants;

	MainMenu *mainMenu = nullptr;
	TextureStore *textureStore = nullptr;
	SupportersTribute *supportersTribute = nullptr;
	OSLiscences *osLiscences = nullptr;
	ProjectManager *projectManager = nullptr;
	ExportManager* exportManager = nullptr;
	Model* mainModel = nullptr;
	RendererManager* rendererManager = nullptr;
	Dashboard* dashboard = nullptr;
	ViewportManager* viewportManagers[MAX_VIEWPORT_COUNT];
	Style* styleManager = nullptr;
	GenerationManager* generationManager = nullptr;
	JobSystem::JobSystem* jobSystem = nullptr;
	JobSystem::JobManager* jobManager = nullptr;
	EventManager* eventManager = nullptr;

	struct
	{
		int mapResolution, tileResolution, tileCount, currentTileX, currentTileY;
		float tileSize, tileOffsetX, tileOffsetY;
	} mainMap;


	ApplicationState();
	~ApplicationState();
};
