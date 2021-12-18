#pragma once

#include "json.hpp"

struct NodeOutput
{
	float value;
};

struct NodeInputParam
{
	float x;
	float y;
	float z;

	float texX;
	float texY;

	float minX;
	float minY;
	float minZ;

	float maxX;
	float maxY;
	float maxZ;

	NodeInputParam(float* pos, float* texCoord, float* minPos, float* maxPos);
};

MODULE_API void OnNodeLoad();

MODULE_API void OnNodeRender();

MODULE_API NodeOutput OnNodeEvaluate(NodeInputParam input);

MODULE_API nlohmann::json OnNodeDataSave();

MODULE_API void OnNodeDataLoad(nlohmann::json data);