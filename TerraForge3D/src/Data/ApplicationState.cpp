#include "ApplicationState.h"

static ApplicationState* appState = nullptr;

void SetUpApplicationState()
{
	appState = new ApplicationState();
}

void DeleteApplicationState()
{
	delete appState;
}

ApplicationState* GetApplicationState()
{
	return appState;
}

ApplicationState::ApplicationState()
{

}

ApplicationState::~ApplicationState()
{

}

ApplicationStateModels::ApplicationStateModels()
{
	coreTerrain = new Model("Terrain");
	grid = new Model("Grid");
	screenQuad = new Model("Sreen Quad");

	customBase = nullptr;
	customBaseCopy = nullptr;
}

ApplicationStateModels::~ApplicationStateModels()
{
	delete coreTerrain;
	delete grid;
	delete screenQuad;

	if (customBase)
		delete customBase;

	if (customBaseCopy)
		delete customBaseCopy;
}

nlohmann::json ApplicationStateCameras::Save()
{
	nlohmann::json data = nlohmann::json();
	data["main"] = appState->cameras.main.Save();
	data["postProcess"] = appState->cameras.postPorcess.Save();
	return data;
}

void ApplicationStateCameras::Load(nlohmann::json data)
{
	main.Load(data["main"]);
	postPorcess.Load(data["postProcess"]);
}

nlohmann::json ApplicationStateWindows::Save()
{
	nlohmann::json data;

	data["styleEditor"] = styleEditor;
	data["statsWindow"] = statsWindow;
	data["shaderEditor"] = shaderEditorWindow;
	data["textureEditor"] = texturEditorWindow;
	data["seaEditor"] = seaEditor;
	data["textureStore"] = textureStore;
	data["osLiscence"] = osLisc;
	data["filterManager"] = filtersManager;
	data["foliageManager"] = foliageManager;
	data["supportersTribute"] = supportersTribute;
	data["skySettings"] = skySettings;
	data["modulesManager"] = modulesManager;
	data["lightControls"] = lightControls;
	data["cameraControls"] = cameraControls;

	return data;
}

void ApplicationStateWindows::Load(nlohmann::json data)
{
	styleEditor = data["styleEditor"];
	statsWindow = data["statsWindow"];
	shaderEditorWindow = data["shaderEditor"];
	texturEditorWindow = data["textureEditor"];
	seaEditor = data["seaEditor"];
	textureStore = data["textureStore"];
	osLisc = data["osLiscence"];
	filtersManager = data["filterManager"];
	foliageManager = data["foliageManager"];
	supportersTribute = data["supportersTribute"];
	skySettings = data["skySettings"];
	modulesManager = data["modulesManager"];
	lightControls = data["lightControls"];
	cameraControls = data["cameraControls"];
}

nlohmann::json ApplicationStateStates::Save()
{
	nlohmann::json data;

	data["usingBase"] = usingBase;
	data["skyboxEnabled"] = skyboxEnabled;
	data["vSync"] = vSync;
	data["autoUpdate"] = autoUpdate;
	data["wirframeMode"] = wireFrameMode;
	data["autoSave"] = autoSave;
	data["explorerMode"] = exploreMode;
	data["iExplorerMode"] = iExploreMode;
	data["showFoliage"] = showFoliage;
	data["textureBake"] = textureBake;
	data["postProcess"] = postProcess;
	data["autoAspectCalcRatio"] = autoAspectCalcRatio;
	data["useGPUForNormals"] = useGPUForNormals;
	return data;
}

void ApplicationStateStates::Load(nlohmann::json data)
{
	usingBase = data["usingBase"];
	skyboxEnabled = data["skyboxEnabled"];
	vSync = data["vSync"];
	autoUpdate = data["autoUpdate"];
	wireFrameMode = data["wirframeMode"];
	autoSave = data["autoSave"];
	exploreMode = data["explorerMode"];
	iExploreMode = data["iExplorerMode"];
	showFoliage = data["showFoliage"];
	textureBake = data["textureBake"];
	postProcess = data["postProcess"];
	autoAspectCalcRatio = data["autoAspectCalcRatio"];
	useGPUForNormals = data["useGPUForNormals"];
}

nlohmann::json ApplicationStateGlobals::Save()
{
	nlohmann::json data;

	data["mouseSpeed"] = mouseSpeed;
	data["scrollSpeed"] = scrollSpeed;
	data["mouseScrollAmount"] = mouseScrollAmount;
	data["viewportMousePosX"] = viewportMousePosX;
	data["viewportMousePosY"] = viewportMousePosY;
	data["scale"] = scale;
	data["resolution"] = resolution;
	data["openFilePath"] = currentOpenFilePath;
	data["customBaseModelPath"] = currentBaseModelPath;
	data["offsetX"] = offset[0];
	data["offsetY"] = offset[1];
	data["offsetZ"] = offset[2];
	data["textureBakeMode"] = textureBakeMode;
	data["hMapCX"] = hMapC[0];
	data["hMapCY"] = hMapC[1];
	data["hMapCZ"] = hMapC[2];
	data["hMapCW"] = hMapC[3];
	return data;
}

void ApplicationStateGlobals::Load(nlohmann::json data)
{
	mouseSpeed = data["mouseSpeed"];
	scrollSpeed = data["scrollSpeed"];
	mouseScrollAmount = data["mouseScrollAmount"];
	viewportMousePosX = data["viewportMousePosX"];
	viewportMousePosY = data["viewportMousePosY"];
	scale = data["scale"];
	resolution = data["resolution"];
	currentOpenFilePath = data["openFilePath"];
	currentBaseModelPath = data["customBaseModelPath"];
	offset[0] = data["offsetX"];
	offset[1] = data["offsetY"];
	offset[2] = data["offsetZ"];
	textureBakeMode = data["textureBakeMode"];
	hMapC[0] = data["hMapCX"];
	hMapC[1] = data["hMapCY"];
	hMapC[2] = data["hMapCZ"];
	hMapC[3] = data["hMapCW"];
}
