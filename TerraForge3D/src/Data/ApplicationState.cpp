#include "Data/ApplicationState.h"

ApplicationState::ApplicationState()
{
}

ApplicationState::~ApplicationState()
{
}

ApplicationStateModels::ApplicationStateModels()
{
	mainModel = new Model("MainModel");
	screenQuad = new Model("Screen Quad");
}

ApplicationStateModels::~ApplicationStateModels()
{
	delete mainModel;
	delete screenQuad;
}

nlohmann::json ApplicationStateCameras::Save()
{
	nlohmann::json data = nlohmann::json();
	data["main"] = main.Save();
	return data;
}

void ApplicationStateCameras::Load(nlohmann::json data)
{
	main.Load(data["main"]);
}

nlohmann::json ApplicationStateWindows::Save()
{
	nlohmann::json data;
	return data;
}

void ApplicationStateWindows::Load(nlohmann::json data)
{
}

nlohmann::json ApplicationStateStates::Save()
{
	nlohmann::json data;
	return data;
}

void ApplicationStateStates::Load(nlohmann::json data)
{
}

nlohmann::json ApplicationStateGlobals::Save()
{
	nlohmann::json data;
	return data;
}

void ApplicationStateGlobals::Load(nlohmann::json data)
{
	
}
