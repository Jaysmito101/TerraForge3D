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
ShaderStorageBuffer* ssbo;

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
	ImGui::DragInt("Num Iterations##GPUErosionFilter", &iterations, 1, 0);
	ImGui::DragFloat("Actual Dimensions##GPUErosionFilter", &dimension, 1, 0);
	ImGui::DragFloat("Strength##GPUErosionFilter", &tfac, 1000);

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
	ssbo = new ShaderStorageBuffer();
	ssbo->Bind(0);	

	float* verts = new float[model->mesh->vertexCount];
	for (int i = 0; i < model->mesh->vertexCount; i++) { 
		verts[i] = model->mesh->vert[i].position.y;
	}

	ssbo->SetData(verts, sizeof(float) * model->mesh->vertexCount);

	shd->Bind();
	shd->Dispatch(model->mesh->res, model->mesh->res, 1);
	shd->SetMemoryBarrier();
	delete verts;
	verts = new float[model->mesh->vertexCount];

	ssbo->GetData(verts, sizeof(float) * model->mesh->vertexCount);
	for (int i = 0; i < model->mesh->vertexCount; i++) {
		model->mesh->vert[i].position.y = verts[i];
	}





	delete shd;
	delete ssbo; 

	model->mesh->RecalculateNormals();
	model->UploadToGPU();
}

void GPUErosionFilter::OnAttach()
{

	
}
