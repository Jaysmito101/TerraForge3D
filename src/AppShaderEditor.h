#pragma once
#include <imgui.h>
#include <string>

void ShowShaderEditor(bool* pOpen);

void SecondlyShaderEditorUpdate();
void SetupShaderManager();
bool ReqRefresh();
std::string GetVertexShaderSource();
std::string GetFragmentShaderSource();
std::string GetGeometryShaderSource();
std::string GetWireframeGeometryShaderSource();