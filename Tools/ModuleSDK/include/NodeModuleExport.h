#pragma once

#include "ModuleAPI.h"

struct NodeInputParam;


extern "C" MODULE_API void LoadNode();

extern "C" MODULE_API void RenderNode(void* imguiContext);

extern "C" MODULE_API float EvaluateNode(NodeInputParam input);

extern "C" MODULE_API char* GetNodeName();

extern "C" MODULE_API char* SaveNodeData();

extern "C" MODULE_API void LoadNodeData(char* data);