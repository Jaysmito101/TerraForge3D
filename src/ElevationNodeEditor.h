#pragma once

#include <json.hpp>

float GetElevation(float x, float y);
nlohmann::json GetElevationNodeEditorSaveData();

void SetupElevationManager(int* resolution);
void ShutdownElevationNodeEditor();
void ShowElevationNodeEditor(bool* pOpen);
void SetElevationNodeEditorSaveData(nlohmann::json data);