#include "NoiseLayers/LayeredNoiseManager.h"

#include "imgui.h"

#include <iostream>

#define ADD_ST_CH(x) stateChanged = x || stateChanged;
#define MAKE_IMGUI_ID(x) ("##LayeredNoiseManager" + std::string(x)).c_str()


LayeredNoiseManager::LayeredNoiseManager()
{
	offset[0] = offset[1] = offset[2] = 0;
	strength = 1.0f;
	absv = false;
	sq = false;
	noiseLayers.push_back(new NoiseLayer());
}

bool LayeredNoiseManager::Render()
{
	bool stateChanged = false;
	ImGui::Text("Global Offsets:");
	ADD_ST_CH(ImGui::DragFloat3(MAKE_IMGUI_ID("Global Offsets"), offset, 0.01f));
	ADD_ST_CH(ImGui::DragFloat("Global Strength##LayeredNoiseManager", &strength, 0.01f));
	ADD_ST_CH(ImGui::Checkbox("Absolute Value##LayeredNoiseManager", &absv));
	ADD_ST_CH(ImGui::Checkbox("Square Value##LayeredNoiseManager", &sq));
	ImGui::NewLine();
	ImGui::Text("Noise Layers");
	std::vector<NoiseLayer *> nl = noiseLayers;
	for (int i = 0; i < nl.size(); i++)
	{
		bool state = ImGui::CollapsingHeader((std::string("##noiseLayerName") + std::to_string(i)).c_str());
		if (state)
		{
			ADD_ST_CH(nl[i]->Render(i));
			if (toAdd.size() == 0)
			{
				if (ImGui::Button("Duplicate"))
				{
					nlohmann::json data = nl[i]->Save();
					toAdd.push_back(new NoiseLayer());
					toAdd.back()->Load(data);
				}
			}

			if (noiseLayers.size() > 1 && toDelete.size() == 0)
			{
				ImGui::SameLine();
				if (ImGui::Button("Delete")) toDelete.push_back(i);
			}
		}

		else
		{
			ImGui::SameLine();
			ImGui::Text(nl[i]->name.data());
		}
	}

	if (toAdd.size() == 0)
	{
		if (ImGui::Button("Add New Layer"))
		{
			toAdd.push_back(new NoiseLayer());
		}
	}
	return stateChanged;
}

void LayeredNoiseManager::UpdateLayers()
{
	if (toDelete.size() > 0)
	{
		noiseLayers.erase(noiseLayers.begin() + toDelete[0]);
		toDelete.clear();
	}

	if (toAdd.size() > 0)
	{
		noiseLayers.push_back(toAdd[0]);
		toAdd.clear(); 
	}
}

void LayeredNoiseManager::Load(nlohmann::json data)
{
	sq = data["sq"];
	absv = data["absv"];
	offset[0] = data["offsetX"];
	offset[1] = data["offsetY"];
	offset[2] = data["offsetZ"];
	strength = data["strength"];
	mutex.lock();

	for (NoiseLayer *nl : noiseLayers)
	{
		delete nl;
	}

	noiseLayers.clear();

	for (nlohmann::json tmp : data["noiseLayers"])
	{
		noiseLayers.push_back(new NoiseLayer());
		noiseLayers.back()->Load(tmp);
	}

	mutex.unlock();
}

nlohmann::json LayeredNoiseManager::Save()
{
	nlohmann::json data, tmp, tmp2;
	mutex.lock();

	for (int i = 0; i < noiseLayers.size(); i++)
	{
		tmp2 = noiseLayers[i]->Save();
		tmp2["index"] = i;
		tmp.push_back(tmp2);
	}

	mutex.unlock();
	data["noiseLayers"] = tmp;
	data["sq"] = sq;
	data["absv"] = absv;
	data["offsetX"] = offset[0];
	data["offsetY"] = offset[1];
	data["offsetZ"] = offset[2];
	data["strength"] = strength;
	return data;
}

float LayeredNoiseManager::Evaluate(float x, float y, float z) const
{
	const std::vector<NoiseLayer*>& nl = noiseLayers;
	float noise = 0.0f;
	for (const NoiseLayer* n : nl) noise += n->Evaluate({ x + offset[0], y + offset[1], z + offset[2] });
	noise *= strength;
	if (absv) noise = abs(noise);
	if (sq) noise = noise * noise;
	return noise;
}
