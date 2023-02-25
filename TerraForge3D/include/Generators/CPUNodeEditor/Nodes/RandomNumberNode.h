#pragma once

#include <vector>
#include <random>

#ifndef _WIN32
#include<bits/stdc++.h>
#endif

#include "Base/NodeEditor/NodeEditor.h"

#ifdef min 
#undef min
#endif
#ifdef max
#undef max
#endif


class RandomNumberNode : public NodeEditorNode
{
public:


	virtual NodeOutput Evaluate(NodeInputParam input, NodeEditorPin *pin);

	virtual void Load(nlohmann::json data);
	virtual nlohmann::json Save();
	virtual void OnRender();

	RandomNumberNode();

	int seed, min, max;
	std::mt19937 engine;
};


