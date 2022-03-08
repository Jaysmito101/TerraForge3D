#pragma once

#include "Base/NodeEditor/NodeEditor.h"
#include <vector>
#include "Modules/ModuleManager.h"

class ModuleNode : public NodeEditorNode
{
public:


	virtual NodeOutput Evaluate(NodeInputParam input, NodeEditorPin *pin);

	virtual void Load(nlohmann::json data);
	virtual nlohmann::json Save();
	virtual void OnRender();

	ModuleNode(std::string id, NodeModule *mod);

	NodeModule *mod;
	std::string id;
};

