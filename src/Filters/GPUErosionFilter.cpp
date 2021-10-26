#include "GPUErosionFilter.h"
#include <imgui.h>
#include <iostream>

#include <ComputeShader.h>
#include <ShaderStorageBuffer.h>

#include <Utils.h>

#include <string>
#include <regex> 
#define MAX(x, y) x>y?x:y

ComputeShader* shd;
ShaderStorageBuffer* heightMap_ssbo;

struct ResultRecord
{
	float a[10];
	float b[10];
	float c[10];
	float weight;
};

GLuint tex, buf;
void GPUErosionFilter::Render()
{
	ImGui::DragInt("Num Iterations##GPUErosionFilter", &numErosionIterations, 1, 1);
	ImGui::DragInt("Erosion Brush Radius##GPUErosionFilter", &erosionBrushRadius, 1, 1);
	ImGui::DragInt("Max Lifetime##GPUErosionFilter", &maxLifetime, 1, 1);
	
	ImGui::DragFloat("Sediment Capacity Factor##GPUErosionFilter", &sedimentCapacityFactor, 0.1f, 0);
	ImGui::DragFloat("Min Sediment Capacity##GPUErosionFilter", &minSedimentCapacity, 0.01f, 0);
	ImGui::DragFloat("Deposit Speed##GPUErosionFilter", &depositSpeed, 0.01f, 0);
	ImGui::DragFloat("Errode Speed##GPUErosionFilter", &errodeSpeed, 0.01f, 0);
	ImGui::DragFloat("Evaporation Speed##GPUErosionFilter", &evaporationSpeed, 0.01f, 0);
	ImGui::DragFloat("Gravity##GPUErosionFilter", &gravity, 0.01f, 0);
	ImGui::DragFloat("Start Speed##GPUErosionFilter", &startSpeed, 0.01f, 0);
	ImGui::DragFloat("Start Water##GPUErosionFilter", &startWater, 0.01f, 0);
	ImGui::DragFloat("Inertia##GPUErosionFilter", &inertia, 0.01f, 0, 1);
	

}

nlohmann::json GPUErosionFilter::Save()
{
	return nlohmann::json();
}

void GPUErosionFilter::Load(nlohmann::json data)
{
}

void GPUErosionFilter::Apply()
{
	model->mesh->RecalculateNormals();

	bool tmp = false;
	std::string shaderSrc = ReadShaderSourceFile(GetExecutableDir() + "\\Data\\compute\\erosion.glsl", &tmp);
	shaderSrc = std::regex_replace(shaderSrc, std::regex("LAYOUT_SIZE_X"), std::to_string(1));
	shaderSrc = std::regex_replace(shaderSrc, std::regex("LAYOUT_SIZE_Y"), std::to_string(1));
	shaderSrc = std::regex_replace(shaderSrc, std::regex("LAYOUT_SIZE_Z"), std::to_string(1));
	shaderSrc = std::regex_replace(shaderSrc, std::regex("MESH_RESOLUTION"), std::to_string(model->mesh->res));
	
	

	shd = new ComputeShader(shaderSrc);
	heightMap_ssbo = new ShaderStorageBuffer();
	heightMap_ssbo->Bind(0);	

	float* hMap = new float[model->mesh->vertexCount];
	for (int i = 0; i < model->mesh->vertexCount; i++) { 
		hMap[i] = model->mesh->vert[i].position.y;
	}

	heightMap_ssbo->SetData(hMap, sizeof(float) * model->mesh->vertexCount);

	shd->Bind();
	
	// SETTING UNIFORMS
	shd->SetUniformi("mapSize", model->mesh->res);
	shd->SetUniformi("brushLength", 0); // TODO
	shd->SetUniformi("borderSize", 0); // TODO
	shd->SetUniformi("maxLifetime", maxLifetime); // TODO

	shd->SetUniformf("inertia", inertia);
	shd->SetUniformf("sedimentCapacityFactor", sedimentCapacityFactor);
	shd->SetUniformf("minSedimentCapacity", minSedimentCapacity);
	shd->SetUniformf("depositSpeed", depositSpeed);
	shd->SetUniformf("errodeSpeed", errodeSpeed);
	shd->SetUniformf("evaporationSpeed", evaporationSpeed);
	shd->SetUniformf("gravity", gravity);
	shd->SetUniformf("startSpeed", startSpeed);
	shd->SetUniformf("startWater", startWater);
	

	shd->Dispatch(model->mesh->res, model->mesh->res, 1);
	shd->SetMemoryBarrier(); 

	heightMap_ssbo->GetData(hMap, sizeof(float) * model->mesh->vertexCount);
	
	for (int i = 0; i < model->mesh->vertexCount; i++) {
		model->mesh->vert[i].position.y = hMap[i];
	}
	




	delete shd;
	delete heightMap_ssbo; 

	model->mesh->RecalculateNormals();
	model->UploadToGPU();
}

void GPUErosionFilter::OnAttach()
{

	
}
