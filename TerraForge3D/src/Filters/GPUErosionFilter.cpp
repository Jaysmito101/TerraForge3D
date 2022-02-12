#include "GPUErosionFilter.h"
#include <imgui.h>
#include <iostream>

#include <ComputeShader.h>
#include <ShaderStorageBuffer.h>

#include <Utils.h>

#include <string>
#include <regex> 
#define MAX(x, y) x>y?x:y
#define MIN(x, y) x<y?x:y

ComputeShader* shd;
ShaderStorageBuffer* heightMap_ssbo, *brushIndexOffset_ssbo, *brushWeights_ssbo, *randomIndex_ssbo;

static int randRange(int l, int h) {
	if (h < l) {
		int t = h;
		l = h;
		l = t;
	}
	return rand() % (h - l) + l;
}

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
	float maxHeight = model->mesh->maxHeight;
	float minHeight = model->mesh->minHeight;

	model->mesh->RecalculateNormals();
	mapSize = model->mesh->res;
	mapSize = MAX(0, mapSize - 2 * erosionBrushRadius);

	bool tmp = false;
	std::string shaderSrc = ReadShaderSourceFile(GetExecutableDir() + "\\Data\\compute\\erosion.glsl", &tmp);
	shaderSrc = std::regex_replace(shaderSrc, std::regex("LAYOUT_SIZE_X"), std::to_string(1024));
	//shaderSrc = std::regex_replace(shaderSrc, std::regex("LAYOUT_SIZE_X"), std::to_string(1));
	shaderSrc = std::regex_replace(shaderSrc, std::regex("LAYOUT_SIZE_Y"), std::to_string(1));
	shaderSrc = std::regex_replace(shaderSrc, std::regex("LAYOUT_SIZE_Z"), std::to_string(1));
	shaderSrc = std::regex_replace(shaderSrc, std::regex("MESH_RESOLUTION"), std::to_string(model->mesh->res));
	
	

	shd = new ComputeShader(shaderSrc);
	heightMap_ssbo = new ShaderStorageBuffer();

	
	brushIndexOffset_ssbo = new ShaderStorageBuffer();
	brushWeights_ssbo = new ShaderStorageBuffer();
	randomIndex_ssbo = new ShaderStorageBuffer();

	float* hMap = new float[model->mesh->vertexCount];
	for (int i = 0; i < model->mesh->vertexCount; i++) { 
		hMap[i] = (model->mesh->vert[i].position.y - minHeight)/maxHeight;
	}
	

	// Data Preparation

	int numThreads = numErosionIterations / 1024;

	std::vector<int> brushIndexOffsets;
	std::vector<float> brushWeights;

	float weightSum = 0;
	for (int brushY = -erosionBrushRadius; brushY <= erosionBrushRadius; brushY++) {
		for (int brushX = -erosionBrushRadius; brushX <= erosionBrushRadius; brushX++) {
			float sqrDst = brushX * brushX + brushY * brushY;
			if (sqrDst < erosionBrushRadius * erosionBrushRadius) {
				brushIndexOffsets.push_back(brushY * mapSize + brushX);
				float brushWeight = 1 - sqrt(sqrDst) / erosionBrushRadius;
				weightSum += brushWeight;
				brushWeights.push_back(brushWeight);
			}
		}
	}
	for (int i = 0; i < brushWeights.size(); i++) {
		brushWeights[i] /= weightSum;
	}

	srand(time(NULL));
	int* randomIndices = new int[numErosionIterations];
	for (int i = 0; i < numErosionIterations; i++) {
		int randomX = randRange(erosionBrushRadius, mapSize + erosionBrushRadius);
		int randomY = randRange(erosionBrushRadius, mapSize + erosionBrushRadius);
		randomIndices[i] = randomY * mapSize + randomX;
	}
	

	
	

	heightMap_ssbo->SetData(hMap, sizeof(float) * model->mesh->vertexCount);
	randomIndex_ssbo->SetData(randomIndices, sizeof(int) * numErosionIterations);
	brushIndexOffset_ssbo->SetData(brushIndexOffsets.data(), brushIndexOffsets.size() * sizeof(int));
	brushWeights_ssbo->SetData(brushWeights.data(), brushWeights.size() * sizeof(float));
	

	heightMap_ssbo->Bind(0); 
	randomIndex_ssbo->Bind(3);
	brushWeights_ssbo->Bind(2);
	brushIndexOffset_ssbo->Bind(1);

	shd->Bind();
	
	// SETTING UNIFORMS
	shd->SetUniformi("mapSize", model->mesh->res);
	shd->SetUniformi("brushLength", brushIndexOffsets.size());
	shd->SetUniformi("borderSize", erosionBrushRadius);
	shd->SetUniformi("maxLifetime", maxLifetime);

	shd->SetUniformf("inertia", inertia);
	shd->SetUniformf("sedimentCapacityFactor", sedimentCapacityFactor);
	shd->SetUniformf("minSedimentCapacity", minSedimentCapacity);
	shd->SetUniformf("depositSpeed", depositSpeed);
	shd->SetUniformf("errodeSpeed", errodeSpeed);
	shd->SetUniformf("evaporationSpeed", evaporationSpeed);
	shd->SetUniformf("gravity", gravity);
	shd->SetUniformf("startSpeed", startSpeed);
	shd->SetUniformf("startWater", startWater);
	
	

	
	shd->Dispatch(numThreads, 1, 1);
	shd->SetMemoryBarrier(); 


	heightMap_ssbo->Bind();
	heightMap_ssbo->GetData(hMap, sizeof(float) * model->mesh->vertexCount);
	
	for (int i = 0; i < model->mesh->vertexCount; i++) {
		model->mesh->vert[i].position.y = (hMap[i] * maxHeight) + minHeight;
	}
	 

	delete shd;
	delete heightMap_ssbo; 
	delete brushIndexOffset_ssbo;
	delete brushWeights_ssbo;
	delete randomIndex_ssbo;

	model->mesh->RecalculateNormals();
	model->UploadToGPU();
}

void GPUErosionFilter::OnAttach()
{

	
}
