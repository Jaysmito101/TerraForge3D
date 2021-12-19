#pragma once

#include "ModuleAPI.h"



extern "C" MODULE_API void LoadNoiseLayer();

extern "C" MODULE_API void RenderNoiseLayer(void* imguiContext);

extern "C" MODULE_API float EvaluateNoiseLayer(float x, float y, float z);

extern "C" MODULE_API char* GetNoiseLayerName();

extern "C" MODULE_API char* SaveNoiseLayerData();

extern "C" MODULE_API void LoadNoiseLayerData(char* data);