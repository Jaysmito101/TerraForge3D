#pragma once

#include "Terr3DModule.h"
#include "json.hpp"

struct NodeInputParam;

class NodeModule : public Module
{
public:
	NodeModule(std::string id, ModuleManager *manager);

	void Render();
	nlohmann::json Save();
	void Load(nlohmann::json data);
	float Evaluate(NodeInputParam input);

	std::string name;
	bool active;
};