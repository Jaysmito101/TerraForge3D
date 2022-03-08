#pragma once

#include <string>
#include "json.hpp"

MODULE_API void OnNoiseLayerLoad();

MODULE_API void OnNoiseLayerRender();

MODULE_API float OnNoiseLayerEvaluate(float x, float y, float z);

MODULE_API std::string NoiseLayerName();

MODULE_API nlohmann::json OnNoiseLayerDataSave();

MODULE_API void OnNoiseLayerDataLoad(nlohmann::json data);