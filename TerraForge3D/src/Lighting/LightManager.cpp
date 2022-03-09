#include "Lighting/LightManager.h"
#include "imgui/imgui.h"

#include "glm/gtc/type_ptr.hpp"

#include <string>

static int ID = 0;

LightManager::LightManager()
{
	lightManagerID = ID++;
	position.y = -0.3f;
	position.x = position.z = 0.0f;
	color[0] = color[1] = color[2] = color[3] = 0.8;
	ambientColor[0] = ambientColor[1] = ambientColor[2] = ambientColor[3] = 0.8;
	strength = 50.0f;
}

LightManager::~LightManager()
{
}

void LightManager::ShowSettings(bool renderWindow, bool *pOpen)
{
	if (*pOpen)
	{
		if (renderWindow)
		{
			ImGui::Begin(("Light Setting##" + std::to_string(lightManagerID)).c_str(), pOpen);
		}

		ImGui::Text("Strength");
		ImGui::DragFloat("##lightStrength", &strength, 0.1f);
		ImGui::Text("Position");
		ImGui::DragFloat3("##lightPosition", &position[0], 0.01f);
		ImGui::Separator();
		ImGui::Separator();
		ImGui::Text("Light Color");
		ImGui::ColorEdit3("##lightColor", color);

		if (renderWindow)
		{
			ImGui::End();
		}
	}
}

nlohmann::json LightManager::Save()
{
	nlohmann::json data, tmp;
	tmp["x"] = position.x;
	tmp["y"] = position.y;
	tmp["z"] = position.z;
	data["position"] = tmp;
	tmp = nlohmann::json();
	tmp["r"] = color[0];
	tmp["g"] = color[1];
	tmp["b"] = color[2];
	tmp["a"] = color[3];
	data["color"] = tmp;
	data["strength"] = strength;
	return data;
}

void LightManager::Load(nlohmann::json data)
{
	position[0] = data["position"]["x"];
	position[1] = data["position"]["y"];
	position[2] = data["position"]["z"];
	color[0] = data["color"]["r"];
	color[1] = data["color"]["g"];
	color[2] = data["color"]["b"];
	color[3] = data["color"]["a"];
	strength = data["strength"];
}
