#include "Generators/GPUNoiseLayerGenerator.h"
#include "Utils/Utils.h"
#include "Data/ApplicationState.h"
#include "Profiler.h"

#include <regex>

#ifndef max 
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#define ADD_ST_CH(x) stateChanged = x || stateChanged;

static int count = 1;

GPUNoiseLayerGenerator::GPUNoiseLayerGenerator(ApplicationState *as)
{
	bool tmp = false;
	appState = as;
	uid = GenerateId(32);
	name = "GPU Noise Layer " + std::to_string(count++);
	noiseLayers.push_back(GPUNoiseLayer());
	noiseLayers.push_back(GPUNoiseLayer()); 
	noiseLayers.push_back(GPUNoiseLayer()); 
}

void GPUNoiseLayerGenerator::Generate(OpenCLContext* kernels, int tx, int ty)
{  
	START_PROFILER();

	noiseLayers[0].strength = appState->mainMap.tileSize / appState->mainMap.tileResolution;
	noiseLayers[0].offsetX = appState->mainMap.tileOffsetX;
	noiseLayers[0].offsetY = appState->mainMap.tileOffsetY;
	noiseLayers[0].offsetZ = (float)appState->mainMap.tileResolution + 0.2f;
	noiseLayers[0].offsetW = (float)this->setMode + 0.2f;
	noiseLayers[0].valueX = (float)(appState->workManager->GetWorkResolution() + 0.1f); // tile size
	noiseLayers[0].valueY = appState->mainMap.tileResolution / noiseLayers[0].valueX; // tile count
	noiseLayers[0].valueZ = (float)tx; // tile x
	noiseLayers[0].valueW = (float)ty; // tile y

	kernels->CreateBuffer("noise_layers_data", CL_MEM_READ_WRITE, sizeof(GPUNoiseLayer) * noiseLayers.size());
	kernels->WriteBuffer("noise_layers_data", true, 0, sizeof(GPUNoiseLayer) * noiseLayers.size(), noiseLayers.data());
	kernels->CreateBuffer("noise_layers_size", CL_MEM_READ_WRITE, sizeof(int));
	int tmp = (int)noiseLayers.size(); kernels->WriteBuffer("noise_layers_size", true, 0, sizeof(int), &tmp);
	
	{		
		kernels->SetKernelArg("process_map_noise_layer", 0, "noise_layers_data");
		kernels->SetKernelArg("process_map_noise_layer", 1, "noise_layers_size");
		kernels->SetKernelArg("process_map_noise_layer", 2, "data_layer_0");
		kernels->SetKernelArg("process_map_noise_layer", 3, "data_layer_1");
		vc = appState->workManager->GetWorkResolution();
		int ls = max(localSize, 1); while (vc % ls != 0) ls--;
		kernels->ExecuteKernel("process_map_noise_layer", cl::NDRange(ls, ls), cl::NDRange(vc, vc));
	}

	END_PROFILER(time);
}

nlohmann::json GPUNoiseLayerGenerator::Save()
{
	nlohmann::json data;
	data["uid"] = uid;
	data["name"] = name;
	data["window"] = windowStat;
	data["enabled"] = enabled;
	data["ls"] = localSize;
	nlohmann::json tmp;

	for (int i = 0; i < noiseLayers.size(); i++)
	{
		tmp.push_back(SaveNoiseLayer(noiseLayers[i]));
	}

	data["nl"] = tmp;
	return data;
}

nlohmann::json GPUNoiseLayerGenerator::SaveNoiseLayer(GPUNoiseLayer nl)
{
	nlohmann::json data;
	data["octaves"] = nl.octaves;
	data["fractal"] = nl.fractal;
	data["frequency"] = nl.frequency;
	data["lacunarity"] = nl.lacunarity;
	data["gain"] = nl.gain;
	data["weightedStrength"] = nl.weightedStrength;
	data["pingPongStrength"] = nl.pingPongStrength;
	data["strength"] = nl.strength;
	data["offsetX"] = nl.offsetX;
	data["offsetY"] = nl.offsetY;
	data["offsetZ"] = nl.offsetZ;
	data["domainWrapDepth"] = nl.domainWrapDepth;
	return data;
}

GPUNoiseLayer GPUNoiseLayerGenerator::LoadNoiseLayer(nlohmann::json data)
{
	GPUNoiseLayer nl;
	nl.octaves = data["octaves"];
	nl.fractal = data["fractal"];
	nl.frequency = data["frequency"];
	nl.lacunarity = data["lacunarity"];
	nl.gain = data["gain"];
	nl.weightedStrength = data["weightedStrength"];
	nl.pingPongStrength = data["pingPongStrength"];
	nl.strength = data["strength"];
	nl.offsetX = data["offsetX"];
	nl.offsetY = data["offsetY"];
	nl.offsetZ = data["offsetZ"];
	nl.domainWrapDepth = data["domainWrapDepth"];
	return nl;
}


void GPUNoiseLayerGenerator::Load(nlohmann::json data)
{
	uid = data["uid"];
	name = data["name"];
	windowStat = data["window"];
	enabled = data["enabled"];
	localSize = data["ls"];

	appState->workManager->WaitForFinish();

	noiseLayers.clear();
	nlohmann::json tmp = data["nl"];

	for (int i = 0; i < tmp.size(); i++)
	{
		noiseLayers.push_back(LoadNoiseLayer(tmp[i]));
	}
}

bool GPUNoiseLayerGenerator::ShowSetting(int id)
{
	bool stateChanged = false;
	ImGui::PushID(id);

	ADD_ST_CH(ImGui::Checkbox("Enabled##GPUNL", &enabled));
	ADD_ST_CH(ImGui::DragInt("Local Work Group Size##GPUNL", &localSize));

	if (ImGui::Button("Edit Layers##GPUNL")) windowStat = true;

	ImGui::PopID();

	ImGui::Text("Vertices Processed : %d", vc);
	ImGui::Text("Time : %lf ms", time);
	return stateChanged;
}

bool GPUNoiseLayerGenerator::Update()
{
	bool stateChanged = false;
	if (windowStat)
	{
		ImGui::Begin((name + "##" + uid).c_str(), &windowStat);
		ADD_ST_CH(ShowLayerUpdationMethod("Set Method##GPUNL", &this->setMode));
		ImGui::Text("Global Settings");
		ADD_ST_CH(ImGui::DragFloat("Offset X", &noiseLayers[1].offsetX, 0.01f));
		ADD_ST_CH(ImGui::DragFloat("Offset Y", &noiseLayers[1].offsetY, 0.01f));
		ADD_ST_CH(ImGui::DragFloat("Offset Z", &noiseLayers[1].offsetZ, 0.01f));
		ADD_ST_CH(ImGui::DragFloat("Strength", &noiseLayers[1].strength, 0.01f));
		ADD_ST_CH(ImGui::DragFloat("Frequency", &noiseLayers[1].frequency, 0.001f));
		ImGui::Separator();

		for (int i = 2; i < noiseLayers.size(); i++)
		{
			ImGui::PushID(i);
			if (ImGui::CollapsingHeader(("Noise Layer " + std::to_string(i + 1)).c_str()))
			{
				ImGui::Text("Noise Layer %d", (i + 1));
				ADD_ST_CH(ImGui::DragFloat("Offset X##GPUNL", &noiseLayers[i].offsetX, 0.01f));
				ADD_ST_CH(ImGui::DragFloat("Offset Y##GPUNL", &noiseLayers[i].offsetY, 0.01f));
				ADD_ST_CH(ImGui::DragFloat("Offset Z##GPUNL", &noiseLayers[i].offsetZ, 0.01f));
				ADD_ST_CH(ImGui::DragFloat("Strength##GPUNL", &noiseLayers[i].strength, 0.01f));
				ADD_ST_CH(ImGui::DragFloat("Frequency##GPUNL", &noiseLayers[i].frequency, 0.001f));
				ADD_ST_CH(ImGui::DragFloat("Domain Wrap Depth##GPUNL", &noiseLayers[i].domainWrapDepth, 0.1f));

				if (noiseLayers[i].fractal != 0)
				{
					ADD_ST_CH(ImGui::DragFloat("Octaves##GPUNL", &noiseLayers[i].octaves, 0.01f, 1, 128));
					ADD_ST_CH(ImGui::DragFloat("Lacunarity##GPUNL", &noiseLayers[i].lacunarity, 0.01f));
					ADD_ST_CH(ImGui::DragFloat("Gain##GPUNL", &noiseLayers[i].gain, 0.01f));
					ADD_ST_CH(ImGui::DragFloat("Weighted Strength##GPUNL", &noiseLayers[i].weightedStrength, 0.01f));
					ADD_ST_CH(ImGui::DragFloat("Ping Pong Strength##GPUNL", &noiseLayers[i].pingPongStrength, 0.01f));
				}

				ImGui::Text("Fractal Type %f", noiseLayers[i].fractal);

				if (ImGui::Button("Change Fractal Type##GPUNL"))
				{
					noiseLayers[i].fractal += 1;
					if (noiseLayers[i].fractal == 4) noiseLayers[i].fractal = 0.0f;
					stateChanged |= true;
				}
			}

			if (ImGui::Button("Delete##GPUNL"))
			{
				appState->workManager->WaitForFinish();
				noiseLayers.erase(noiseLayers.begin() + i);
				stateChanged |= true;
				ImGui::PopID();
				break;
			}
			ImGui::Separator();
			ImGui::PopID();
		}

		if (ImGui::Button("Add##GPUNL"))
		{
			noiseLayers.push_back(GPUNoiseLayer());
			stateChanged |= true;
		}

		ImGui::End();
	}
	return stateChanged;
}
