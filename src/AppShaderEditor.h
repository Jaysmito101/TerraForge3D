#pragma once
#include <imgui.h>
#include <string>

void ShowShaderEditor(bool* pOpen);

void SecondlyShaderEditorUpdate();
void SetupShaderManager(std::string execDir);
bool ReqRefresh();
std::string GetDefaultVertexShaderSource();
std::string GetVertexShaderSource();
std::string GetDefaultFragmentShaderSource();
std::string GetFragmentShaderSource();
std::string GetGeometryShaderSource();
std::string GetWireframeGeometryShaderSource();