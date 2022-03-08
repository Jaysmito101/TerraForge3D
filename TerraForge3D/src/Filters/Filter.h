#pragma once
#include <Model.h>
#include <json.hpp>

class ApplicationState;

class Filter
{
public:
	Filter(ApplicationState *appState, std::string name = "Filter")
		:name(name), appState(appState) {}

	virtual void Load(nlohmann::json data) = 0;
	virtual nlohmann::json Save() = 0;
	virtual void Render() = 0;
	virtual void Apply() = 0;
	virtual void OnAttach() {};

	ApplicationState *appState;
	std::string name;
};