#pragma once

#include "Base/NodeEditor/NodeEditor.h"
#include <vector>

/*
*
* Warning : This node is may not have any real value!
* Its made mainly for debugging and fun!
*
*/


class VisualizerNode : public NodeEditorNode
{
public:


	virtual NodeOutput Evaluate(NodeInputParam input, NodeEditorPin *pin);

	virtual void Load(nlohmann::json data);
	virtual nlohmann::json Save();
	virtual void OnRender();

	VisualizerNode();

	std::vector<float> map;
	NodeInputParam inputC;
	std::mutex mutex;
};

