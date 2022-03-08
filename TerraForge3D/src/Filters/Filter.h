#pragma once
#include <Model.h>
#include <json.hpp>

class Filter
{
public:
	Filter(Model *model, std::string name = "Filter")
		:name(name), model(model) {}

	virtual void Load(nlohmann::json data) = 0;
	virtual nlohmann::json Save() = 0;
	virtual void Render() = 0;
	virtual void Apply() = 0;
	virtual void OnAttach() {};

	Model *model;
	std::string name;
};