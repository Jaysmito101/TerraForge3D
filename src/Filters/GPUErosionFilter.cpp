#include "GPUErosionFilter.h"
#include <imgui.h>
#include <iostream>

#include <ComputeShader.h>
#include <ShaderStorageBuffer.h>

#include <Utils.h>

#include <string>

#define MAX(x, y) x>y?x:y

std::string src = R"(
#version 430 core

layout(local_size_x=10) in;

layout(std430, binding = 0) buffer data
{
  float coordinate_array[1000];
};



void main(void)
{
 uint x = gl_GlobalInvocationID.x;
 coordinate_array[x] = x * 1.0f; 

}

)";

ComputeShader* shd;
ShaderStorageBuffer* ssbo;


static unsigned int tex;
void GPUErosionFilter::Render()
{
	ImGui::DragInt("Num Iterations##GPUErosionFilter", &iterations, 1, 0);
	ImGui::DragFloat("Actual Dimensions##GPUErosionFilter", &dimension, 1, 0);
	ImGui::DragFloat("Strength##GPUErosionFilter", &tfac, 1000);

	ImGui::Image((ImTextureID)tex, ImVec2(200, 200));

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

	shd = new ComputeShader(src);
	ssbo = new ShaderStorageBuffer();
	ssbo->SetData(NULL, sizeof(float) * 1000);
	ssbo->Bind(0);

	shd->Bind();
	shd->Dispatch(100, 1, 1);
	shd->SetMemoryBarrier();
	std::vector<float> storage(1000);

	glGetNamedBufferSubData(ssbo->rendererId, 0, 1000 * sizeof(float), storage.data());

	for (float t : storage) {
		std::cout << t << "\n";
	}

	delete shd;
	delete ssbo;

	model->mesh->RecalculateNormals();
	model->UploadToGPU();
}

void GPUErosionFilter::OnAttach()
{
	

}
