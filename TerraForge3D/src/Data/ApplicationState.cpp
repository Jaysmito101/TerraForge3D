#include "Data/ApplicationState.h"

ApplicationState::ApplicationState()
{
}

ApplicationState::~ApplicationState()
{
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
