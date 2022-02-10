#include "GPUNoiseLayerGenerator.h"
#include "Utils.h"
#include "Data/ApplicationState.h"
#include "Profiler.h"

#include <regex>

static int count = 1;

GPUNoiseLayerGenerator::GPUNoiseLayerGenerator(ApplicationState* as, ComputeKernel* kernels)
{
    bool tmp = false;
    appState = as;
    uid = GenerateId(32);
    name = "GPU Noise Layer " + std::to_string(count++);   
    kernels->CreateBuffer("noise_layers_size", CL_MEM_READ_WRITE, sizeof(int));
}

void GPUNoiseLayerGenerator::Generate(ComputeKernel* kernels)
{
    START_PROFILER();

    kernels->CreateBuffer("noise_layers", CL_MEM_READ_WRITE, sizeof(GPUNoiseLayer) * noiseLayers.size() );
    kernels->WriteBuffer("noise_layers", true, sizeof(GPUNoiseLayer) * noiseLayers.size(), noiseLayers.data());
    int tmp = noiseLayers.size();
    kernels->WriteBuffer("noise_layers_size", true, sizeof(int), &tmp);


    if (appState->mode == ApplicationMode::TERRAIN)
    {
        kernels->SetKernelArg("noise_layer_terrain", 0, "mesh");
        kernels->SetKernelArg("noise_layer_terrain", 1, "noise_layers");
        kernels->SetKernelArg("noise_layer_terrain", 2, "noise_layers_size");

        vc = appState->models.coreTerrain->mesh->vertexCount;
        if (appState->models.coreTerrain->mesh->vertexCount % localSize != 0)
            localSize = 1;

        kernels->ExecuteKernel("noise_layer_terrain", cl::NDRange(localSize), cl::NDRange(appState->models.coreTerrain->mesh->vertexCount / localSize));
    }
    else if (appState->mode == ApplicationMode::CUSTOM_BASE)
    {        

        kernels->SetKernelArg("noise_layer_custom_base", 0, "mesh");
        kernels->SetKernelArg("noise_layer_custom_base", 1, "mesh_copy");
        kernels->SetKernelArg("noise_layer_custom_base", 2, "noise_layers");
        kernels->SetKernelArg("noise_layer_custom_base", 3, "noise_layers_size");

        vc = appState->models.customBase->mesh->vertexCount;
        //if (appState->models.customBase->mesh->vertexCount % localSize != 0)
        //    localSize = 1;

        kernels->ExecuteKernel("noise_layer_custom_base", cl::NDRange(localSize), cl::NDRange(appState->models.customBase->mesh->vertexCount));
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
    while (appState->states.remeshing);
    noiseLayers.clear();
    nlohmann::json tmp = data["nl"];
    for (int i = 0; i < tmp.size(); i++)
    {
        noiseLayers.push_back(LoadNoiseLayer(tmp[i]));
    }
} 

void GPUNoiseLayerGenerator::ShowSetting(int id)
{
    ImGui::Checkbox(("Enabled##GPUNL" + std::to_string(id)).c_str(), &enabled);
    ImGui::DragInt(("Local Work Group Size##GPUNL" + std::to_string(id)).c_str(), &localSize);
    if (ImGui::Button(("Edit##GPUNL" + std::to_string(id)).c_str()))
    {
        windowStat = true;
    }
    ImGui::Text("Vertices Processed : %d", vc);
    ImGui::Text("Time : %lf ms", time);
}
 
void GPUNoiseLayerGenerator::Update()
{
    if (windowStat)
    {
        ImGui::Begin((name + "##" + uid).c_str(), &windowStat);

        for (int i = 0; i < noiseLayers.size(); i++)
        {
            if (ImGui::CollapsingHeader(("Noise Layer " + std::to_string(i + 1)).c_str()))
            {
                 
                ImGui::Text("Noise Layer %d", (i + 1));
                ImGui::DragFloat(("Offset X" + std::string("##GPUNL") + std::to_string(i)).c_str(), &noiseLayers[i].offsetX, 0.01f);
                ImGui::DragFloat(("Offset Y" + std::string("##GPUNL") + std::to_string(i)).c_str(), &noiseLayers[i].offsetY, 0.01f);
                ImGui::DragFloat(("Offset Z" + std::string("##GPUNL") + std::to_string(i)).c_str(), &noiseLayers[i].offsetZ, 0.01f);
                ImGui::DragFloat(("Strength" + std::string("##GPUNL") + std::to_string(i)).c_str(), &noiseLayers[i].strength, 0.01f);
                ImGui::DragFloat(("Frequency" + std::string("##GPUNL") + std::to_string(i)).c_str(), &noiseLayers[i].frequency, 0.001f);
		ImGui::DragFloat(("Domain Wrap Depth" + std::string("##GPUNL") + std::to_string(i)).c_str(), &noiseLayers[i].domainWrapDepth, 0.1f);
                if (noiseLayers[i].fractal != 0)
                {
                    ImGui::DragFloat(("Octaves" + std::string("##GPUNL") + std::to_string(i)).c_str(), &noiseLayers[i].octaves, 0.01f, 1, 128);
                    ImGui::DragFloat(("Lacunarity" + std::string("##GPUNL") + std::to_string(i)).c_str(), &noiseLayers[i].lacunarity, 0.01f);
                    ImGui::DragFloat(("Gain" + std::string("##GPUNL") + std::to_string(i)).c_str(), &noiseLayers[i].gain, 0.01f);
                    ImGui::DragFloat(("Weighted Strength" + std::string("##GPUNL") + std::to_string(i)).c_str(), &noiseLayers[i].weightedStrength, 0.01f);
                    ImGui::DragFloat(("Ping Pong Strength" + std::string("##GPUNL") + std::to_string(i)).c_str(), &noiseLayers[i].pingPongStrength, 0.01f);
                }
                ImGui::Text("Fractal Type %f", noiseLayers[i].fractal);
                if (ImGui::Button(("Change Fractal Type##GPUNL" + std::to_string(i)).c_str()))
                {
                    noiseLayers[i].fractal += 1;
                    if (noiseLayers[i].fractal == 4)
                        noiseLayers[i].fractal = 0.0f;
                }
            }
            if (ImGui::Button(("Delete" + std::string("##GPUNL") + std::to_string(i)).c_str()))
            {
                while (appState->states.remeshing);
                noiseLayers.erase(noiseLayers.begin() + i);
                break;
            }
            ImGui::Separator();
        }
            if (ImGui::Button("Add##GPUNL"))
            {
                noiseLayers.push_back(GPUNoiseLayer());
            }

        ImGui::End();
            
    }
}
